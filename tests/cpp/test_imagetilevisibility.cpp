// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagetilevisibility.h"

#include <QObject>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QtGlobal>
#include <cstddef>
#include <vector>

class TestImageTileVisibility : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayScaleUsesPhysicalPixelsPerSourcePixel();
    void itemRectMapsToClampedLevelRect();
    void visibleTileKeysSelectDisplayLevelAndPrefetchNeighbors();
};

void TestImageTileVisibility::displayScaleUsesPhysicalPixelsPerSourcePixel()
{
    const KiriView::TilePyramid pyramid(QSize(1000, 500));

    QCOMPARE(KiriView::tileDisplayPixelsPerSourcePixel(pyramid, QSizeF(500.0, 500.0), 2.0), 1.0);
    QCOMPARE(KiriView::tileDisplayPixelsPerSourcePixel(pyramid, QSizeF(), 2.0), 0.0);
    QCOMPARE(KiriView::tileDisplayPixelsPerSourcePixel(pyramid, QSizeF(500.0, 500.0), 0.0), 0.0);
}

void TestImageTileVisibility::itemRectMapsToClampedLevelRect()
{
    const KiriView::TilePyramid pyramid(QSize(1000, 500));

    QCOMPARE(KiriView::tileLevelRectForItemRect(
                 pyramid, 0, QSizeF(500.0, 250.0), QRectF(-50.0, -10.0, 100.0, 60.0)),
        QRect(0, 0, 100, 100));
    QCOMPARE(KiriView::tileLevelRectForItemRect(
                 pyramid, 0, QSizeF(500.0, 250.0), QRectF(500.0, 0.0, 10.0, 10.0)),
        QRect());
}

void TestImageTileVisibility::visibleTileKeysSelectDisplayLevelAndPrefetchNeighbors()
{
    const KiriView::TilePyramid pyramid(QSize(2048, 1024));
    const std::vector<KiriView::TileKey> tiles = KiriView::visibleTileKeys(pyramid,
        KiriView::TileVisibilityContext {
            QSizeF(2048.0, 1024.0),
            QRectF(512.0, 0.0, 512.0, 512.0),
            1.0,
        });

    QCOMPARE(tiles.size(), std::size_t(6));
    QCOMPARE(tiles[0], (KiriView::TileKey { 0, 1, 0 }));
    QCOMPARE(tiles[1], (KiriView::TileKey { 0, 0, 0 }));
    QCOMPARE(tiles[2], (KiriView::TileKey { 0, 2, 0 }));
    QCOMPARE(tiles[3], (KiriView::TileKey { 0, 0, 1 }));
    QCOMPARE(tiles[4], (KiriView::TileKey { 0, 1, 1 }));
    QCOMPARE(tiles[5], (KiriView::TileKey { 0, 2, 1 }));

    const std::vector<KiriView::TileKey> downsampledTiles = KiriView::visibleTileKeys(pyramid,
        KiriView::TileVisibilityContext {
            QSizeF(512.0, 256.0),
            QRectF(0.0, 0.0, 512.0, 256.0),
            1.0,
        });
    QVERIFY(!downsampledTiles.empty());
    QCOMPARE(downsampledTiles.front().level, 2);
}

QTEST_GUILESS_MAIN(TestImageTileVisibility)

#include "test_imagetilevisibility.moc"
