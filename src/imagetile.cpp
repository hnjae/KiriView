// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetile.h"

#include "kiriview/src/imagetilegeometry.cxx.h"
#include "qtgeometryconversion.h"

#include <algorithm>
#include <utility>

namespace {
KiriView::RustTileSize rustTileSize(const QSize &size)
{
    return KiriView::Bridge::rustSize<KiriView::RustTileSize>(size);
}

KiriView::RustTileRect rustTileRect(const QRect &rect)
{
    return KiriView::Bridge::rustRect<KiriView::RustTileRect>(rect);
}

KiriView::RustTileKey rustTileKey(const KiriView::TileKey &key)
{
    return KiriView::RustTileKey { key.level, key.x, key.y };
}

KiriView::TileKey tileKeyFromRust(const KiriView::RustTileKey &key)
{
    return KiriView::TileKey { key.level, key.x, key.y };
}

KiriView::TileRequest tileRequestFromRust(const KiriView::RustTileRequest &request)
{
    return KiriView::TileRequest {
        tileKeyFromRust(request.key),
        KiriView::Bridge::qtSize(request.level_size),
        KiriView::Bridge::qtRect(request.level_rect),
        KiriView::Bridge::qtRect(request.texture_level_rect),
        KiriView::Bridge::qtRect(request.source_rect),
    };
}

}

namespace KiriView {
uint qHash(const TileKey &key, uint seed) { return qHashMulti(seed, key.level, key.x, key.y); }

TilePyramid::TilePyramid(QSize imageSize, int tileSize, int apronSourcePixels)
    : m_imageSize(std::move(imageSize))
    , m_tileSize(std::max(1, tileSize))
    , m_apronSourcePixels(std::max(0, apronSourcePixels))
{
    rebuildLevels();
}

QSize TilePyramid::imageSize() const { return m_imageSize; }

int TilePyramid::tileSize() const { return m_tileSize; }

int TilePyramid::apronSourcePixels() const { return m_apronSourcePixels; }

int TilePyramid::levelCount() const { return static_cast<int>(m_levels.size()); }

QSize TilePyramid::levelSize(int level) const
{
    if (!containsLevel(level)) {
        return {};
    }

    return m_levels[static_cast<std::size_t>(level)].size;
}

QSize TilePyramid::tileGridSize(int level) const
{
    return Bridge::qtSize(
        rustTilePyramidTileGridSize(rustTileSize(m_imageSize), m_tileSize, level));
}

bool TilePyramid::containsLevel(int level) const
{
    return rustTilePyramidContainsLevel(levelCount(), level);
}

bool TilePyramid::containsTile(const TileKey &key) const
{
    return rustTilePyramidContainsTile(rustTileSize(m_imageSize), m_tileSize, rustTileKey(key));
}

int TilePyramid::selectLevelForDisplayScale(qreal displayPixelsPerSourcePixel) const
{
    return rustTilePyramidSelectLevelForDisplayScale(
        rustTileSize(m_imageSize), displayPixelsPerSourcePixel);
}

QRect TilePyramid::levelTileRect(const TileKey &key) const
{
    return Bridge::qtRect(
        rustTilePyramidLevelTileRect(rustTileSize(m_imageSize), m_tileSize, rustTileKey(key)));
}

QRect TilePyramid::levelTileTextureRect(const TileKey &key) const
{
    return Bridge::qtRect(rustTilePyramidLevelTileTextureRect(
        rustTileSize(m_imageSize), m_tileSize, m_apronSourcePixels, rustTileKey(key)));
}

QRect TilePyramid::sourceRectForLevelRect(int level, const QRect &levelRect) const
{
    return Bridge::qtRect(rustTilePyramidSourceRectForLevelRect(
        rustTileSize(m_imageSize), level, rustTileRect(levelRect)));
}

TileRequest TilePyramid::requestForTile(const TileKey &key) const
{
    return tileRequestFromRust(rustTilePyramidRequestForTile(
        rustTileSize(m_imageSize), m_tileSize, m_apronSourcePixels, rustTileKey(key)));
}

std::vector<TileKey> TilePyramid::tilesIntersectingLevelRect(
    int level, const QRect &levelRect) const
{
    std::vector<TileKey> keys;
    const rust::Vec<RustTileKey> rustKeys = rustTilePyramidTilesIntersectingLevelRect(
        rustTileSize(m_imageSize), m_tileSize, level, rustTileRect(levelRect));
    keys.reserve(rustKeys.size());
    for (const RustTileKey &key : rustKeys) {
        keys.push_back(tileKeyFromRust(key));
    }
    return keys;
}

void TilePyramid::rebuildLevels()
{
    m_levels.clear();
    const rust::Vec<RustTileLevel> rustLevels = rustTilePyramidLevels(rustTileSize(m_imageSize));
    m_levels.reserve(rustLevels.size());
    for (const RustTileLevel &level : rustLevels) {
        m_levels.push_back(TileLevel { level.index, Bridge::qtSize(level.size) });
    }
}

qreal TilePyramid::levelPixelsPerSourcePixel(int level) const
{
    return rustTilePyramidLevelPixelsPerSourcePixel(rustTileSize(m_imageSize), level);
}
}
