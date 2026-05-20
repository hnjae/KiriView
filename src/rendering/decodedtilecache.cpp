// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedtilecache.h"

#include "cache/imagebytecost.h"

#include <utility>

namespace KiriView {
qsizetype decodedTileByteCost(const DecodedTile &tile) { return imageByteCost(tile.image); }

DecodedTileCache::DecodedTileCache(qsizetype byteBudget)
    : m_cache(byteBudget)
{
}

qsizetype DecodedTileCache::byteBudget() const { return m_cache.byteBudget(); }

qsizetype DecodedTileCache::byteCost() const { return m_cache.byteCost(); }

bool DecodedTileCache::contains(const TileKey &key) const { return m_cache.contains(key); }

std::optional<DecodedTile> DecodedTileCache::find(const TileKey &key) { return m_cache.find(key); }

std::vector<DecodedTile> DecodedTileCache::tiles() const { return m_cache.values(); }

bool DecodedTileCache::insert(DecodedTile tile)
{
    const qsizetype byteCost = decodedTileByteCost(tile);
    const TileKey key = tile.key;
    return m_cache.insert(key, std::move(tile), byteCost);
}

void DecodedTileCache::clear() { m_cache.clear(); }
}
