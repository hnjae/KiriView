// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heiftilingplan.h"

#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/heiftiling.cxx.h"

namespace {
KiriView::RustHeifTiling rustHeifTiling(const heif_image_tiling &tiling)
{
    return KiriView::RustHeifTiling {
        tiling.num_columns,
        tiling.num_rows,
        tiling.tile_width,
        tiling.tile_height,
    };
}

KiriView::HeifTileGrid heifTileGridFromRust(const KiriView::RustHeifTileGrid &grid)
{
    return KiriView::HeifTileGrid {
        grid.columns,
        grid.rows,
        grid.tile_width,
        grid.tile_height,
    };
}

KiriView::HeifTileDecodeRegion heifTileDecodeRegionFromRust(
    const KiriView::RustHeifTileDecodeRegion &region)
{
    return KiriView::HeifTileDecodeRegion {
        region.tile_x,
        region.tile_y,
        QPoint(region.target_x, region.target_y),
    };
}

KiriView::RustHeifTileGrid rustHeifTileGrid(const KiriView::HeifTileGrid &grid)
{
    return KiriView::RustHeifTileGrid {
        grid.columns,
        grid.rows,
        grid.tileWidth,
        grid.tileHeight,
    };
}

KiriView::RustHeifTileRect rustHeifTileRect(const QRect &rect)
{
    return KiriView::Bridge::rustRect<KiriView::RustHeifTileRect>(rect);
}

}

namespace KiriView {
std::optional<HeifTileGrid> heifTileGridForTiling(const heif_image_tiling &tiling)
{
    const RustHeifTileGridPlan plan = rustHeifTileGridForTiling(rustHeifTiling(tiling));
    if (!plan.found) {
        return std::nullopt;
    }

    return heifTileGridFromRust(plan.grid);
}

std::vector<HeifTileDecodeRegion> heifTileDecodeRegions(
    const HeifTileGrid &grid, const QRect &sourceRect)
{
    std::vector<HeifTileDecodeRegion> regions;
    const rust::Vec<RustHeifTileDecodeRegion> rustRegions
        = rustHeifTileDecodeRegions(rustHeifTileGrid(grid), rustHeifTileRect(sourceRect));
    regions.reserve(rustRegions.size());
    for (const RustHeifTileDecodeRegion &region : rustRegions) {
        regions.push_back(heifTileDecodeRegionFromRust(region));
    }
    return regions;
}
}
