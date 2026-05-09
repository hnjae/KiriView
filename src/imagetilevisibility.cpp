// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilevisibility.h"

#include "kiriview/src/imagetilegeometry.cxx.h"

namespace {
KiriView::RustTileSize rustTileSize(const QSize &size)
{
    return KiriView::RustTileSize { size.width(), size.height() };
}

KiriView::RustTileSizeF rustTileSizeF(const QSizeF &size)
{
    return KiriView::RustTileSizeF { size.width(), size.height() };
}

KiriView::RustTileRectF rustTileRectF(const QRectF &rect)
{
    return KiriView::RustTileRectF { rect.x(), rect.y(), rect.width(), rect.height() };
}

QRect qtRect(const KiriView::RustTileRect &rect)
{
    return QRect(rect.x, rect.y, rect.width, rect.height);
}

KiriView::TileKey tileKeyFromRust(const KiriView::RustTileKey &key)
{
    return KiriView::TileKey { key.level, key.x, key.y };
}
}

namespace KiriView {
qreal tileDisplayPixelsPerSourcePixel(
    const TilePyramid &pyramid, const QSizeF &displaySize, qreal devicePixelRatio)
{
    return rustTileDisplayPixelsPerSourcePixel(
        rustTileSize(pyramid.imageSize()), rustTileSizeF(displaySize), devicePixelRatio);
}

bool tileFirstDisplayIsSufficient(const TilePyramid &pyramid, const QSizeF &displaySize,
    qreal devicePixelRatio, qreal firstDisplayPixelsPerSourcePixel)
{
    return rustTileFirstDisplayIsSufficient(rustTileSize(pyramid.imageSize()),
        rustTileSizeF(displaySize), devicePixelRatio, firstDisplayPixelsPerSourcePixel);
}

QRect tileLevelRectForItemRect(
    const TilePyramid &pyramid, int level, const QSizeF &displaySize, const QRectF &itemRect)
{
    return qtRect(rustTileLevelRectForItemRect(rustTileSize(pyramid.imageSize()), level,
        rustTileSizeF(displaySize), rustTileRectF(itemRect)));
}

std::vector<TileKey> visibleTileKeys(
    const TilePyramid &pyramid, const TileVisibilityContext &context)
{
    std::vector<TileKey> keys;
    const rust::Vec<RustTileKey> rustKeys = rustVisibleTileKeys(rustTileSize(pyramid.imageSize()),
        pyramid.tileSize(), rustTileSizeF(context.displaySize),
        rustTileRectF(context.visibleItemRect), context.devicePixelRatio);
    keys.reserve(rustKeys.size());
    for (const RustTileKey &key : rustKeys) {
        keys.push_back(tileKeyFromRust(key));
    }

    return keys;
}
}
