// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendering.h"

#include "kiriview/src/policy/imagerendergeometry.cxx.h"
#include "presentation/imagerotation.h"
#include "qtgeometryconversion.h"

#include <QRect>
#include <QRectF>
#include <optional>
#include <utility>
#include <vector>

namespace {
KiriView::ImageSurfaceDrawEntry drawEntry(
    const QImage &image, const QRectF &targetRect, const QRectF &textureRect, int rotationDegrees)
{
    return KiriView::ImageSurfaceDrawEntry {
        image,
        targetRect,
        textureRect,
        KiriView::imageTextureCoordinateTransform(textureRect, rotationDegrees),
    };
}

KiriView::ImageSurfaceDrawEntry fullImageDrawEntry(
    const QImage &image, const QRectF &targetRect, int rotationDegrees)
{
    return drawEntry(image, targetRect, QRectF(0.0, 0.0, 1.0, 1.0), rotationDegrees);
}

QRectF tileTargetRect(
    const QRect &sourceRect, const QSize &imageSize, const QRectF &targetRect, int rotationDegrees)
{
    if (KiriView::normalizedImageRotationDegrees(rotationDegrees) != 0) {
        return KiriView::rotatedSourceRectInTarget(
            QRectF(sourceRect), QSizeF(imageSize), targetRect, rotationDegrees);
    }

    return KiriView::Bridge::qtRectF(KiriView::rustImageTileTargetRect(
        KiriView::Bridge::rustRect<KiriView::RustImageRenderRect>(sourceRect),
        KiriView::Bridge::rustSize<KiriView::RustImageRenderSize>(imageSize),
        KiriView::Bridge::rustRectF<KiriView::RustImageRenderRectF>(targetRect)));
}

QRectF tileTextureRect(const KiriView::DecodedTile &tile)
{
    return KiriView::Bridge::qtRectF(KiriView::rustImageTileTextureRect(
        KiriView::Bridge::rustRect<KiriView::RustImageRenderRect>(tile.levelRect),
        KiriView::Bridge::rustRect<KiriView::RustImageRenderRect>(tile.textureLevelRect)));
}

std::optional<KiriView::ImageSurfaceDrawEntry> staticTileDrawEntry(
    const KiriView::StaticTileSurface &surface, const KiriView::DecodedTile &tile,
    const QRectF &targetRect, int rotationDegrees)
{
    const QSize imageSize = surface.imageSize();
    if (imageSize.isEmpty() || tile.textureLevelRect.isEmpty() || tile.image.isNull()) {
        return std::nullopt;
    }

    QRect sourceRect = tile.displaySourceRect;
    if (sourceRect.isEmpty()) {
        sourceRect = surface.pyramid().sourceRectForLevelRect(tile.key.level, tile.levelRect);
    }
    if (sourceRect.isEmpty()) {
        return std::nullopt;
    }

    const QRectF textureRect = tileTextureRect(tile);
    return drawEntry(tile.image, tileTargetRect(sourceRect, imageSize, targetRect, rotationDegrees),
        textureRect, rotationDegrees);
}

void appendLegacySurfaceDrawEntries(std::vector<KiriView::ImageSurfaceDrawEntry> *entries,
    const KiriView::LegacyFrameSurface &surface, const QRectF &targetRect, int rotationDegrees)
{
    if (!surface.image.isNull()) {
        entries->push_back(fullImageDrawEntry(surface.image, targetRect, rotationDegrees));
    }
}

void appendStaticSurfaceDrawEntries(std::vector<KiriView::ImageSurfaceDrawEntry> *entries,
    const KiriView::StaticTileSurface &surface, const QRectF &targetRect, int rotationDegrees)
{
    if (!surface.isValid()) {
        return;
    }

    entries->push_back(fullImageDrawEntry(surface.preview(), targetRect, rotationDegrees));
    for (const KiriView::DecodedTile &tile : surface.tiles()) {
        std::optional<KiriView::ImageSurfaceDrawEntry> entry
            = staticTileDrawEntry(surface, tile, targetRect, rotationDegrees);
        if (entry.has_value()) {
            entries->push_back(std::move(*entry));
        }
    }
}
}

namespace KiriView {
QRectF imageTargetRect(const QSize &imageSize, const QSizeF &boundsSize)
{
    return Bridge::qtRectF(rustImageTargetRect(Bridge::rustSize<RustImageRenderSize>(imageSize),
        Bridge::rustSizeF<RustImageRenderSizeF>(boundsSize)));
}

QSize scaledImageSizeToFit(const QSizeF &imageSize, const QSize &boundsSize)
{
    return Bridge::qtSize(
        rustScaledImageSizeToFit(Bridge::rustSizeF<RustImageRenderSizeF>(imageSize),
            Bridge::rustSize<RustImageRenderSize>(boundsSize)));
}

QSize firstDisplayScaledImageSize(const QSize &imageSize, const QSize &physicalViewportSize)
{
    return Bridge::qtSize(
        rustFirstDisplayScaledImageSize(Bridge::rustSize<RustImageRenderSize>(imageSize),
            Bridge::rustSize<RustImageRenderSize>(physicalViewportSize)));
}

qreal imagePixelsPerSourcePixel(const QSize &imageSize, const QSize &displaySize)
{
    return rustImagePixelsPerSourcePixel(Bridge::rustSize<RustImageRenderSize>(imageSize),
        Bridge::rustSize<RustImageRenderSize>(displaySize));
}

std::vector<ImageSurfaceDrawEntry> imageSurfaceDrawEntries(
    const DisplayedImageSurface &surface, const QRectF &targetRect, int rotationDegrees)
{
    std::vector<ImageSurfaceDrawEntry> entries;
    if (targetRect.isEmpty()) {
        return entries;
    }

    if (const auto *legacySurface = surface.legacyFrameSurface()) {
        appendLegacySurfaceDrawEntries(&entries, *legacySurface, targetRect, rotationDegrees);
    } else if (const auto *staticSurface = surface.staticTileSurface()) {
        appendStaticSurfaceDrawEntries(&entries, *staticSurface, targetRect, rotationDegrees);
    }
    return entries;
}

QImage displayReadyImage(const QImage &image)
{
    if (image.format() == QImage::Format_RGBA8888_Premultiplied) {
        return image;
    }
    return image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
}

QSize svgRasterSize(const QSizeF &displaySize, qreal devicePixelRatio, int maximumTextureSize)
{
    return Bridge::qtSize(rustSvgRasterSize(Bridge::rustSizeF<RustImageRenderSizeF>(displaySize),
        devicePixelRatio, maximumTextureSize, fallbackTextureSizeMax));
}

ImageDocumentRenderContext normalizedImageDocumentRenderContext(ImageDocumentRenderContext context)
{
    const RustImageDocumentRenderContext normalized = rustNormalizedImageDocumentRenderContext(
        RustImageDocumentRenderContext { context.devicePixelRatio, context.maximumTextureSize },
        fallbackTextureSizeMax);
    return ImageDocumentRenderContext {
        normalized.device_pixel_ratio,
        normalized.maximum_texture_size,
    };
}

ImageFirstDisplayDecodeContext imageFirstDisplayDecodeContext(
    const QSizeF &viewportSize, qreal devicePixelRatio)
{
    return ImageFirstDisplayDecodeContext {
        Bridge::qtSize(rustFirstDisplayPhysicalViewportSize(
            Bridge::rustSizeF<RustImageRenderSizeF>(viewportSize), devicePixelRatio)),
    };
}
}
