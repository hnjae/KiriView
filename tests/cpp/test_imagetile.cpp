// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetile.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <cstddef>

class TestImageTile : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pyramidBuildsDownsampleLevels();
    void levelSelectionUsesSmallestLevelAtLeastAsDenseAsScreenPixels();
    void tileRectsAndApronsAreClipped();
    void tilesIntersectingVisibleRectAreReturnedInScanOrder();
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

QTEST_GUILESS_MAIN(TestImageTile)

#include "test_imagetile.moc"
