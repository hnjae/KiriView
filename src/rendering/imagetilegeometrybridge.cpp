// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilegeometrybridge.h"

#include "kiriview/src/imagetilegeometry.cxx.h"
#include "qtgeometryconversion.h"

namespace {
KiriView::RustTileSize rustTileSize(const QSize &size)
{
    return KiriView::Bridge::rustSize<KiriView::RustTileSize>(size);
}

KiriView::RustTileSizeF rustTileSizeF(const QSizeF &size)
{
    return KiriView::Bridge::rustSizeF<KiriView::RustTileSizeF>(size);
}

KiriView::RustTileRect rustTileRect(const QRect &rect)
{
    return KiriView::Bridge::rustRect<KiriView::RustTileRect>(rect);
}

KiriView::RustTileRectF rustTileRectF(const QRectF &rect)
{
    return KiriView::Bridge::rustRectF<KiriView::RustTileRectF>(rect);
}

KiriView::RustTileKey rustTileKey(const KiriView::TileKey &key)
{
    return KiriView::RustTileKey { key.level, key.x, key.y };
}

KiriView::TileKey tileKeyFromRust(const KiriView::RustTileKey &key)
{
    return KiriView::TileKey { key.level, key.x, key.y };
}

KiriView::TileLevel tileLevelFromRust(const KiriView::RustTileLevel &level)
{
    return KiriView::TileLevel { level.index, KiriView::Bridge::qtSize(level.size) };
}

KiriView::TileRequest tileRequestFromRust(const KiriView::RustTileRequest &request)
{
    return KiriView::TileRequest {
        tileKeyFromRust(request.key),
        KiriView::Bridge::qtSize(request.level_size),
        KiriView::Bridge::qtRect(request.level_rect),
        KiriView::Bridge::qtRect(request.texture_level_rect),
        KiriView::Bridge::qtRect(request.source_rect),
        {},
    };
}

}

namespace KiriView::ImageTileGeometryBridge {
QRect boundedIntegerRect(const QRect &rect, const QSize &boundsSize)
{
    return Bridge::qtRect(rustBoundedIntegerRect(rustTileRect(rect), rustTileSize(boundsSize)));
}

QRect scaledIntegerRect(const QRectF &rect, const QSizeF &sourceSize, const QSize &targetSize)
{
    return Bridge::qtRect(rustScaledIntegerRect(
        rustTileRectF(rect), rustTileSizeF(sourceSize), rustTileSize(targetSize)));
}

std::vector<TileLevel> tilePyramidLevels(const QSize &imageSize)
{
    std::vector<TileLevel> levels;
    const rust::Vec<RustTileLevel> rustLevels = rustTilePyramidLevels(rustTileSize(imageSize));
    levels.reserve(rustLevels.size());
    for (const RustTileLevel &level : rustLevels) {
        levels.push_back(tileLevelFromRust(level));
    }
    return levels;
}

QSize tilePyramidTileGridSize(const QSize &imageSize, int tileSize, int level)
{
    return Bridge::qtSize(rustTilePyramidTileGridSize(rustTileSize(imageSize), tileSize, level));
}

bool tilePyramidContainsLevel(int levelCount, int level)
{
    return rustTilePyramidContainsLevel(levelCount, level);
}

bool tilePyramidContainsTile(const QSize &imageSize, int tileSize, const TileKey &key)
{
    return rustTilePyramidContainsTile(rustTileSize(imageSize), tileSize, rustTileKey(key));
}

int tilePyramidSelectLevelForDisplayScale(const QSize &imageSize, qreal displayPixelsPerSourcePixel)
{
    return rustTilePyramidSelectLevelForDisplayScale(
        rustTileSize(imageSize), displayPixelsPerSourcePixel);
}

QRect tilePyramidLevelTileRect(const QSize &imageSize, int tileSize, const TileKey &key)
{
    return Bridge::qtRect(
        rustTilePyramidLevelTileRect(rustTileSize(imageSize), tileSize, rustTileKey(key)));
}

QRect tilePyramidLevelTileTextureRect(
    const QSize &imageSize, int tileSize, int apronSourcePixels, const TileKey &key)
{
    return Bridge::qtRect(rustTilePyramidLevelTileTextureRect(
        rustTileSize(imageSize), tileSize, apronSourcePixels, rustTileKey(key)));
}

QRect tilePyramidSourceRectForLevelRect(const QSize &imageSize, int level, const QRect &levelRect)
{
    return Bridge::qtRect(rustTilePyramidSourceRectForLevelRect(
        rustTileSize(imageSize), level, rustTileRect(levelRect)));
}

TileRequest tilePyramidRequestForTile(
    const QSize &imageSize, int tileSize, int apronSourcePixels, const TileKey &key)
{
    return tileRequestFromRust(rustTilePyramidRequestForTile(
        rustTileSize(imageSize), tileSize, apronSourcePixels, rustTileKey(key)));
}

std::vector<TileKey> tilePyramidTilesIntersectingLevelRect(
    const QSize &imageSize, int tileSize, int level, const QRect &levelRect)
{
    std::vector<TileKey> keys;
    const rust::Vec<RustTileKey> rustKeys = rustTilePyramidTilesIntersectingLevelRect(
        rustTileSize(imageSize), tileSize, level, rustTileRect(levelRect));
    keys.reserve(rustKeys.size());
    for (const RustTileKey &key : rustKeys) {
        keys.push_back(tileKeyFromRust(key));
    }
    return keys;
}

qreal tilePyramidLevelPixelsPerSourcePixel(const QSize &imageSize, int level)
{
    return rustTilePyramidLevelPixelsPerSourcePixel(rustTileSize(imageSize), level);
}

qreal tileDisplayPixelsPerSourcePixel(
    const QSize &imageSize, const QSizeF &displaySize, qreal devicePixelRatio)
{
    return rustTileDisplayPixelsPerSourcePixel(
        rustTileSize(imageSize), rustTileSizeF(displaySize), devicePixelRatio);
}

bool tileFirstDisplayIsSufficient(const QSize &imageSize, const QSizeF &displaySize,
    qreal devicePixelRatio, qreal firstDisplayPixelsPerSourcePixel)
{
    return rustTileFirstDisplayIsSufficient(rustTileSize(imageSize), rustTileSizeF(displaySize),
        devicePixelRatio, firstDisplayPixelsPerSourcePixel);
}

QRect tileLevelRectForItemRect(
    const QSize &imageSize, int level, const QSizeF &displaySize, const QRectF &itemRect)
{
    return Bridge::qtRect(rustTileLevelRectForItemRect(
        rustTileSize(imageSize), level, rustTileSizeF(displaySize), rustTileRectF(itemRect)));
}

std::vector<TileKey> visibleTileKeys(const QSize &imageSize, int tileSize,
    const QSizeF &displaySize, const QRectF &visibleItemRect, qreal devicePixelRatio)
{
    std::vector<TileKey> keys;
    const rust::Vec<RustTileKey> rustKeys = rustVisibleTileKeys(rustTileSize(imageSize), tileSize,
        rustTileSizeF(displaySize), rustTileRectF(visibleItemRect), devicePixelRatio);
    keys.reserve(rustKeys.size());
    for (const RustTileKey &key : rustKeys) {
        keys.push_back(tileKeyFromRust(key));
    }
    return keys;
}

}
