// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DECODEDTILECACHE_H
#define KIRIVIEW_DECODEDTILECACHE_H

#include "cache/imagelrucachestate.h"
#include "imagetile.h"

#include <QtGlobal>
#include <optional>
#include <vector>

namespace KiriView {
qsizetype decodedTileByteCost(const DecodedTile &tile);

class DecodedTileCache
{
public:
    explicit DecodedTileCache(qsizetype byteBudget);

    qsizetype byteBudget() const;
    qsizetype byteCost() const;
    bool contains(const TileKey &key) const;
    std::optional<DecodedTile> find(const TileKey &key);
    std::vector<DecodedTile> tiles() const;
    bool insert(DecodedTile tile);
    void clear();

private:
    ImageLruCacheState<TileKey, DecodedTile> m_cache;
};
}

#endif
