// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILE_H
#define KIRIVIEW_IMAGETILE_H

#include <QHash>
#include <QImage>
#include <QRect>
#include <QSize>
#include <QtGlobal>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
inline constexpr int imageTileCoreSize = 512;
inline constexpr int imageTileApronSourcePixels = 1;

struct TileKey {
    int level = 0;
    int x = 0;
    int y = 0;

    friend bool operator==(const TileKey &left, const TileKey &right)
    {
        return left.level == right.level && left.x == right.x && left.y == right.y;
    }

    friend bool operator!=(const TileKey &left, const TileKey &right) { return !(left == right); }
};

uint qHash(const TileKey &key, uint seed = 0);

struct TileLevel {
    int index = 0;
    QSize size;
};

struct TileRequest {
    TileKey key;
    QSize levelSize;
    QRect levelRect;
    QRect textureLevelRect;
    QRect sourceRect;
};

struct DecodedTile {
    TileKey key;
    QSize levelSize;
    QRect levelRect;
    QRect textureLevelRect;
    QImage image;
};

qsizetype decodedTileByteCost(const DecodedTile &tile);

class TilePyramid
{
public:
    explicit TilePyramid(QSize imageSize = QSize(), int tileSize = imageTileCoreSize,
        int apronSourcePixels = imageTileApronSourcePixels);

    QSize imageSize() const;
    int tileSize() const;
    int apronSourcePixels() const;
    int levelCount() const;
    QSize levelSize(int level) const;
    QSize tileGridSize(int level) const;
    bool containsLevel(int level) const;
    bool containsTile(const TileKey &key) const;
    int selectLevelForDisplayScale(qreal displayPixelsPerSourcePixel) const;
    QRect levelTileRect(const TileKey &key) const;
    QRect levelTileTextureRect(const TileKey &key) const;
    QRect sourceRectForLevelRect(int level, const QRect &levelRect) const;
    TileRequest requestForTile(const TileKey &key) const;
    std::vector<TileKey> tilesIntersectingLevelRect(int level, const QRect &levelRect) const;

private:
    void rebuildLevels();
    qreal levelPixelsPerSourcePixel(int level) const;

    QSize m_imageSize;
    int m_tileSize = imageTileCoreSize;
    int m_apronSourcePixels = imageTileApronSourcePixels;
    std::vector<TileLevel> m_levels;
};

class DecodedTileCache
{
public:
    explicit DecodedTileCache(qsizetype byteBudget);

    qsizetype byteBudget() const;
    qsizetype byteCost() const;
    bool contains(const TileKey &key) const;
    std::optional<DecodedTile> find(const TileKey &key);
    std::vector<DecodedTile> tiles() const;
    void insert(DecodedTile tile);
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
