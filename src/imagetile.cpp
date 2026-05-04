// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetile.h"

#include "imagebytecost.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {
QSize halfSize(const QSize &size)
{
    return QSize(std::max(1, (size.width() + 1) / 2), std::max(1, (size.height() + 1) / 2));
}

QRect boundedRect(const QRect &rect, const QSize &size)
{
    return rect.intersected(QRect(QPoint(0, 0), size));
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
    const QSize size = levelSize(level);
    if (size.isEmpty()) {
        return {};
    }

    return QSize((size.width() + m_tileSize - 1) / m_tileSize,
        (size.height() + m_tileSize - 1) / m_tileSize);
}

bool TilePyramid::containsLevel(int level) const { return level >= 0 && level < levelCount(); }

bool TilePyramid::containsTile(const TileKey &key) const
{
    const QSize grid = tileGridSize(key.level);
    return !grid.isEmpty() && key.x >= 0 && key.y >= 0 && key.x < grid.width()
        && key.y < grid.height();
}

int TilePyramid::selectLevelForDisplayScale(qreal displayPixelsPerSourcePixel) const
{
    if (m_levels.empty()) {
        return 0;
    }
    if (!std::isfinite(displayPixelsPerSourcePixel) || displayPixelsPerSourcePixel <= 0.0) {
        return static_cast<int>(m_levels.size()) - 1;
    }

    int selectedLevel = 0;
    for (int level = 0; level < levelCount(); ++level) {
        if (levelPixelsPerSourcePixel(level) + std::numeric_limits<qreal>::epsilon()
            >= displayPixelsPerSourcePixel) {
            selectedLevel = level;
        }
    }
    return selectedLevel;
}

QRect TilePyramid::levelTileRect(const TileKey &key) const
{
    if (!containsTile(key)) {
        return {};
    }

    const QRect tileRect(key.x * m_tileSize, key.y * m_tileSize, m_tileSize, m_tileSize);
    return boundedRect(tileRect, levelSize(key.level));
}

QRect TilePyramid::levelTileTextureRect(const TileKey &key) const
{
    const QRect tileRect = levelTileRect(key);
    if (tileRect.isEmpty()) {
        return {};
    }

    const int apron = std::min(m_tileSize,
        std::max(1,
            static_cast<int>(
                std::ceil(m_apronSourcePixels * levelPixelsPerSourcePixel(key.level)))));
    return boundedRect(tileRect.adjusted(-apron, -apron, apron, apron), levelSize(key.level));
}

QRect TilePyramid::sourceRectForLevelRect(int level, const QRect &levelRect) const
{
    const QSize levelSize = this->levelSize(level);
    if (m_imageSize.isEmpty() || levelSize.isEmpty() || levelRect.isEmpty()) {
        return {};
    }

    const QRect boundedLevelRect = boundedRect(levelRect, levelSize);
    const qreal xScale = static_cast<qreal>(m_imageSize.width()) / levelSize.width();
    const qreal yScale = static_cast<qreal>(m_imageSize.height()) / levelSize.height();
    const int left = std::clamp(
        static_cast<int>(std::floor(boundedLevelRect.left() * xScale)), 0, m_imageSize.width());
    const int top = std::clamp(
        static_cast<int>(std::floor(boundedLevelRect.top() * yScale)), 0, m_imageSize.height());
    const int right
        = std::clamp(static_cast<int>(std::ceil((boundedLevelRect.right() + 1) * xScale)), left,
            m_imageSize.width());
    const int bottom
        = std::clamp(static_cast<int>(std::ceil((boundedLevelRect.bottom() + 1) * yScale)), top,
            m_imageSize.height());
    return QRect(left, top, right - left, bottom - top);
}

TileRequest TilePyramid::requestForTile(const TileKey &key) const
{
    const QRect levelRect = levelTileRect(key);
    const QRect textureLevelRect = levelTileTextureRect(key);
    return TileRequest {
        key,
        levelSize(key.level),
        levelRect,
        textureLevelRect,
        sourceRectForLevelRect(key.level, textureLevelRect),
    };
}

std::vector<TileKey> TilePyramid::tilesIntersectingLevelRect(
    int level, const QRect &levelRect) const
{
    std::vector<TileKey> keys;
    const QSize grid = tileGridSize(level);
    const QSize size = levelSize(level);
    if (grid.isEmpty() || size.isEmpty() || levelRect.isEmpty()) {
        return keys;
    }

    const QRect bounded = boundedRect(levelRect, size);
    if (bounded.isEmpty()) {
        return keys;
    }

    const int left = bounded.left() / m_tileSize;
    const int top = bounded.top() / m_tileSize;
    const int right = bounded.right() / m_tileSize;
    const int bottom = bounded.bottom() / m_tileSize;
    keys.reserve(static_cast<std::size_t>((right - left + 1) * (bottom - top + 1)));
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            keys.push_back(TileKey { level, x, y });
        }
    }
    return keys;
}

void TilePyramid::rebuildLevels()
{
    m_levels.clear();
    if (m_imageSize.isEmpty()) {
        return;
    }

    QSize size = m_imageSize;
    int level = 0;
    while (true) {
        m_levels.push_back(TileLevel { level, size });
        if (size.width() == 1 && size.height() == 1) {
            break;
        }
        size = halfSize(size);
        ++level;
    }
}

qreal TilePyramid::levelPixelsPerSourcePixel(int level) const
{
    const QSize size = levelSize(level);
    if (m_imageSize.isEmpty() || size.isEmpty()) {
        return 0.0;
    }

    return std::min(static_cast<qreal>(size.width()) / m_imageSize.width(),
        static_cast<qreal>(size.height()) / m_imageSize.height());
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

void DecodedTileCache::insert(DecodedTile tile)
{
    const qsizetype byteCost = decodedTileByteCost(tile);
    if (byteCost <= 0 || byteCost > m_byteBudget) {
        return;
    }

    auto existing = findEntry(tile.key);
    if (existing != m_entries.end()) {
        m_byteCost -= existing->byteCost;
        m_entries.erase(existing);
    }

    m_entries.push_back(Entry { std::move(tile), byteCost, ++m_useClock });
    m_byteCost += byteCost;
    trimToBudget();
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
