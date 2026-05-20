// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetile.h"

#include "imagetilegeometrybridge.h"

#include <algorithm>
#include <utility>

namespace KiriView {
uint qHash(const TileKey &key, uint seed)
{
    return qHashMulti(seed, key.level, key.x, key.y, key.scaleBucket);
}

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
    return ImageTileGeometryBridge::tilePyramidTileGridSize(m_imageSize, m_tileSize, level);
}

bool TilePyramid::containsLevel(int level) const
{
    return ImageTileGeometryBridge::tilePyramidContainsLevel(levelCount(), level);
}

bool TilePyramid::containsTile(const TileKey &key) const
{
    return ImageTileGeometryBridge::tilePyramidContainsTile(m_imageSize, m_tileSize, key);
}

int TilePyramid::selectLevelForDisplayScale(qreal displayPixelsPerSourcePixel) const
{
    return ImageTileGeometryBridge::tilePyramidSelectLevelForDisplayScale(
        m_imageSize, displayPixelsPerSourcePixel);
}

QRect TilePyramid::levelTileRect(const TileKey &key) const
{
    return ImageTileGeometryBridge::tilePyramidLevelTileRect(m_imageSize, m_tileSize, key);
}

QRect TilePyramid::levelTileTextureRect(const TileKey &key) const
{
    return ImageTileGeometryBridge::tilePyramidLevelTileTextureRect(
        m_imageSize, m_tileSize, m_apronSourcePixels, key);
}

QRect TilePyramid::sourceRectForLevelRect(int level, const QRect &levelRect) const
{
    return ImageTileGeometryBridge::tilePyramidSourceRectForLevelRect(
        m_imageSize, level, levelRect);
}

TileRequest TilePyramid::requestForTile(const TileKey &key) const
{
    TileRequest request = ImageTileGeometryBridge::tilePyramidRequestForTile(
        m_imageSize, m_tileSize, m_apronSourcePixels, key);
    request.key = key;
    request.displaySourceRect = sourceRectForLevelRect(key.level, request.levelRect);
    return request;
}

std::vector<TileKey> TilePyramid::tilesIntersectingLevelRect(
    int level, const QRect &levelRect) const
{
    return ImageTileGeometryBridge::tilePyramidTilesIntersectingLevelRect(
        m_imageSize, m_tileSize, level, levelRect);
}

void TilePyramid::rebuildLevels()
{
    m_levels = ImageTileGeometryBridge::tilePyramidLevels(m_imageSize);
}

qreal TilePyramid::levelPixelsPerSourcePixel(int level) const
{
    return ImageTileGeometryBridge::tilePyramidLevelPixelsPerSourcePixel(m_imageSize, level);
}
}
