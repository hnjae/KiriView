// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendering.h"

#include "bridge/qtgeometryconversion.h"
#include "imagetilevisibility.h"
#include "kiriview/src/policy/imagerendergeometry.cxx.h"
#include "presentation/imagerotation.h"

#include <QRect>
#include <QRectF>
#include <optional>
#include <utility>
#include <vector>

namespace {
KiriView::ImageSurfaceDrawEntry drawEntry(KiriView::ImageSurfaceDrawIdentity identity,
    const QImage &image, const QRectF &targetRect, const QRectF &textureRect, int rotationDegrees)
{
    return KiriView::ImageSurfaceDrawEntry {
        identity,
        image,
        targetRect,
        textureRect,
        KiriView::imageTextureCoordinateTransform(textureRect, rotationDegrees),
    };
}

KiriView::ImageSurfaceDrawEntry fullImageDrawEntry(KiriView::ImageSurfaceDrawIdentity identity,
    const QImage &image, const QRectF &targetRect, int rotationDegrees)
{
    return drawEntry(identity, image, targetRect, QRectF(0.0, 0.0, 1.0, 1.0), rotationDegrees);
}

QRectF tileTargetRect(
    const QRectF &sourceRect, const QSize &imageSize, const QRectF &targetRect, int rotationDegrees)
{
    if (KiriView::normalizedImageRotationDegrees(rotationDegrees) != 0) {
        return KiriView::rotatedSourceRectInTarget(
            sourceRect, QSizeF(imageSize), targetRect, rotationDegrees);
    }

    return KiriView::Bridge::qtRectF(KiriView::rustImageTileTargetRectF(
        KiriView::Bridge::rustRectF<KiriView::RustImageRenderRectF>(sourceRect),
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
    const KiriView::ImageSurfaceDrawContext &context, bool resolutionIndependent)
{
    const QSize imageSize = surface.imageSize();
    if (imageSize.isEmpty() || tile.textureLevelRect.isEmpty() || tile.image.isNull()) {
        return std::nullopt;
    }

    QRectF sourceRect = resolutionIndependent ? tile.displaySourceRectF : QRectF();
    if (sourceRect.isEmpty()) {
        sourceRect = QRectF(tile.displaySourceRect);
    }
    if (sourceRect.isEmpty()) {
        sourceRect
            = QRectF(surface.pyramid().sourceRectForLevelRect(tile.key.level, tile.levelRect));
    }
    if (sourceRect.isEmpty()) {
        return std::nullopt;
    }

    const QRectF textureRect = tileTextureRect(tile);
    const QRectF tileTarget
        = tileTargetRect(sourceRect, imageSize, context.targetRect, context.rotationDegrees);
    if (!context.visibleItemRect.isEmpty() && !tileTarget.intersects(context.visibleItemRect)) {
        return std::nullopt;
    }

    return drawEntry(
        KiriView::ImageSurfaceDrawIdentity {
            KiriView::ImageSurfaceDrawIdentityKind::Tile,
            tile.key,
            resolutionIndependent,
        },
        tile.image, tileTarget, textureRect, context.rotationDegrees);
}

void appendLegacySurfaceDrawEntries(std::vector<KiriView::ImageSurfaceDrawEntry> *entries,
    const KiriView::LegacyFrameSurface &surface, const KiriView::ImageSurfaceDrawContext &context)
{
    if (!surface.image.isNull()) {
        entries->push_back(fullImageDrawEntry(
            KiriView::ImageSurfaceDrawIdentity {
                KiriView::ImageSurfaceDrawIdentityKind::LegacyFrame,
                {},
                false,
            },
            surface.image, context.targetRect, context.rotationDegrees));
    }
}

void appendStaticSurfaceDrawEntries(std::vector<KiriView::ImageSurfaceDrawEntry> *entries,
    const KiriView::StaticTileSurface &surface, const KiriView::ImageSurfaceDrawContext &context)
{
    if (!surface.isValid()) {
        return;
    }

    entries->push_back(fullImageDrawEntry(
        KiriView::ImageSurfaceDrawIdentity {
            KiriView::ImageSurfaceDrawIdentityKind::Preview,
            {},
            false,
        },
        surface.preview(), context.targetRect, context.rotationDegrees));
    const std::shared_ptr<KiriView::ImageTileSource> source = surface.source();
    const bool resolutionIndependent = source != nullptr && source->isResolutionIndependent();
    const KiriView::ActiveTileLayer activeLayer = KiriView::activeTileLayer(surface.pyramid(),
        KiriView::TileVisibilityContext {
            context.displaySize,
            context.visibleItemRect,
            context.devicePixelRatio,
            context.rotationDegrees,
        },
        resolutionIndependent);
    for (const KiriView::DecodedTile &tile : surface.tiles()) {
        if (!activeLayer.contains(tile.key)) {
            continue;
        }

        std::optional<KiriView::ImageSurfaceDrawEntry> entry
            = staticTileDrawEntry(surface, tile, context, resolutionIndependent);
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
    const DisplayedImageSurface &surface, const ImageSurfaceDrawContext &context)
{
    std::vector<ImageSurfaceDrawEntry> entries;
    if (context.targetRect.isEmpty()) {
        return entries;
    }

    if (const auto *legacySurface = surface.legacyFrameSurface()) {
        appendLegacySurfaceDrawEntries(&entries, *legacySurface, context);
    } else if (const auto *staticSurface = surface.staticTileSurface()) {
        appendStaticSurfaceDrawEntries(&entries, *staticSurface, context);
    }
    return entries;
}

std::vector<ImageSurfaceDrawEntry> imageSurfaceDrawEntries(
    const DisplayedImageSurface &surface, const QRectF &targetRect, int rotationDegrees)
{
    return imageSurfaceDrawEntries(surface,
        ImageSurfaceDrawContext {
            targetRect,
            targetRect.size(),
            targetRect,
            1.0,
            rotationDegrees,
        });
}

std::vector<ImageSurfaceDrawIdentity> imageSurfaceDrawIdentities(
    const std::vector<ImageSurfaceDrawEntry> &entries)
{
    std::vector<ImageSurfaceDrawIdentity> identities;
    identities.reserve(entries.size());
    for (const ImageSurfaceDrawEntry &entry : entries) {
        identities.push_back(entry.identity);
    }
    return identities;
}

QImage displayReadyImage(const QImage &image)
{
    if (image.format() == QImage::Format_RGBA8888_Premultiplied) {
        return image;
    }
    return image.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
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
