// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/imagetilevisibility.h"

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
    void sourceVisibilityContextMapsRotatedViewToSourceSpace();
    void displayScaleUsesPhysicalPixelsPerSourcePixel();
    void firstDisplaySufficiencyUsesCurrentDisplayScale();
    void itemRectMapsToClampedLevelRect();
    void visibleTileKeysSelectDisplayLevelAndPrefetchNeighbors();
    void visibleTileKeysMapRotatedViewToSourceTiles();
};

void TestImageTileVisibility::sourceVisibilityContextMapsRotatedViewToSourceSpace()
{
    const kiriview::TileSourceVisibilityContext unrotatedContext
        = kiriview::tileSourceVisibilityContext(kiriview::TileVisibilityContext {
            QSizeF(1000.0, 500.0),
            QRectF(10.0, 20.0, 100.0, 200.0),
            1.0,
            0,
        });
    QCOMPARE(unrotatedContext.displaySize, QSizeF(1000.0, 500.0));
    QCOMPARE(unrotatedContext.visibleItemRect, QRectF(10.0, 20.0, 100.0, 200.0));

    const kiriview::TileSourceVisibilityContext clockwiseContext
        = kiriview::tileSourceVisibilityContext(kiriview::TileVisibilityContext {
            QSizeF(500.0, 1000.0),
            QRectF(10.0, 20.0, 100.0, 200.0),
            1.0,
            90,
        });
    QCOMPARE(clockwiseContext.displaySize, QSizeF(1000.0, 500.0));
    QCOMPARE(clockwiseContext.visibleItemRect, QRectF(20.0, 390.0, 200.0, 100.0));
}

void TestImageTileVisibility::displayScaleUsesPhysicalPixelsPerSourcePixel()
{
    const kiriview::TilePyramid pyramid(QSize(1000, 500));

    QCOMPARE(kiriview::tileDisplayPixelsPerSourcePixel(pyramid, QSizeF(500.0, 500.0), 2.0), 1.0);
    QCOMPARE(kiriview::tileDisplayPixelsPerSourcePixel(pyramid, QSizeF(), 2.0), 0.0);
    QCOMPARE(kiriview::tileDisplayPixelsPerSourcePixel(pyramid, QSizeF(500.0, 500.0), 0.0), 0.0);
}

void TestImageTileVisibility::firstDisplaySufficiencyUsesCurrentDisplayScale()
{
    const kiriview::TilePyramid pyramid(QSize(2048, 2048));

    QVERIFY(kiriview::tileFirstDisplayIsSufficient(pyramid, QSizeF(512.0, 512.0), 1.0, 0.25));
    QVERIFY(kiriview::tileFirstDisplayIsSufficient(pyramid, QSizeF(514.0, 514.0), 1.0, 0.25));
    QVERIFY(!kiriview::tileFirstDisplayIsSufficient(pyramid, QSizeF(516.0, 516.0), 1.0, 0.25));
    QVERIFY(!kiriview::tileFirstDisplayIsSufficient(pyramid, QSizeF(512.0, 512.0), 1.0, 0.0));
    QVERIFY(!kiriview::tileFirstDisplayIsSufficient(pyramid, QSizeF(512.0, 512.0), 0.0, 0.25));
}

void TestImageTileVisibility::itemRectMapsToClampedLevelRect()
{
    const kiriview::TilePyramid pyramid(QSize(1000, 500));

    QCOMPARE(kiriview::tileLevelRectForItemRect(
                 pyramid, 0, QSizeF(500.0, 250.0), QRectF(-50.0, -10.0, 100.0, 60.0)),
        QRect(0, 0, 100, 100));
    QCOMPARE(kiriview::tileLevelRectForItemRect(
                 pyramid, 0, QSizeF(500.0, 250.0), QRectF(500.0, 0.0, 10.0, 10.0)),
        QRect());
}

void TestImageTileVisibility::visibleTileKeysSelectDisplayLevelAndPrefetchNeighbors()
{
    const kiriview::TilePyramid pyramid(QSize(2048, 1024));
    const std::vector<kiriview::TileKey> tiles = kiriview::visibleTileKeys(pyramid,
        kiriview::TileVisibilityContext {
            QSizeF(2048.0, 1024.0),
            QRectF(512.0, 0.0, 512.0, 512.0),
            1.0,
        });

    QCOMPARE(tiles.size(), std::size_t(6));
    QCOMPARE(tiles[0], (kiriview::TileKey { 0, 1, 0 }));
    QCOMPARE(tiles[1], (kiriview::TileKey { 0, 0, 0 }));
    QCOMPARE(tiles[2], (kiriview::TileKey { 0, 2, 0 }));
    QCOMPARE(tiles[3], (kiriview::TileKey { 0, 0, 1 }));
    QCOMPARE(tiles[4], (kiriview::TileKey { 0, 1, 1 }));
    QCOMPARE(tiles[5], (kiriview::TileKey { 0, 2, 1 }));

    const std::vector<kiriview::TileKey> downsampledTiles = kiriview::visibleTileKeys(pyramid,
        kiriview::TileVisibilityContext {
            QSizeF(512.0, 256.0),
            QRectF(0.0, 0.0, 512.0, 256.0),
            1.0,
        });
    QVERIFY(!downsampledTiles.empty());
    QCOMPARE(downsampledTiles.front().level, 2);
}

void TestImageTileVisibility::visibleTileKeysMapRotatedViewToSourceTiles()
{
    const kiriview::TilePyramid pyramid(QSize(2048, 1024));

    const std::vector<kiriview::TileKey> clockwiseTiles = kiriview::visibleTileKeys(pyramid,
        kiriview::TileVisibilityContext {
            QSizeF(1024.0, 2048.0),
            QRectF(0.0, 512.0, 512.0, 512.0),
            1.0,
            90,
        });
    QVERIFY(!clockwiseTiles.empty());
    QCOMPARE(clockwiseTiles.front(), (kiriview::TileKey { 0, 1, 1 }));

    const std::vector<kiriview::TileKey> upsideDownTiles = kiriview::visibleTileKeys(pyramid,
        kiriview::TileVisibilityContext {
            QSizeF(2048.0, 1024.0),
            QRectF(1024.0, 512.0, 512.0, 512.0),
            1.0,
            180,
        });
    QVERIFY(!upsideDownTiles.empty());
    QCOMPARE(upsideDownTiles.front(), (kiriview::TileKey { 0, 1, 0 }));

    const std::vector<kiriview::TileKey> counterclockwiseTiles = kiriview::visibleTileKeys(pyramid,
        kiriview::TileVisibilityContext {
            QSizeF(1024.0, 2048.0),
            QRectF(512.0, 0.0, 512.0, 512.0),
            1.0,
            270,
        });
    QVERIFY(!counterclockwiseTiles.empty());
    QCOMPARE(counterclockwiseTiles.front(), (kiriview::TileKey { 0, 3, 1 }));
}

QTEST_GUILESS_MAIN(TestImageTileVisibility)

#include "test_imagetilevisibility.moc"
