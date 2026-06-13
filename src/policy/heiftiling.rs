// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "kiriview")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustHeifTiling {
        num_columns: u32,
        num_rows: u32,
        tile_width: u32,
        tile_height: u32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustHeifTileGrid {
        columns: i32,
        rows: i32,
        tile_width: i32,
        tile_height: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustHeifTileGridPlan {
        found: bool,
        grid: RustHeifTileGrid,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustHeifTileRect {
        x: i32,
        y: i32,
        width: i32,
        height: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustHeifTileDecodeRegion {
        tile_x: i32,
        tile_y: i32,
        target_x: i32,
        target_y: i32,
    }

    extern "Rust" {
        #[cxx_name = "rustHeifTileGridForTiling"]
        fn rust_heif_tile_grid_for_tiling(tiling: RustHeifTiling) -> RustHeifTileGridPlan;

        #[cxx_name = "rustHeifTileDecodeRegions"]
        fn rust_heif_tile_decode_regions(
            grid: RustHeifTileGrid,
            source_rect: RustHeifTileRect,
        ) -> Vec<RustHeifTileDecodeRegion>;
    }
}

use ffi::{
    RustHeifTileDecodeRegion, RustHeifTileGrid, RustHeifTileGridPlan, RustHeifTileRect,
    RustHeifTiling,
};

const EMPTY_TILE_GRID: RustHeifTileGrid = RustHeifTileGrid {
    columns: 0,
    rows: 0,
    tile_width: 0,
    tile_height: 0,
};

fn rust_heif_tile_grid_for_tiling(tiling: RustHeifTiling) -> RustHeifTileGridPlan {
    heif_tile_grid_for_tiling(tiling).map_or(
        RustHeifTileGridPlan {
            found: false,
            grid: EMPTY_TILE_GRID,
        },
        |grid| RustHeifTileGridPlan { found: true, grid },
    )
}

fn rust_heif_tile_decode_regions(
    grid: RustHeifTileGrid,
    source_rect: RustHeifTileRect,
) -> Vec<RustHeifTileDecodeRegion> {
    heif_tile_decode_regions(grid, source_rect)
}

fn heif_tile_grid_for_tiling(tiling: RustHeifTiling) -> Option<RustHeifTileGrid> {
    let columns = positive_i32(tiling.num_columns)?;
    let rows = positive_i32(tiling.num_rows)?;
    let tile_width = positive_i32(tiling.tile_width)?;
    let tile_height = positive_i32(tiling.tile_height)?;
    if columns == 1 && rows == 1 {
        return None;
    }

    Some(RustHeifTileGrid {
        columns,
        rows,
        tile_width,
        tile_height,
    })
}

fn positive_i32(value: u32) -> Option<i32> {
    if value == 0 || value > i32::MAX as u32 {
        return None;
    }

    Some(value as i32)
}

fn heif_tile_decode_regions(
    grid: RustHeifTileGrid,
    source_rect: RustHeifTileRect,
) -> Vec<RustHeifTileDecodeRegion> {
    if rect_is_empty(source_rect)
        || grid.columns <= 0
        || grid.rows <= 0
        || grid.tile_width <= 0
        || grid.tile_height <= 0
    {
        return Vec::new();
    }

    let first_tile_x = 0.max(source_rect.x / grid.tile_width);
    let first_tile_y = 0.max(source_rect.y / grid.tile_height);
    let last_tile_x = (grid.columns - 1).min(rect_right(source_rect) / grid.tile_width);
    let last_tile_y = (grid.rows - 1).min(rect_bottom(source_rect) / grid.tile_height);
    if first_tile_x > last_tile_x || first_tile_y > last_tile_y {
        return Vec::new();
    }

    let mut regions = Vec::with_capacity(region_capacity(
        first_tile_x,
        last_tile_x,
        first_tile_y,
        last_tile_y,
    ));
    for tile_y in first_tile_y..=last_tile_y {
        for tile_x in first_tile_x..=last_tile_x {
            regions.push(RustHeifTileDecodeRegion {
                tile_x,
                tile_y,
                target_x: clamped_i32(
                    i64::from(tile_x) * i64::from(grid.tile_width) - i64::from(source_rect.x),
                ),
                target_y: clamped_i32(
                    i64::from(tile_y) * i64::from(grid.tile_height) - i64::from(source_rect.y),
                ),
            });
        }
    }

    regions
}

fn rect_is_empty(rect: RustHeifTileRect) -> bool {
    rect.width <= 0 || rect.height <= 0
}

fn rect_right(rect: RustHeifTileRect) -> i32 {
    clamped_i32(i64::from(rect.x) + i64::from(rect.width) - 1)
}

fn rect_bottom(rect: RustHeifTileRect) -> i32 {
    clamped_i32(i64::from(rect.y) + i64::from(rect.height) - 1)
}

fn clamped_i32(value: i64) -> i32 {
    value.clamp(i64::from(i32::MIN), i64::from(i32::MAX)) as i32
}

fn region_capacity(
    first_tile_x: i32,
    last_tile_x: i32,
    first_tile_y: i32,
    last_tile_y: i32,
) -> usize {
    let columns = i64::from(last_tile_x - first_tile_x + 1);
    let rows = i64::from(last_tile_y - first_tile_y + 1);
    usize::try_from(columns.saturating_mul(rows)).unwrap_or(0)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn tiling(
        num_columns: u32,
        num_rows: u32,
        tile_width: u32,
        tile_height: u32,
    ) -> RustHeifTiling {
        RustHeifTiling {
            num_columns,
            num_rows,
            tile_width,
            tile_height,
        }
    }

    fn rect(x: i32, y: i32, width: i32, height: i32) -> RustHeifTileRect {
        RustHeifTileRect {
            x,
            y,
            width,
            height,
        }
    }

    #[test]
    fn grid_rejects_invalid_and_single_tile_layouts() {
        assert!(!rust_heif_tile_grid_for_tiling(tiling(0, 2, 64, 64)).found);
        assert!(!rust_heif_tile_grid_for_tiling(tiling(2, 0, 64, 64)).found);
        assert!(!rust_heif_tile_grid_for_tiling(tiling(2, 2, 0, 64)).found);
        assert!(!rust_heif_tile_grid_for_tiling(tiling(2, 2, 64, 0)).found);
        assert!(!rust_heif_tile_grid_for_tiling(tiling(1, 1, 64, 64)).found);
        assert!(!rust_heif_tile_grid_for_tiling(tiling(i32::MAX as u32 + 1, 2, 64, 64)).found);
    }

    #[test]
    fn grid_accepts_multi_tile_layouts() {
        assert_eq!(
            rust_heif_tile_grid_for_tiling(tiling(3, 2, 128, 64)),
            RustHeifTileGridPlan {
                found: true,
                grid: RustHeifTileGrid {
                    columns: 3,
                    rows: 2,
                    tile_width: 128,
                    tile_height: 64,
                },
            }
        );
    }

    #[test]
    fn regions_cover_intersecting_tiles_in_paint_order() {
        let regions = heif_tile_decode_regions(
            RustHeifTileGrid {
                columns: 3,
                rows: 2,
                tile_width: 100,
                tile_height: 80,
            },
            rect(50, 20, 180, 90),
        );

        assert_eq!(
            regions,
            vec![
                region(0, 0, -50, -20),
                region(1, 0, 50, -20),
                region(2, 0, 150, -20),
                region(0, 1, -50, 60),
                region(1, 1, 50, 60),
                region(2, 1, 150, 60),
            ]
        );
    }

    #[test]
    fn regions_clamp_to_available_grid() {
        let grid = RustHeifTileGrid {
            columns: 3,
            rows: 2,
            tile_width: 100,
            tile_height: 80,
        };

        assert_eq!(
            heif_tile_decode_regions(grid, rect(250, 70, 200, 40)),
            vec![region(2, 0, -50, -70), region(2, 1, -50, 10)]
        );
        assert!(heif_tile_decode_regions(grid, rect(400, 0, 50, 50)).is_empty());
        assert!(heif_tile_decode_regions(grid, rect(0, 0, 0, 0)).is_empty());
    }

    fn region(tile_x: i32, tile_y: i32, target_x: i32, target_y: i32) -> RustHeifTileDecodeRegion {
        RustHeifTileDecodeRegion {
            tile_x,
            tile_y,
            target_x,
            target_y,
        }
    }
}
