// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendering.h"

#include "kiriview/src/imagerendergeometry.cxx.h"
#include "qtgeometryconversion.h"

#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>
#include <Qt>
#include <optional>
#include <utility>
#include <vector>

namespace {
KiriView::ImageSurfaceDrawEntry fullImageDrawEntry(const QImage &image, const QRectF &targetRect)
{
    return KiriView::ImageSurfaceDrawEntry {
        image,
        targetRect,
        QRectF(0.0, 0.0, 1.0, 1.0),
    };
}

QRectF tileTargetRect(const QRect &sourceRect, const QSize &imageSize, const QRectF &targetRect)
{
    return QRectF(targetRect.x()
            + (static_cast<qreal>(sourceRect.x()) / imageSize.width()) * targetRect.width(),
        targetRect.y()
            + (static_cast<qreal>(sourceRect.y()) / imageSize.height()) * targetRect.height(),
        (static_cast<qreal>(sourceRect.width()) / imageSize.width()) * targetRect.width(),
        (static_cast<qreal>(sourceRect.height()) / imageSize.height()) * targetRect.height());
}

QRectF tileTextureRect(const KiriView::DecodedTile &tile)
{
    return QRectF(static_cast<qreal>(tile.levelRect.x() - tile.textureLevelRect.x())
            / tile.textureLevelRect.width(),
        static_cast<qreal>(tile.levelRect.y() - tile.textureLevelRect.y())
            / tile.textureLevelRect.height(),
        static_cast<qreal>(tile.levelRect.width()) / tile.textureLevelRect.width(),
        static_cast<qreal>(tile.levelRect.height()) / tile.textureLevelRect.height());
}

std::optional<KiriView::ImageSurfaceDrawEntry> staticTileDrawEntry(
    const KiriView::StaticTileSurface &surface, const KiriView::DecodedTile &tile,
    const QRectF &targetRect)
{
    const QSize imageSize = surface.imageSize();
    if (imageSize.isEmpty() || tile.textureLevelRect.isEmpty() || tile.image.isNull()) {
        return std::nullopt;
    }

    const QRect sourceRect
        = surface.pyramid().sourceRectForLevelRect(tile.key.level, tile.levelRect);
    if (sourceRect.isEmpty()) {
        return std::nullopt;
    }

    return KiriView::ImageSurfaceDrawEntry {
        tile.image,
        tileTargetRect(sourceRect, imageSize, targetRect),
        tileTextureRect(tile),
    };
}

void appendLegacySurfaceDrawEntries(std::vector<KiriView::ImageSurfaceDrawEntry> *entries,
    const KiriView::LegacyFrameSurface &surface, const QRectF &targetRect)
{
    if (!surface.image.isNull()) {
        entries->push_back(fullImageDrawEntry(surface.image, targetRect));
    }
}

void appendStaticSurfaceDrawEntries(std::vector<KiriView::ImageSurfaceDrawEntry> *entries,
    const KiriView::StaticTileSurface &surface, const QRectF &targetRect)
{
    if (!surface.isValid()) {
        return;
    }

    entries->push_back(fullImageDrawEntry(surface.preview(), targetRect));
    for (const KiriView::DecodedTile &tile : surface.tiles()) {
        std::optional<KiriView::ImageSurfaceDrawEntry> entry
            = staticTileDrawEntry(surface, tile, targetRect);
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

std::vector<ImageSurfaceDrawEntry> imageSurfaceDrawEntries(
    const DisplayedImageSurface &surface, const QRectF &targetRect)
{
    std::vector<ImageSurfaceDrawEntry> entries;
    if (targetRect.isEmpty()) {
        return entries;
    }

    if (const auto *legacySurface = surface.legacyFrameSurface()) {
        appendLegacySurfaceDrawEntries(&entries, *legacySurface, targetRect);
    } else if (const auto *staticSurface = surface.staticTileSurface()) {
        appendStaticSurfaceDrawEntries(&entries, *staticSurface, targetRect);
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

QImage renderSvgImage(const QByteArray &data, const QSize &size)
{
    if (data.isEmpty() || size.isEmpty()) {
        return {};
    }

    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return {};
    }

    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    if (image.isNull()) {
        return {};
    }

    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.render(&painter, QRectF(QPointF(0.0, 0.0), QSizeF(size)));
    return image;
}
}
