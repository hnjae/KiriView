// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagerendering.h"

#include <QPainter>
#include <QRectF>
#include <QSvgRenderer>
#include <Qt>
#include <algorithm>
#include <cmath>

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

std::vector<ImageSurfaceDrawEntry> imageSurfaceDrawEntries(
    const DisplayedImageSurface &surface, const QRectF &targetRect)
{
    std::vector<ImageSurfaceDrawEntry> entries;
    if (targetRect.isEmpty()) {
        return entries;
    }

    const auto addLegacy = [&](const LegacyFrameSurface &legacySurface) {
        if (!legacySurface.image.isNull()) {
            entries.push_back(ImageSurfaceDrawEntry {
                legacySurface.image,
                targetRect,
                QRectF(0.0, 0.0, 1.0, 1.0),
            });
        }
    };
    const auto addStatic = [&](const StaticTileSurface &staticSurface) {
        if (!staticSurface.isValid()) {
            return;
        }

        entries.push_back(ImageSurfaceDrawEntry {
            staticSurface.preview(),
            targetRect,
            QRectF(0.0, 0.0, 1.0, 1.0),
        });

        const QSize imageSize = staticSurface.imageSize();
        if (imageSize.isEmpty()) {
            return;
        }

        for (const DecodedTile &tile : staticSurface.tiles()) {
            const QRect sourceRect
                = staticSurface.pyramid().sourceRectForLevelRect(tile.key.level, tile.levelRect);
            if (sourceRect.isEmpty() || tile.textureLevelRect.isEmpty() || tile.image.isNull()) {
                continue;
            }

            const QRectF tileTargetRect(targetRect.x()
                    + (static_cast<qreal>(sourceRect.x()) / imageSize.width()) * targetRect.width(),
                targetRect.y()
                    + (static_cast<qreal>(sourceRect.y()) / imageSize.height())
                        * targetRect.height(),
                (static_cast<qreal>(sourceRect.width()) / imageSize.width()) * targetRect.width(),
                (static_cast<qreal>(sourceRect.height()) / imageSize.height())
                    * targetRect.height());
            const QRectF textureRect(
                static_cast<qreal>(tile.levelRect.x() - tile.textureLevelRect.x())
                    / tile.textureLevelRect.width(),
                static_cast<qreal>(tile.levelRect.y() - tile.textureLevelRect.y())
                    / tile.textureLevelRect.height(),
                static_cast<qreal>(tile.levelRect.width()) / tile.textureLevelRect.width(),
                static_cast<qreal>(tile.levelRect.height()) / tile.textureLevelRect.height());
            entries.push_back(ImageSurfaceDrawEntry {
                tile.image,
                tileTargetRect,
                textureRect,
            });
        }
    };

    if (const auto *legacySurface = surface.legacyFrameSurface()) {
        addLegacy(*legacySurface);
    } else if (const auto *staticSurface = surface.staticTileSurface()) {
        addStatic(*staticSurface);
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
    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0 || height <= 0.0) {
        return {};
    }

    const int maximumSize = maximumTextureSize > 0 ? maximumTextureSize : fallbackTextureSizeMax;
    const qreal scale = std::min<qreal>(1.0, std::min(maximumSize / width, maximumSize / height));
    return QSize(std::clamp(static_cast<int>(std::ceil(width * scale)), 1, maximumSize),
        std::clamp(static_cast<int>(std::ceil(height * scale)), 1, maximumSize));
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
