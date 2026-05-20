// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/heiftilingplan.h"

#include <QObject>
#include <QPoint>
#include <QRect>
#include <QTest>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace {
heif_image_tiling tiling(
    std::uint32_t columns, std::uint32_t rows, std::uint32_t tileWidth, std::uint32_t tileHeight)
{
    heif_image_tiling result {};
    result.version = 1;
    result.num_columns = columns;
    result.num_rows = rows;
    result.tile_width = tileWidth;
    result.tile_height = tileHeight;
    return result;
}
}

class TestHeifTilingPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gridRejectsInvalidAndSingleTileLayouts();
    void gridAcceptsMultiTileLayouts();
    void regionsCoverIntersectingTilesInPaintOrder();
    void regionsClampToAvailableGrid();
};

void TestHeifTilingPlan::gridRejectsInvalidAndSingleTileLayouts()
{
    QVERIFY(!KiriView::heifTileGridForTiling(tiling(0, 2, 64, 64)).has_value());
    QVERIFY(!KiriView::heifTileGridForTiling(tiling(2, 0, 64, 64)).has_value());
    QVERIFY(!KiriView::heifTileGridForTiling(tiling(2, 2, 0, 64)).has_value());
    QVERIFY(!KiriView::heifTileGridForTiling(tiling(2, 2, 64, 0)).has_value());
    QVERIFY(!KiriView::heifTileGridForTiling(tiling(1, 1, 64, 64)).has_value());
    QVERIFY(!KiriView::heifTileGridForTiling(
        tiling(static_cast<std::uint32_t>(std::numeric_limits<int>::max()) + 1U, 2, 64, 64))
            .has_value());
}

void TestHeifTilingPlan::gridAcceptsMultiTileLayouts()
{
    const std::optional<KiriView::HeifTileGrid> grid
        = KiriView::heifTileGridForTiling(tiling(3, 2, 128, 64));

    QVERIFY(grid.has_value());
    QCOMPARE(grid->columns, 3);
    QCOMPARE(grid->rows, 2);
    QCOMPARE(grid->tileWidth, 128);
    QCOMPARE(grid->tileHeight, 64);
}

void TestHeifTilingPlan::regionsCoverIntersectingTilesInPaintOrder()
{
    const KiriView::HeifTileGrid grid {
        3,
        2,
        100,
        80,
    };

    const std::vector<KiriView::HeifTileDecodeRegion> regions
        = KiriView::heifTileDecodeRegions(grid, QRect(50, 20, 180, 90));

    QCOMPARE(regions.size(), std::size_t(6));
    QCOMPARE(regions.at(0).tileX, 0);
    QCOMPARE(regions.at(0).tileY, 0);
    QCOMPARE(regions.at(0).targetPoint, QPoint(-50, -20));
    QCOMPARE(regions.at(1).tileX, 1);
    QCOMPARE(regions.at(1).tileY, 0);
    QCOMPARE(regions.at(1).targetPoint, QPoint(50, -20));
    QCOMPARE(regions.at(2).tileX, 2);
    QCOMPARE(regions.at(2).tileY, 0);
    QCOMPARE(regions.at(2).targetPoint, QPoint(150, -20));
    QCOMPARE(regions.at(3).tileX, 0);
    QCOMPARE(regions.at(3).tileY, 1);
    QCOMPARE(regions.at(3).targetPoint, QPoint(-50, 60));
    QCOMPARE(regions.at(5).tileX, 2);
    QCOMPARE(regions.at(5).tileY, 1);
    QCOMPARE(regions.at(5).targetPoint, QPoint(150, 60));
}

void TestHeifTilingPlan::regionsClampToAvailableGrid()
{
    const KiriView::HeifTileGrid grid {
        3,
        2,
        100,
        80,
    };

    std::vector<KiriView::HeifTileDecodeRegion> regions
        = KiriView::heifTileDecodeRegions(grid, QRect(250, 70, 200, 40));
    QCOMPARE(regions.size(), std::size_t(2));
    QCOMPARE(regions.at(0).tileX, 2);
    QCOMPARE(regions.at(0).tileY, 0);
    QCOMPARE(regions.at(0).targetPoint, QPoint(-50, -70));
    QCOMPARE(regions.at(1).tileX, 2);
    QCOMPARE(regions.at(1).tileY, 1);
    QCOMPARE(regions.at(1).targetPoint, QPoint(-50, 10));

    regions = KiriView::heifTileDecodeRegions(grid, QRect(400, 0, 50, 50));
    QVERIFY(regions.empty());

    regions = KiriView::heifTileDecodeRegions(grid, QRect());
    QVERIFY(regions.empty());
}

QTEST_GUILESS_MAIN(TestHeifTilingPlan)

#include "test_heiftilingplan.moc"
