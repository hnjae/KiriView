// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilegeometrypolicy.h"

#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/imagetilegeometry.cxx.h"

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
    return KiriView::RustTileKey { key.level, key.x, key.y, key.scaleBucket };
}

KiriView::TileKey tileKeyFromRust(const KiriView::RustTileKey &key)
{
    return KiriView::TileKey { key.level, key.x, key.y, key.scale_bucket };
}

KiriView::TileLevel tileLevelFromRust(const KiriView::RustTileLevel &level)
{
    return KiriView::TileLevel { level.index, KiriView::Bridge::qtSize(level.size) };
}

KiriView::ActiveTileLayer activeTileLayerFromRust(const KiriView::RustActiveTileLayer &layer)
{
    return KiriView::ActiveTileLayer { layer.level, layer.scale_bucket };
}

KiriView::TileRequest tileRequestFromRust(const KiriView::RustTileRequest &request)
{
    return KiriView::TileRequest {
        tileKeyFromRust(request.key),
        KiriView::Bridge::qtSize(request.level_size),
        KiriView::Bridge::qtRect(request.level_rect),
        KiriView::Bridge::qtRect(request.texture_level_rect),
        KiriView::Bridge::qtRect(request.source_rect),
        KiriView::Bridge::qtRect(request.display_source_rect),
    };
}

}

namespace KiriView::ImageTileGeometryPolicy {
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

ActiveTileLayer activeTileLayer(const QSize &imageSize, const QSizeF &displaySize,
    qreal devicePixelRatio, int rotationDegrees, bool resolutionIndependent)
{
    return activeTileLayerFromRust(rustActiveTileLayer(rustTileSize(imageSize),
        rustTileSizeF(displaySize), devicePixelRatio, rotationDegrees, resolutionIndependent));
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

std::vector<TileRequest> svgRasterTileRequests(const QSize &imageSize, int tileSize,
    int apronSourcePixels, const QSizeF &displaySize, const QRectF &visibleItemRect,
    qreal devicePixelRatio)
{
    std::vector<TileRequest> requests;
    const rust::Vec<RustTileRequest> rustRequests
        = rustSvgRasterTileRequests(rustTileSize(imageSize), tileSize, apronSourcePixels,
            rustTileSizeF(displaySize), rustTileRectF(visibleItemRect), devicePixelRatio);
    requests.reserve(rustRequests.size());
    for (const RustTileRequest &request : rustRequests) {
        requests.push_back(tileRequestFromRust(request));
    }
    return requests;
}

}
