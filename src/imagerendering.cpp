// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendering.h"

#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>
#include <Qt>
#include <algorithm>
#include <cmath>
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
    if (imageSize.isEmpty() || boundsSize.isEmpty()) {
        return {};
    }

    const qreal scale = std::min(
        boundsSize.width() / imageSize.width(), boundsSize.height() / imageSize.height());
    const QSizeF targetSize(imageSize.width() * scale, imageSize.height() * scale);
    return QRectF((boundsSize.width() - targetSize.width()) / 2.0,
        (boundsSize.height() - targetSize.height()) / 2.0, targetSize.width(), targetSize.height());
}

QSize scaledImageSizeToFit(const QSizeF &imageSize, const QSize &boundsSize)
{
    if (imageSize.isEmpty() || boundsSize.isEmpty()) {
        return {};
    }

    const qreal imageWidth = imageSize.width();
    const qreal imageHeight = imageSize.height();
    if (!std::isfinite(imageWidth) || !std::isfinite(imageHeight) || imageWidth <= 0.0
        || imageHeight <= 0.0) {
        return {};
    }

    const qreal scale = std::min<qreal>(
        1.0, std::min(boundsSize.width() / imageWidth, boundsSize.height() / imageHeight));
    if (!std::isfinite(scale) || scale <= 0.0) {
        return {};
    }

    return QSize(std::clamp(static_cast<int>(std::ceil(imageWidth * scale)), 1, boundsSize.width()),
        std::clamp(static_cast<int>(std::ceil(imageHeight * scale)), 1, boundsSize.height()));
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
    if (displaySize.isEmpty() || !std::isfinite(devicePixelRatio) || devicePixelRatio <= 0.0) {
        return {};
    }

    const qreal width = displaySize.width() * devicePixelRatio;
    const qreal height = displaySize.height() * devicePixelRatio;
    const int maximumSize = maximumTextureSize > 0 ? maximumTextureSize : fallbackTextureSizeMax;
    return scaledImageSizeToFit(QSizeF(width, height), QSize(maximumSize, maximumSize));
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
