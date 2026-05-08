// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetile.h"

#include "imagebytecost.h"
#include "kiriview/src/imagetilegeometry.cxx.h"

#include <algorithm>
#include <utility>

namespace {
KiriView::RustTileSize rustTileSize(const QSize &size)
{
    return KiriView::RustTileSize { size.width(), size.height() };
}

KiriView::RustTileRect rustTileRect(const QRect &rect)
{
    return KiriView::RustTileRect { rect.x(), rect.y(), rect.width(), rect.height() };
}

KiriView::RustTileKey rustTileKey(const KiriView::TileKey &key)
{
    return KiriView::RustTileKey { key.level, key.x, key.y };
}

QSize qtSize(const KiriView::RustTileSize &size) { return QSize(size.width, size.height); }

QRect qtRect(const KiriView::RustTileRect &rect)
{
    return QRect(rect.x, rect.y, rect.width, rect.height);
}

KiriView::TileKey tileKeyFromRust(const KiriView::RustTileKey &key)
{
    return KiriView::TileKey { key.level, key.x, key.y };
}

KiriView::TileRequest tileRequestFromRust(const KiriView::RustTileRequest &request)
{
    return KiriView::TileRequest {
        tileKeyFromRust(request.key),
        qtSize(request.level_size),
        qtRect(request.level_rect),
        qtRect(request.texture_level_rect),
        qtRect(request.source_rect),
    };
}
}

namespace KiriView {
uint qHash(const TileKey &key, uint seed) { return qHashMulti(seed, key.level, key.x, key.y); }

qsizetype decodedTileByteCost(const DecodedTile &tile) { return imageByteCost(tile.image); }

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
    return qtSize(rustTilePyramidTileGridSize(rustTileSize(m_imageSize), m_tileSize, level));
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
    return qtRect(
        rustTilePyramidLevelTileRect(rustTileSize(m_imageSize), m_tileSize, rustTileKey(key)));
}

QRect TilePyramid::levelTileTextureRect(const TileKey &key) const
{
    return qtRect(rustTilePyramidLevelTileTextureRect(
        rustTileSize(m_imageSize), m_tileSize, m_apronSourcePixels, rustTileKey(key)));
}

QRect TilePyramid::sourceRectForLevelRect(int level, const QRect &levelRect) const
{
    return qtRect(rustTilePyramidSourceRectForLevelRect(
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
        m_levels.push_back(TileLevel { level.index, qtSize(level.size) });
    }
}

qreal TilePyramid::levelPixelsPerSourcePixel(int level) const
{
    return rustTilePyramidLevelPixelsPerSourcePixel(rustTileSize(m_imageSize), level);
}

DecodedTileCache::DecodedTileCache(qsizetype byteBudget)
    : m_byteBudget(std::max<qsizetype>(0, byteBudget))
{
}

qsizetype DecodedTileCache::byteBudget() const { return m_byteBudget; }

qsizetype DecodedTileCache::byteCost() const { return m_byteCost; }

bool DecodedTileCache::contains(const TileKey &key) const
{
    return findEntry(key) != m_entries.cend();
}

std::optional<DecodedTile> DecodedTileCache::find(const TileKey &key)
{
    auto entry = findEntry(key);
    if (entry == m_entries.end()) {
        return std::nullopt;
    }

    entry->lastUse = ++m_useClock;
    return entry->tile;
}

std::vector<DecodedTile> DecodedTileCache::tiles() const
{
    std::vector<DecodedTile> tiles;
    tiles.reserve(m_entries.size());
    for (const Entry &entry : m_entries) {
        tiles.push_back(entry.tile);
    }
    return tiles;
}

bool DecodedTileCache::insert(DecodedTile tile)
{
    const qsizetype byteCost = decodedTileByteCost(tile);
    if (byteCost <= 0 || byteCost > m_byteBudget) {
        return false;
    }

    auto existing = findEntry(tile.key);
    if (existing != m_entries.end()) {
        m_byteCost -= existing->byteCost;
        m_entries.erase(existing);
    }

    m_entries.push_back(Entry { std::move(tile), byteCost, ++m_useClock });
    m_byteCost += byteCost;
    trimToBudget();
    return true;
}

void DecodedTileCache::clear()
{
    m_entries.clear();
    m_byteCost = 0;
}

void DecodedTileCache::trimToBudget()
{
    while (m_byteCost > m_byteBudget && !m_entries.empty()) {
        const auto oldest = std::min_element(m_entries.begin(), m_entries.end(),
            [](const Entry &left, const Entry &right) { return left.lastUse < right.lastUse; });
        m_byteCost -= oldest->byteCost;
        m_entries.erase(oldest);
    }
}

std::vector<DecodedTileCache::Entry>::iterator DecodedTileCache::findEntry(const TileKey &key)
{
    return std::find_if(m_entries.begin(), m_entries.end(),
        [&key](const Entry &entry) { return entry.tile.key == key; });
}

std::vector<DecodedTileCache::Entry>::const_iterator DecodedTileCache::findEntry(
    const TileKey &key) const
{
    return std::find_if(m_entries.cbegin(), m_entries.cend(),
        [&key](const Entry &entry) { return entry.tile.key == key; });
}
}
