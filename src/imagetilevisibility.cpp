// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilevisibility.h"

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

    const QRectF bounded = itemRect.intersected(QRectF(QPointF(0.0, 0.0), displaySize));
    if (bounded.isEmpty()) {
        return {};
    }

    const QSize levelSize = pyramid.levelSize(level);
    if (levelSize.isEmpty()) {
        return {};
    }

    const qreal xScale = static_cast<qreal>(levelSize.width()) / displaySize.width();
    const qreal yScale = static_cast<qreal>(levelSize.height()) / displaySize.height();
    const int left
        = std::clamp(static_cast<int>(std::floor(bounded.left() * xScale)), 0, levelSize.width());
    const int top
        = std::clamp(static_cast<int>(std::floor(bounded.top() * yScale)), 0, levelSize.height());
    const int right = std::clamp(
        static_cast<int>(std::ceil(bounded.right() * xScale)), left, levelSize.width());
    const int bottom = std::clamp(
        static_cast<int>(std::ceil(bounded.bottom() * yScale)), top, levelSize.height());
    return QRect(left, top, right - left, bottom - top);
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
