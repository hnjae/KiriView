// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DECODEDTILECACHE_H
#define KIRIVIEW_DECODEDTILECACHE_H

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
    struct Entry {
        DecodedTile tile;
        qsizetype byteCost = 0;
        quint64 lastUse = 0;
    };

    void trimToBudget();
    std::vector<Entry>::iterator findEntry(const TileKey &key);
    std::vector<Entry>::const_iterator findEntry(const TileKey &key) const;

    std::vector<Entry> m_entries;
    qsizetype m_byteBudget = 0;
    qsizetype m_byteCost = 0;
    quint64 m_useClock = 0;
};
}

#endif
