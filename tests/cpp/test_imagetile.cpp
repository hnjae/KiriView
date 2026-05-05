// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetile.h"

#include <QImage>
#include <QObject>
#include <QSize>
#include <QTest>
#include <Qt>
#include <optional>

namespace {
KiriView::DecodedTile decodedTile(KiriView::TileKey key, QSize size)
{
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    return KiriView::DecodedTile { key, size, QRect(QPoint(0, 0), size), QRect(QPoint(0, 0), size),
        image };
}
}

class TestImageTile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pyramidBuildsDownsampleLevels();
    void levelSelectionUsesSmallestLevelAtLeastAsDenseAsScreenPixels();
    void tileRectsAndApronsAreClipped();
    void tilesIntersectingVisibleRectAreReturnedInScanOrder();
    void cacheEvictsLeastRecentlyUsedTilesToBudget();
    void cacheReportsRejectedTiles();
};

void TestImageTile::pyramidBuildsDownsampleLevels()
{
    const KiriView::TilePyramid pyramid(QSize(1025, 513));

    QCOMPARE(pyramid.levelCount(), 12);
    QCOMPARE(pyramid.levelSize(0), QSize(1025, 513));
    QCOMPARE(pyramid.levelSize(1), QSize(513, 257));
    QCOMPARE(pyramid.levelSize(11), QSize(1, 1));
    QCOMPARE(pyramid.tileGridSize(0), QSize(3, 2));
}

void TestImageTile::levelSelectionUsesSmallestLevelAtLeastAsDenseAsScreenPixels()
{
    const KiriView::TilePyramid pyramid(QSize(4096, 4096));

    QCOMPARE(pyramid.selectLevelForDisplayScale(1.25), 0);
    QCOMPARE(pyramid.selectLevelForDisplayScale(1.0), 0);
    QCOMPARE(pyramid.selectLevelForDisplayScale(0.75), 0);
    QCOMPARE(pyramid.selectLevelForDisplayScale(0.5), 1);
    QCOMPARE(pyramid.selectLevelForDisplayScale(0.26), 1);
    QCOMPARE(pyramid.selectLevelForDisplayScale(0.25), 2);
    QCOMPARE(pyramid.selectLevelForDisplayScale(0.01), 6);
}

void TestImageTile::tileRectsAndApronsAreClipped()
{
    const KiriView::TilePyramid pyramid(QSize(1025, 513));

    const KiriView::TileKey topLeft { 0, 0, 0 };
    QCOMPARE(pyramid.levelTileRect(topLeft), QRect(0, 0, 512, 512));
    QCOMPARE(pyramid.levelTileTextureRect(topLeft), QRect(0, 0, 513, 513));
    QCOMPARE(pyramid.sourceRectForLevelRect(0, pyramid.levelTileTextureRect(topLeft)),
        QRect(0, 0, 513, 513));

    const KiriView::TileKey bottomRight { 0, 2, 1 };
    QCOMPARE(pyramid.levelTileRect(bottomRight), QRect(1024, 512, 1, 1));
    QCOMPARE(pyramid.levelTileTextureRect(bottomRight), QRect(1023, 511, 2, 2));
}

void TestImageTile::tilesIntersectingVisibleRectAreReturnedInScanOrder()
{
    const KiriView::TilePyramid pyramid(QSize(1536, 1536));
    const std::vector<KiriView::TileKey> tiles
        = pyramid.tilesIntersectingLevelRect(0, QRect(500, 500, 24, 24));

    QCOMPARE(tiles.size(), std::size_t(4));
    QCOMPARE(tiles[0], (KiriView::TileKey { 0, 0, 0 }));
    QCOMPARE(tiles[1], (KiriView::TileKey { 0, 1, 0 }));
    QCOMPARE(tiles[2], (KiriView::TileKey { 0, 0, 1 }));
    QCOMPARE(tiles[3], (KiriView::TileKey { 0, 1, 1 }));
}

void TestImageTile::cacheEvictsLeastRecentlyUsedTilesToBudget()
{
    KiriView::DecodedTileCache cache(32);
    const KiriView::TileKey first { 0, 0, 0 };
    const KiriView::TileKey second { 0, 1, 0 };
    const KiriView::TileKey third { 0, 2, 0 };

    QVERIFY(cache.insert(decodedTile(first, QSize(2, 2))));
    QVERIFY(cache.insert(decodedTile(second, QSize(2, 2))));
    QVERIFY(cache.find(first).has_value());
    QVERIFY(cache.insert(decodedTile(third, QSize(2, 2))));

    QVERIFY(cache.contains(first));
    QVERIFY(!cache.contains(second));
    QVERIFY(cache.contains(third));
    QVERIFY(cache.byteCost() <= cache.byteBudget());
}

void TestImageTile::cacheReportsRejectedTiles()
{
    KiriView::DecodedTileCache cache(1);
    const KiriView::TileKey key { 0, 0, 0 };

    QVERIFY(!cache.insert(decodedTile(key, QSize(1, 1))));

    QVERIFY(!cache.contains(key));
    QCOMPARE(cache.byteCost(), qsizetype(0));
}

QTEST_GUILESS_MAIN(TestImageTile)

#include "test_imagetile.moc"
