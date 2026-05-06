// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilevisibility.h"

#include "imagerectmapping.h"

#include <QPointF>
#include <algorithm>
#include <cmath>

namespace KiriView {
qreal tileDisplayPixelsPerSourcePixel(
    const TilePyramid &pyramid, const QSizeF &displaySize, qreal devicePixelRatio)
{
    if (pyramid.imageSize().isEmpty() || displaySize.isEmpty() || !std::isfinite(devicePixelRatio)
        || devicePixelRatio <= 0.0) {
        return 0.0;
    }

    return std::min((displaySize.width() * devicePixelRatio) / pyramid.imageSize().width(),
        (displaySize.height() * devicePixelRatio) / pyramid.imageSize().height());
}

QRect tileLevelRectForItemRect(
    const TilePyramid &pyramid, int level, const QSizeF &displaySize, const QRectF &itemRect)
{
    if (itemRect.isEmpty() || displaySize.isEmpty() || pyramid.imageSize().isEmpty()) {
        return {};
    }

    const QSize levelSize = pyramid.levelSize(level);
    if (levelSize.isEmpty()) {
        return {};
    }

    return scaledIntegerRect(itemRect, displaySize, levelSize);
}

std::vector<TileKey> visibleTileKeys(
    const TilePyramid &pyramid, const TileVisibilityContext &context)
{
    std::vector<TileKey> keys;
    if (pyramid.imageSize().isEmpty() || context.displaySize.isEmpty()
        || context.visibleItemRect.isEmpty()) {
        return keys;
    }

    const int level = pyramid.selectLevelForDisplayScale(
        tileDisplayPixelsPerSourcePixel(pyramid, context.displaySize, context.devicePixelRatio));
    const QRect currentLevelRect
        = tileLevelRectForItemRect(pyramid, level, context.displaySize, context.visibleItemRect);
    keys = pyramid.tilesIntersectingLevelRect(level, currentLevelRect);

    QRectF prefetchItemRect = context.visibleItemRect.adjusted(-context.visibleItemRect.width(),
        -context.visibleItemRect.height(), context.visibleItemRect.width(),
        context.visibleItemRect.height());
    prefetchItemRect = prefetchItemRect.intersected(QRectF(QPointF(0.0, 0.0), context.displaySize));
    const QRect prefetchLevelRect
        = tileLevelRectForItemRect(pyramid, level, context.displaySize, prefetchItemRect);
    for (const TileKey &key : pyramid.tilesIntersectingLevelRect(level, prefetchLevelRect)) {
        if (std::find(keys.cbegin(), keys.cend(), key) == keys.cend()) {
            keys.push_back(key);
        }
    }

    return keys;
}
}
