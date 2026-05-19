// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const FIRST_DISPLAY_PIXELS_PER_SOURCE_PIXEL_EPSILON: f64 = 0.001;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustTileSize {
        width: i32,
        height: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustTileSizeF {
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustTileRect {
        x: i32,
        y: i32,
        width: i32,
        height: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustTileRectF {
        x: f64,
        y: f64,
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustTileKey {
        level: i32,
        x: i32,
        y: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustTileLevel {
        index: i32,
        size: RustTileSize,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustTileRequest {
        key: RustTileKey,
        level_size: RustTileSize,
        level_rect: RustTileRect,
        texture_level_rect: RustTileRect,
        source_rect: RustTileRect,
    }

    extern "Rust" {
        #[cxx_name = "rustBoundedIntegerRect"]
        fn rust_bounded_integer_rect(rect: RustTileRect, bounds_size: RustTileSize)
        -> RustTileRect;

        #[cxx_name = "rustScaledIntegerRect"]
        fn rust_scaled_integer_rect(
            rect: RustTileRectF,
            source_size: RustTileSizeF,
            target_size: RustTileSize,
        ) -> RustTileRect;

        #[cxx_name = "rustTilePyramidLevels"]
        fn rust_tile_pyramid_levels(image_size: RustTileSize) -> Vec<RustTileLevel>;

        #[cxx_name = "rustTilePyramidTileGridSize"]
        fn rust_tile_pyramid_tile_grid_size(
            image_size: RustTileSize,
            tile_size: i32,
            level: i32,
        ) -> RustTileSize;

        #[cxx_name = "rustTilePyramidContainsLevel"]
        fn rust_tile_pyramid_contains_level(level_count: i32, level: i32) -> bool;

        #[cxx_name = "rustTilePyramidContainsTile"]
        fn rust_tile_pyramid_contains_tile(
            image_size: RustTileSize,
            tile_size: i32,
            key: RustTileKey,
        ) -> bool;

        #[cxx_name = "rustTilePyramidSelectLevelForDisplayScale"]
        fn rust_tile_pyramid_select_level_for_display_scale(
            image_size: RustTileSize,
            display_pixels_per_source_pixel: f64,
        ) -> i32;

        #[cxx_name = "rustTilePyramidLevelTileRect"]
        fn rust_tile_pyramid_level_tile_rect(
            image_size: RustTileSize,
            tile_size: i32,
            key: RustTileKey,
        ) -> RustTileRect;

        #[cxx_name = "rustTilePyramidLevelTileTextureRect"]
        fn rust_tile_pyramid_level_tile_texture_rect(
            image_size: RustTileSize,
            tile_size: i32,
            apron_source_pixels: i32,
            key: RustTileKey,
        ) -> RustTileRect;

        #[cxx_name = "rustTilePyramidSourceRectForLevelRect"]
        fn rust_tile_pyramid_source_rect_for_level_rect(
            image_size: RustTileSize,
            level: i32,
            level_rect: RustTileRect,
        ) -> RustTileRect;

        #[cxx_name = "rustTilePyramidRequestForTile"]
        fn rust_tile_pyramid_request_for_tile(
            image_size: RustTileSize,
            tile_size: i32,
            apron_source_pixels: i32,
            key: RustTileKey,
        ) -> RustTileRequest;

        #[cxx_name = "rustTilePyramidTilesIntersectingLevelRect"]
        fn rust_tile_pyramid_tiles_intersecting_level_rect(
            image_size: RustTileSize,
            tile_size: i32,
            level: i32,
            level_rect: RustTileRect,
        ) -> Vec<RustTileKey>;

        #[cxx_name = "rustTilePyramidLevelPixelsPerSourcePixel"]
        fn rust_tile_pyramid_level_pixels_per_source_pixel(
            image_size: RustTileSize,
            level: i32,
        ) -> f64;

        #[cxx_name = "rustTileDisplayPixelsPerSourcePixel"]
        fn rust_tile_display_pixels_per_source_pixel(
            image_size: RustTileSize,
            display_size: RustTileSizeF,
            device_pixel_ratio: f64,
        ) -> f64;

        #[cxx_name = "rustTileFirstDisplayIsSufficient"]
        fn rust_tile_first_display_is_sufficient(
            image_size: RustTileSize,
            display_size: RustTileSizeF,
            device_pixel_ratio: f64,
            first_display_pixels_per_source_pixel: f64,
        ) -> bool;

        #[cxx_name = "rustTileLevelRectForItemRect"]
        fn rust_tile_level_rect_for_item_rect(
            image_size: RustTileSize,
            level: i32,
            display_size: RustTileSizeF,
            item_rect: RustTileRectF,
        ) -> RustTileRect;

        #[cxx_name = "rustVisibleTileKeys"]
        fn rust_visible_tile_keys(
            image_size: RustTileSize,
            tile_size: i32,
            display_size: RustTileSizeF,
            visible_item_rect: RustTileRectF,
            device_pixel_ratio: f64,
        ) -> Vec<RustTileKey>;
    }
}

use ffi::{
    RustTileKey, RustTileLevel, RustTileRect, RustTileRectF, RustTileRequest, RustTileSize,
    RustTileSizeF,
};

fn rust_bounded_integer_rect(rect: RustTileRect, bounds_size: RustTileSize) -> RustTileRect {
    bounded_integer_rect(rect, bounds_size)
}

fn rust_scaled_integer_rect(
    rect: RustTileRectF,
    source_size: RustTileSizeF,
    target_size: RustTileSize,
) -> RustTileRect {
    scaled_integer_rect(rect, source_size, target_size)
}

fn rust_tile_pyramid_levels(image_size: RustTileSize) -> Vec<RustTileLevel> {
    tile_pyramid_levels(image_size)
}

fn rust_tile_pyramid_tile_grid_size(
    image_size: RustTileSize,
    tile_size: i32,
    level: i32,
) -> RustTileSize {
    tile_grid_size(image_size, tile_size, level)
}

fn rust_tile_pyramid_contains_level(level_count: i32, level: i32) -> bool {
    level >= 0 && level < level_count
}

fn rust_tile_pyramid_contains_tile(
    image_size: RustTileSize,
    tile_size: i32,
    key: RustTileKey,
) -> bool {
    contains_tile(image_size, tile_size, key)
}

fn rust_tile_pyramid_select_level_for_display_scale(
    image_size: RustTileSize,
    display_pixels_per_source_pixel: f64,
) -> i32 {
    select_level_for_display_scale(image_size, display_pixels_per_source_pixel)
}

fn rust_tile_pyramid_level_tile_rect(
    image_size: RustTileSize,
    tile_size: i32,
    key: RustTileKey,
) -> RustTileRect {
    level_tile_rect(image_size, tile_size, key)
}

fn rust_tile_pyramid_level_tile_texture_rect(
    image_size: RustTileSize,
    tile_size: i32,
    apron_source_pixels: i32,
    key: RustTileKey,
) -> RustTileRect {
    level_tile_texture_rect(image_size, tile_size, apron_source_pixels, key)
}

fn rust_tile_pyramid_source_rect_for_level_rect(
    image_size: RustTileSize,
    level: i32,
    level_rect: RustTileRect,
) -> RustTileRect {
    source_rect_for_level_rect(image_size, level, level_rect)
}

fn rust_tile_pyramid_request_for_tile(
    image_size: RustTileSize,
    tile_size: i32,
    apron_source_pixels: i32,
    key: RustTileKey,
) -> RustTileRequest {
    request_for_tile(image_size, tile_size, apron_source_pixels, key)
}

fn rust_tile_pyramid_tiles_intersecting_level_rect(
    image_size: RustTileSize,
    tile_size: i32,
    level: i32,
    level_rect: RustTileRect,
) -> Vec<RustTileKey> {
    tiles_intersecting_level_rect(image_size, tile_size, level, level_rect)
}

fn rust_tile_pyramid_level_pixels_per_source_pixel(image_size: RustTileSize, level: i32) -> f64 {
    level_pixels_per_source_pixel(image_size, level)
}

fn rust_tile_display_pixels_per_source_pixel(
    image_size: RustTileSize,
    display_size: RustTileSizeF,
    device_pixel_ratio: f64,
) -> f64 {
    tile_display_pixels_per_source_pixel(image_size, display_size, device_pixel_ratio)
}

fn rust_tile_first_display_is_sufficient(
    image_size: RustTileSize,
    display_size: RustTileSizeF,
    device_pixel_ratio: f64,
    first_display_pixels_per_source_pixel: f64,
) -> bool {
    tile_first_display_is_sufficient(
        image_size,
        display_size,
        device_pixel_ratio,
        first_display_pixels_per_source_pixel,
    )
}

fn rust_tile_level_rect_for_item_rect(
    image_size: RustTileSize,
    level: i32,
    display_size: RustTileSizeF,
    item_rect: RustTileRectF,
) -> RustTileRect {
    tile_level_rect_for_item_rect(image_size, level, display_size, item_rect)
}

fn rust_visible_tile_keys(
    image_size: RustTileSize,
    tile_size: i32,
    display_size: RustTileSizeF,
    visible_item_rect: RustTileRectF,
    device_pixel_ratio: f64,
) -> Vec<RustTileKey> {
    visible_tile_keys(
        image_size,
        tile_size,
        display_size,
        visible_item_rect,
        device_pixel_ratio,
    )
}

fn bounded_integer_rect(rect: RustTileRect, bounds_size: RustTileSize) -> RustTileRect {
    if rect_is_empty(rect) || size_is_empty(bounds_size) {
        return empty_rect();
    }

    let left = i64::from(rect.x).max(0);
    let top = i64::from(rect.y).max(0);
    let right = i64::from(rect.x)
        .saturating_add(i64::from(rect.width))
        .min(i64::from(bounds_size.width));
    let bottom = i64::from(rect.y)
        .saturating_add(i64::from(rect.height))
        .min(i64::from(bounds_size.height));
    rect_from_edges(left, top, right, bottom)
}

fn scaled_integer_rect(
    rect: RustTileRectF,
    source_size: RustTileSizeF,
    target_size: RustTileSize,
) -> RustTileRect {
    if rectf_is_empty(rect)
        || !rectf_is_finite(rect)
        || sizef_is_empty(source_size)
        || !sizef_is_finite(source_size)
        || size_is_empty(target_size)
    {
        return empty_rect();
    }

    let bounded = intersect_rectf(
        rect,
        RustTileRectF {
            x: 0.0,
            y: 0.0,
            width: source_size.width,
            height: source_size.height,
        },
    );
    if rectf_is_empty(bounded) {
        return empty_rect();
    }

    let x_scale = f64::from(target_size.width) / source_size.width;
    let y_scale = f64::from(target_size.height) / source_size.height;
    if !x_scale.is_finite() || !y_scale.is_finite() || x_scale <= 0.0 || y_scale <= 0.0 {
        return empty_rect();
    }

    let left = clamp_floor_to_i32(bounded.x * x_scale, 0, target_size.width);
    let top = clamp_floor_to_i32(bounded.y * y_scale, 0, target_size.height);
    let right = clamp_ceil_to_i32(rectf_right(bounded) * x_scale, left, target_size.width);
    let bottom = clamp_ceil_to_i32(rectf_bottom(bounded) * y_scale, top, target_size.height);
    rect_from_i32_edges(left, top, right, bottom)
}

fn tile_pyramid_levels(image_size: RustTileSize) -> Vec<RustTileLevel> {
    let mut levels = Vec::new();
    if size_is_empty(image_size) {
        return levels;
    }

    let mut size = image_size;
    let mut level = 0;
    loop {
        levels.push(RustTileLevel { index: level, size });
        if size.width == 1 && size.height == 1 {
            break;
        }
        size = half_size(size);
        level += 1;
    }
    levels
}

fn tile_grid_size(image_size: RustTileSize, tile_size: i32, level: i32) -> RustTileSize {
    let size = level_size(image_size, level);
    if size_is_empty(size) {
        return empty_size();
    }

    let tile_size = normalized_tile_size(tile_size);
    RustTileSize {
        width: div_ceil_i32(size.width, tile_size),
        height: div_ceil_i32(size.height, tile_size),
    }
}

fn contains_tile(image_size: RustTileSize, tile_size: i32, key: RustTileKey) -> bool {
    let grid = tile_grid_size(image_size, tile_size, key.level);
    !size_is_empty(grid) && key.x >= 0 && key.y >= 0 && key.x < grid.width && key.y < grid.height
}

fn select_level_for_display_scale(
    image_size: RustTileSize,
    display_pixels_per_source_pixel: f64,
) -> i32 {
    let levels = tile_pyramid_levels(image_size);
    if levels.is_empty() {
        return 0;
    }
    if !display_pixels_per_source_pixel.is_finite() || display_pixels_per_source_pixel <= 0.0 {
        return i32::try_from(levels.len() - 1).unwrap_or(i32::MAX);
    }

    let mut selected_level = 0;
    for level in 0..levels.len() {
        let level = i32::try_from(level).unwrap_or(i32::MAX);
        if level_pixels_per_source_pixel(image_size, level) + f64::EPSILON
            >= display_pixels_per_source_pixel
        {
            selected_level = level;
        }
    }
    selected_level
}

fn level_tile_rect(image_size: RustTileSize, tile_size: i32, key: RustTileKey) -> RustTileRect {
    if !contains_tile(image_size, tile_size, key) {
        return empty_rect();
    }

    let tile_size = normalized_tile_size(tile_size);
    let tile_rect = RustTileRect {
        x: key.x.saturating_mul(tile_size),
        y: key.y.saturating_mul(tile_size),
        width: tile_size,
        height: tile_size,
    };
    bounded_integer_rect(tile_rect, level_size(image_size, key.level))
}

fn level_tile_texture_rect(
    image_size: RustTileSize,
    tile_size: i32,
    apron_source_pixels: i32,
    key: RustTileKey,
) -> RustTileRect {
    let tile_rect = level_tile_rect(image_size, tile_size, key);
    if rect_is_empty(tile_rect) {
        return empty_rect();
    }

    let tile_size = normalized_tile_size(tile_size);
    let apron_source_pixels = apron_source_pixels.max(0);
    let apron = tile_size.min(
        (f64::from(apron_source_pixels) * level_pixels_per_source_pixel(image_size, key.level))
            .ceil()
            .max(1.0) as i32,
    );
    bounded_integer_rect(
        adjusted_rect(tile_rect, apron),
        level_size(image_size, key.level),
    )
}

fn source_rect_for_level_rect(
    image_size: RustTileSize,
    level: i32,
    level_rect: RustTileRect,
) -> RustTileRect {
    let level_size = level_size(image_size, level);
    if size_is_empty(image_size) || size_is_empty(level_size) || rect_is_empty(level_rect) {
        return empty_rect();
    }

    scaled_integer_rect(
        rect_to_rectf(level_rect),
        RustTileSizeF {
            width: f64::from(level_size.width),
            height: f64::from(level_size.height),
        },
        image_size,
    )
}

fn request_for_tile(
    image_size: RustTileSize,
    tile_size: i32,
    apron_source_pixels: i32,
    key: RustTileKey,
) -> RustTileRequest {
    let level_rect = level_tile_rect(image_size, tile_size, key);
    let texture_level_rect =
        level_tile_texture_rect(image_size, tile_size, apron_source_pixels, key);
    RustTileRequest {
        key,
        level_size: level_size(image_size, key.level),
        level_rect,
        texture_level_rect,
        source_rect: source_rect_for_level_rect(image_size, key.level, texture_level_rect),
    }
}

fn tiles_intersecting_level_rect(
    image_size: RustTileSize,
    tile_size: i32,
    level: i32,
    level_rect: RustTileRect,
) -> Vec<RustTileKey> {
    let mut keys = Vec::new();
    let tile_size = normalized_tile_size(tile_size);
    let grid = tile_grid_size(image_size, tile_size, level);
    let size = level_size(image_size, level);
    if size_is_empty(grid) || size_is_empty(size) || rect_is_empty(level_rect) {
        return keys;
    }

    let bounded = bounded_integer_rect(level_rect, size);
    if rect_is_empty(bounded) {
        return keys;
    }

    let left = bounded.x / tile_size;
    let top = bounded.y / tile_size;
    let right = (bounded.x + bounded.width - 1) / tile_size;
    let bottom = (bounded.y + bounded.height - 1) / tile_size;
    let capacity = i64::from(right - left + 1) * i64::from(bottom - top + 1);
    if let Ok(capacity) = usize::try_from(capacity) {
        keys.reserve(capacity);
    }

    for y in top..=bottom {
        for x in left..=right {
            keys.push(RustTileKey { level, x, y });
        }
    }
    keys
}

fn level_pixels_per_source_pixel(image_size: RustTileSize, level: i32) -> f64 {
    let size = level_size(image_size, level);
    if size_is_empty(image_size) || size_is_empty(size) {
        return 0.0;
    }

    (f64::from(size.width) / f64::from(image_size.width))
        .min(f64::from(size.height) / f64::from(image_size.height))
}

fn tile_display_pixels_per_source_pixel(
    image_size: RustTileSize,
    display_size: RustTileSizeF,
    device_pixel_ratio: f64,
) -> f64 {
    if size_is_empty(image_size)
        || sizef_is_empty(display_size)
        || !sizef_is_finite(display_size)
        || !device_pixel_ratio.is_finite()
        || device_pixel_ratio <= 0.0
    {
        return 0.0;
    }

    ((display_size.width * device_pixel_ratio) / f64::from(image_size.width))
        .min((display_size.height * device_pixel_ratio) / f64::from(image_size.height))
}

fn tile_first_display_is_sufficient(
    image_size: RustTileSize,
    display_size: RustTileSizeF,
    device_pixel_ratio: f64,
    first_display_pixels_per_source_pixel: f64,
) -> bool {
    if !first_display_pixels_per_source_pixel.is_finite()
        || first_display_pixels_per_source_pixel <= 0.0
    {
        return false;
    }

    let current_display_pixels_per_source_pixel =
        tile_display_pixels_per_source_pixel(image_size, display_size, device_pixel_ratio);
    if !current_display_pixels_per_source_pixel.is_finite()
        || current_display_pixels_per_source_pixel <= 0.0
    {
        return false;
    }

    current_display_pixels_per_source_pixel
        <= first_display_pixels_per_source_pixel + FIRST_DISPLAY_PIXELS_PER_SOURCE_PIXEL_EPSILON
}

fn tile_level_rect_for_item_rect(
    image_size: RustTileSize,
    level: i32,
    display_size: RustTileSizeF,
    item_rect: RustTileRectF,
) -> RustTileRect {
    if rectf_is_empty(item_rect) || sizef_is_empty(display_size) || size_is_empty(image_size) {
        return empty_rect();
    }

    let level_size = level_size(image_size, level);
    if size_is_empty(level_size) {
        return empty_rect();
    }

    scaled_integer_rect(item_rect, display_size, level_size)
}

fn visible_tile_keys(
    image_size: RustTileSize,
    tile_size: i32,
    display_size: RustTileSizeF,
    visible_item_rect: RustTileRectF,
    device_pixel_ratio: f64,
) -> Vec<RustTileKey> {
    if size_is_empty(image_size)
        || sizef_is_empty(display_size)
        || rectf_is_empty(visible_item_rect)
    {
        return Vec::new();
    }

    let level = select_level_for_display_scale(
        image_size,
        tile_display_pixels_per_source_pixel(image_size, display_size, device_pixel_ratio),
    );
    let current_level_rect =
        tile_level_rect_for_item_rect(image_size, level, display_size, visible_item_rect);
    let mut keys = tiles_intersecting_level_rect(image_size, tile_size, level, current_level_rect);

    let prefetch_item_rect = intersect_rectf(
        adjusted_rectf(
            visible_item_rect,
            -visible_item_rect.width,
            -visible_item_rect.height,
            visible_item_rect.width,
            visible_item_rect.height,
        ),
        RustTileRectF {
            x: 0.0,
            y: 0.0,
            width: display_size.width,
            height: display_size.height,
        },
    );
    let prefetch_level_rect =
        tile_level_rect_for_item_rect(image_size, level, display_size, prefetch_item_rect);
    for key in tiles_intersecting_level_rect(image_size, tile_size, level, prefetch_level_rect) {
        if !keys.iter().any(|existing| tile_keys_equal(*existing, key)) {
            keys.push(key);
        }
    }

    keys
}

fn level_size(image_size: RustTileSize, level: i32) -> RustTileSize {
    if level < 0 {
        return empty_size();
    }

    tile_pyramid_levels(image_size)
        .into_iter()
        .nth(usize::try_from(level).unwrap_or(usize::MAX))
        .map_or_else(empty_size, |level| level.size)
}

fn half_size(size: RustTileSize) -> RustTileSize {
    RustTileSize {
        width: ((i64::from(size.width) + 1) / 2).max(1) as i32,
        height: ((i64::from(size.height) + 1) / 2).max(1) as i32,
    }
}

fn normalized_tile_size(tile_size: i32) -> i32 {
    tile_size.max(1)
}

fn div_ceil_i32(value: i32, divisor: i32) -> i32 {
    ((i64::from(value) + i64::from(divisor) - 1) / i64::from(divisor)) as i32
}

fn intersect_rectf(left: RustTileRectF, right: RustTileRectF) -> RustTileRectF {
    let x1 = left.x.max(right.x);
    let y1 = left.y.max(right.y);
    let x2 = rectf_right(left).min(rectf_right(right));
    let y2 = rectf_bottom(left).min(rectf_bottom(right));
    if x2 <= x1 || y2 <= y1 {
        return empty_rectf();
    }

    RustTileRectF {
        x: x1,
        y: y1,
        width: x2 - x1,
        height: y2 - y1,
    }
}

fn adjusted_rect(rect: RustTileRect, apron: i32) -> RustTileRect {
    RustTileRect {
        x: rect.x.saturating_sub(apron),
        y: rect.y.saturating_sub(apron),
        width: rect.width.saturating_add(apron.saturating_mul(2)),
        height: rect.height.saturating_add(apron.saturating_mul(2)),
    }
}

fn adjusted_rectf(rect: RustTileRectF, dx1: f64, dy1: f64, dx2: f64, dy2: f64) -> RustTileRectF {
    RustTileRectF {
        x: rect.x + dx1,
        y: rect.y + dy1,
        width: rect.width + dx2 - dx1,
        height: rect.height + dy2 - dy1,
    }
}

fn rect_to_rectf(rect: RustTileRect) -> RustTileRectF {
    RustTileRectF {
        x: f64::from(rect.x),
        y: f64::from(rect.y),
        width: f64::from(rect.width),
        height: f64::from(rect.height),
    }
}

fn rect_from_edges(left: i64, top: i64, right: i64, bottom: i64) -> RustTileRect {
    if right <= left || bottom <= top {
        return empty_rect();
    }

    RustTileRect {
        x: i64_to_i32_saturating(left),
        y: i64_to_i32_saturating(top),
        width: i64_to_i32_saturating(right - left),
        height: i64_to_i32_saturating(bottom - top),
    }
}

fn rect_from_i32_edges(left: i32, top: i32, right: i32, bottom: i32) -> RustTileRect {
    rect_from_edges(
        i64::from(left),
        i64::from(top),
        i64::from(right),
        i64::from(bottom),
    )
}

fn clamp_floor_to_i32(value: f64, minimum: i32, maximum: i32) -> i32 {
    clamp_rounded_to_i32(value.floor(), minimum, maximum)
}

fn clamp_ceil_to_i32(value: f64, minimum: i32, maximum: i32) -> i32 {
    clamp_rounded_to_i32(value.ceil(), minimum, maximum)
}

fn clamp_rounded_to_i32(value: f64, minimum: i32, maximum: i32) -> i32 {
    if !value.is_finite() {
        return minimum;
    }
    value.clamp(f64::from(minimum), f64::from(maximum)) as i32
}

fn i64_to_i32_saturating(value: i64) -> i32 {
    i32::try_from(value).unwrap_or(if value < 0 { i32::MIN } else { i32::MAX })
}

fn tile_keys_equal(left: RustTileKey, right: RustTileKey) -> bool {
    left.level == right.level && left.x == right.x && left.y == right.y
}

fn rectf_right(rect: RustTileRectF) -> f64 {
    rect.x + rect.width
}

fn rectf_bottom(rect: RustTileRectF) -> f64 {
    rect.y + rect.height
}

fn size_is_empty(size: RustTileSize) -> bool {
    size.width <= 0 || size.height <= 0
}

fn sizef_is_empty(size: RustTileSizeF) -> bool {
    size.width <= 0.0 || size.height <= 0.0
}

fn sizef_is_finite(size: RustTileSizeF) -> bool {
    size.width.is_finite() && size.height.is_finite()
}

fn rect_is_empty(rect: RustTileRect) -> bool {
    rect.width <= 0 || rect.height <= 0
}

fn rectf_is_empty(rect: RustTileRectF) -> bool {
    rect.width <= 0.0 || rect.height <= 0.0
}

fn rectf_is_finite(rect: RustTileRectF) -> bool {
    rect.x.is_finite() && rect.y.is_finite() && rect.width.is_finite() && rect.height.is_finite()
}

fn empty_size() -> RustTileSize {
    RustTileSize {
        width: 0,
        height: 0,
    }
}

fn empty_rect() -> RustTileRect {
    RustTileRect {
        x: 0,
        y: 0,
        width: 0,
        height: 0,
    }
}

fn empty_rectf() -> RustTileRectF {
    RustTileRectF {
        x: 0.0,
        y: 0.0,
        width: 0.0,
        height: 0.0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn size(width: i32, height: i32) -> RustTileSize {
        RustTileSize { width, height }
    }

    fn sizef(width: f64, height: f64) -> RustTileSizeF {
        RustTileSizeF { width, height }
    }

    fn rect(x: i32, y: i32, width: i32, height: i32) -> RustTileRect {
        RustTileRect {
            x,
            y,
            width,
            height,
        }
    }

    fn rectf(x: f64, y: f64, width: f64, height: f64) -> RustTileRectF {
        RustTileRectF {
            x,
            y,
            width,
            height,
        }
    }

    fn key(level: i32, x: i32, y: i32) -> RustTileKey {
        RustTileKey { level, x, y }
    }

    #[test]
    fn maps_and_clips_integer_rects() {
        assert_eq!(
            bounded_integer_rect(rect(-1, -2, 4, 5), size(10, 10)),
            rect(0, 0, 3, 3)
        );
        assert_eq!(
            scaled_integer_rect(rectf(1.0, 2.0, 3.0, 4.0), sizef(10.0, 10.0), size(100, 50)),
            rect(10, 10, 30, 20)
        );
        assert_eq!(
            scaled_integer_rect(rectf(-5.0, 2.0, 10.0, 5.0), sizef(20.0, 10.0), size(40, 20)),
            rect(0, 4, 10, 10)
        );
    }

    #[test]
    fn rejects_invalid_or_overflow_adjacent_geometry() {
        assert!(rect_is_empty(scaled_integer_rect(
            rectf(f64::NAN, 0.0, 1.0, 1.0),
            sizef(10.0, 10.0),
            size(10, 10)
        )));
        assert!(rect_is_empty(scaled_integer_rect(
            rectf(0.0, 0.0, 1.0, 1.0),
            sizef(f64::INFINITY, 10.0),
            size(10, 10)
        )));
        assert!(rect_is_empty(scaled_integer_rect(
            rectf(0.0, 0.0, 1.0, 1.0),
            sizef(10.0, 10.0),
            size(0, 10)
        )));
        assert_eq!(
            bounded_integer_rect(rect(i32::MAX - 1, 0, 10, 1), size(i32::MAX, 1)),
            rect(i32::MAX - 1, 0, 1, 1)
        );
    }

    #[test]
    fn builds_tile_pyramid_levels_and_requests() {
        let levels = tile_pyramid_levels(size(1025, 513));
        assert_eq!(levels.len(), 12);
        assert_eq!(levels[0].size, size(1025, 513));
        assert_eq!(levels[1].size, size(513, 257));
        assert_eq!(levels[11].size, size(1, 1));
        assert_eq!(tile_grid_size(size(1025, 513), 512, 0), size(3, 2));

        assert_eq!(select_level_for_display_scale(size(4096, 4096), 0.25), 2);
        assert_eq!(
            level_tile_rect(size(1025, 513), 512, key(0, 2, 1)),
            rect(1024, 512, 1, 1)
        );
        assert_eq!(
            level_tile_texture_rect(size(1025, 513), 512, 1, key(0, 0, 0)),
            rect(0, 0, 513, 513)
        );
        assert_eq!(
            request_for_tile(size(80, 40), 512, 1, key(0, 0, 0)).source_rect,
            rect(0, 0, 80, 40)
        );
    }

    #[test]
    fn returns_intersecting_tiles_in_scan_order() {
        assert_eq!(
            tiles_intersecting_level_rect(size(1536, 1536), 512, 0, rect(500, 500, 24, 24)),
            vec![key(0, 0, 0), key(0, 1, 0), key(0, 0, 1), key(0, 1, 1)]
        );
    }

    #[test]
    fn selects_visible_tiles_and_prefetch_neighbors() {
        let keys = visible_tile_keys(
            size(2048, 1024),
            512,
            sizef(2048.0, 1024.0),
            rectf(512.0, 0.0, 512.0, 512.0),
            1.0,
        );
        assert_eq!(
            keys,
            vec![
                key(0, 1, 0),
                key(0, 0, 0),
                key(0, 2, 0),
                key(0, 0, 1),
                key(0, 1, 1),
                key(0, 2, 1)
            ]
        );

        let downsampled = visible_tile_keys(
            size(2048, 1024),
            512,
            sizef(512.0, 256.0),
            rectf(0.0, 0.0, 512.0, 256.0),
            1.0,
        );
        assert_eq!(downsampled.first().map(|key| key.level), Some(2));
    }

    #[test]
    fn first_display_sufficiency_uses_current_display_scale_with_tolerance() {
        assert!(tile_first_display_is_sufficient(
            size(2048, 2048),
            sizef(512.0, 512.0),
            1.0,
            0.25,
        ));
        assert!(tile_first_display_is_sufficient(
            size(2048, 2048),
            sizef(514.0, 514.0),
            1.0,
            0.25,
        ));
        assert!(!tile_first_display_is_sufficient(
            size(2048, 2048),
            sizef(516.0, 516.0),
            1.0,
            0.25,
        ));
        assert!(!tile_first_display_is_sufficient(
            size(2048, 2048),
            sizef(512.0, 512.0),
            1.0,
            0.0,
        ));
        assert!(!tile_first_display_is_sufficient(
            size(2048, 2048),
            sizef(512.0, 512.0),
            f64::NAN,
            0.25,
        ));
    }
}
