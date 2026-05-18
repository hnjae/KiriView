// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedtilecache.h"

#include "imagebytecost.h"
#include "kiriview/src/imagetilegeometry.cxx.h"

#include <algorithm>
#include <cstdint>
#include <utility>

namespace {
std::int64_t rustTileByteCost(qsizetype byteCost) { return static_cast<std::int64_t>(byteCost); }

std::uint64_t rustTileUseClock(quint64 useClock) { return static_cast<std::uint64_t>(useClock); }
}

namespace KiriView {
qsizetype decodedTileByteCost(const DecodedTile &tile) { return imageByteCost(tile.image); }

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
    rust::Vec<std::int64_t> byteCosts;
    rust::Vec<std::uint64_t> lastUses;
    byteCosts.reserve(m_entries.size());
    lastUses.reserve(m_entries.size());

    for (const Entry &entry : m_entries) {
        byteCosts.push_back(rustTileByteCost(entry.byteCost));
        lastUses.push_back(rustTileUseClock(entry.lastUse));
    }

    const rust::Vec<std::size_t> retainedIndices = rustTileCacheRetainedIndices(
        std::move(byteCosts), std::move(lastUses), rustTileByteCost(m_byteBudget));

    std::vector<Entry> retainedEntries;
    retainedEntries.reserve(retainedIndices.size());
    qsizetype retainedByteCost = 0;
    for (std::size_t index : retainedIndices) {
        if (index >= m_entries.size()) {
            continue;
        }

        retainedByteCost += m_entries[index].byteCost;
        retainedEntries.push_back(std::move(m_entries[index]));
    }

    m_entries = std::move(retainedEntries);
    m_byteCost = retainedByteCost;
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
