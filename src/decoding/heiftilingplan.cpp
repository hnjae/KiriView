// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "heiftilingplan.h"

#include "bridge/qtgeometryconversion.h"
#include "kiriview/src/policy/heiftiling.cxx.h"

namespace {
kiriview::RustHeifTiling rustHeifTiling(const heif_image_tiling& tiling)
{
    return kiriview::RustHeifTiling {
        tiling.num_columns,
        tiling.num_rows,
        tiling.tile_width,
        tiling.tile_height,
    };
}

kiriview::HeifTileGrid heifTileGridFromRust(kiriview::RustHeifTileGrid grid)
{
    return kiriview::HeifTileGrid {
        grid.columns,
        grid.rows,
        grid.tile_width,
        grid.tile_height,
    };
}

kiriview::HeifTileDecodeRegion heifTileDecodeRegionFromRust(
    kiriview::RustHeifTileDecodeRegion region)
{
    return kiriview::HeifTileDecodeRegion {
        region.tile_x,
        region.tile_y,
        QPoint(region.target_x, region.target_y),
    };
}

kiriview::RustHeifTileGrid rustHeifTileGrid(kiriview::HeifTileGrid grid)
{
    return kiriview::RustHeifTileGrid {
        grid.columns,
        grid.rows,
        grid.tileWidth,
        grid.tileHeight,
    };
}

kiriview::RustHeifTileRect rustHeifTileRect(QRect rect)
{
    return kiriview::Bridge::rustRect<kiriview::RustHeifTileRect>(rect);
}

}

namespace kiriview {
std::optional<HeifTileGrid> heifTileGridForTiling(const heif_image_tiling& tiling)
{
    const RustHeifTileGridPlan plan = rustHeifTileGridForTiling(rustHeifTiling(tiling));
    if (!plan.found) {
        return std::nullopt;
    }

    return heifTileGridFromRust(plan.grid);
}

std::vector<HeifTileDecodeRegion> heifTileDecodeRegions(HeifTileGrid grid, QRect sourceRect)
{
    std::vector<HeifTileDecodeRegion> regions;
    const rust::Vec<RustHeifTileDecodeRegion> rustRegions
        = rustHeifTileDecodeRegions(rustHeifTileGrid(grid), rustHeifTileRect(sourceRect));
    regions.reserve(rustRegions.size());
    for (const RustHeifTileDecodeRegion& region : rustRegions) {
        regions.push_back(heifTileDecodeRegionFromRust(region));
    }
    return regions;
}
}
