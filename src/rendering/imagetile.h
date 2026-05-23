// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGETILE_H
#define KIRIVIEW_IMAGETILE_H

#include <QHash>
#include <QImage>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <vector>

namespace KiriView {
inline constexpr int imageTileCoreSize = 512;
inline constexpr int imageTileApronSourcePixels = 1;

struct TileKey {
    int level = 0;
    int x = 0;
    int y = 0;
    int scaleBucket = 0;

    friend bool operator==(const TileKey &left, const TileKey &right)
    {
        return left.level == right.level && left.x == right.x && left.y == right.y
            && left.scaleBucket == right.scaleBucket;
    }

    friend bool operator!=(const TileKey &left, const TileKey &right) { return !(left == right); }
};

uint qHash(const TileKey &key, uint seed = 0);

struct ActiveTileLayer {
    int level = 0;
    int scaleBucket = 0;

    bool contains(const TileKey &key) const
    {
        return key.level == level && key.scaleBucket == scaleBucket;
    }
};

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
    QRect displaySourceRect;
    QRectF displaySourceRectF;
};

struct DecodedTile {
    TileKey key;
    QSize levelSize;
    QRect levelRect;
    QRect textureLevelRect;
    QImage image;
    QRect displaySourceRect;
    QRectF displaySourceRectF;
};

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

}

#endif
