// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageRenderSize {
        width: i32,
        height: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageRenderSizeF {
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageRenderRectF {
        x: f64,
        y: f64,
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageRenderRect {
        x: i32,
        y: i32,
        width: i32,
        height: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageRenderPointF {
        x: f64,
        y: f64,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageDocumentRenderContext {
        device_pixel_ratio: f64,
        maximum_texture_size: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageTextureCoordinateTransform {
        origin: RustImageRenderPointF,
        x_axis: RustImageRenderPointF,
        y_axis: RustImageRenderPointF,
    }

    extern "Rust" {
        #[cxx_name = "rustImageTargetRect"]
        fn rust_image_target_rect(
            image_size: RustImageRenderSize,
            bounds_size: RustImageRenderSizeF,
        ) -> RustImageRenderRectF;

        #[cxx_name = "rustScaledImageSizeToFit"]
        fn rust_scaled_image_size_to_fit(
            image_size: RustImageRenderSizeF,
            bounds_size: RustImageRenderSize,
        ) -> RustImageRenderSize;

        #[cxx_name = "rustNormalizedImageDocumentRenderContext"]
        fn rust_normalized_image_document_render_context(
            context: RustImageDocumentRenderContext,
            fallback_texture_size_max: i32,
        ) -> RustImageDocumentRenderContext;

        #[cxx_name = "rustFirstDisplayPhysicalViewportSize"]
        fn rust_first_display_physical_viewport_size(
            viewport_size: RustImageRenderSizeF,
            device_pixel_ratio: f64,
        ) -> RustImageRenderSize;

        #[cxx_name = "rustFirstDisplayScaledImageSize"]
        fn rust_first_display_scaled_image_size(
            image_size: RustImageRenderSize,
            physical_viewport_size: RustImageRenderSize,
        ) -> RustImageRenderSize;

        #[cxx_name = "rustImagePixelsPerSourcePixel"]
        fn rust_image_pixels_per_source_pixel(
            image_size: RustImageRenderSize,
            display_size: RustImageRenderSize,
        ) -> f64;

        #[cxx_name = "rustImageTileTargetRect"]
        fn rust_image_tile_target_rect(
            source_rect: RustImageRenderRect,
            image_size: RustImageRenderSize,
            target_rect: RustImageRenderRectF,
        ) -> RustImageRenderRectF;

        #[cxx_name = "rustImageTileTargetRectF"]
        fn rust_image_tile_target_rect_f(
            source_rect: RustImageRenderRectF,
            image_size: RustImageRenderSize,
            target_rect: RustImageRenderRectF,
        ) -> RustImageRenderRectF;

        #[cxx_name = "rustImageTileTextureRect"]
        fn rust_image_tile_texture_rect(
            level_rect: RustImageRenderRect,
            texture_level_rect: RustImageRenderRect,
        ) -> RustImageRenderRectF;

        #[cxx_name = "rustStaticImageFitsFullImageSurface"]
        fn rust_static_image_fits_full_image_surface(
            static_image_valid: bool,
            image_size: RustImageRenderSize,
            preview_size: RustImageRenderSize,
            maximum_texture_size: i32,
        ) -> bool;

        #[cxx_name = "rustNormalizedImageRotationDegrees"]
        fn rust_normalized_image_rotation_degrees(degrees: i32) -> i32;

        #[cxx_name = "rustImageRotationClockwise"]
        fn rust_image_rotation_clockwise(degrees: i32) -> i32;

        #[cxx_name = "rustImageRotationCounterclockwise"]
        fn rust_image_rotation_counterclockwise(degrees: i32) -> i32;

        #[cxx_name = "rustImageRotationSwapsAxes"]
        fn rust_image_rotation_swaps_axes(degrees: i32) -> bool;

        #[cxx_name = "rustRotatedImageSize"]
        fn rust_rotated_image_size(size: RustImageRenderSize, degrees: i32) -> RustImageRenderSize;

        #[cxx_name = "rustRotatedImageSizeF"]
        fn rust_rotated_image_size_f(
            size: RustImageRenderSizeF,
            degrees: i32,
        ) -> RustImageRenderSizeF;

        #[cxx_name = "rustRotatedSourceRectInTarget"]
        fn rust_rotated_source_rect_in_target(
            source_rect: RustImageRenderRectF,
            source_size: RustImageRenderSizeF,
            target_rect: RustImageRenderRectF,
            degrees: i32,
        ) -> RustImageRenderRectF;

        #[cxx_name = "rustUnrotatedVisibleRectForRotation"]
        fn rust_unrotated_visible_rect_for_rotation(
            source_display_size: RustImageRenderSizeF,
            visible_item_rect: RustImageRenderRectF,
            degrees: i32,
        ) -> RustImageRenderRectF;

        #[cxx_name = "rustImageTextureCoordinateTransform"]
        fn rust_image_texture_coordinate_transform(
            texture_rect: RustImageRenderRectF,
            degrees: i32,
        ) -> RustImageTextureCoordinateTransform;
    }
}

use ffi::{
    RustImageDocumentRenderContext, RustImageRenderPointF, RustImageRenderRect,
    RustImageRenderRectF, RustImageRenderSize, RustImageRenderSizeF,
    RustImageTextureCoordinateTransform,
};

fn rust_image_target_rect(
    image_size: RustImageRenderSize,
    bounds_size: RustImageRenderSizeF,
) -> RustImageRenderRectF {
    if size_empty(image_size) || size_f_empty(bounds_size) {
        return empty_rect_f();
    }

    let scale = min_like_cpp(
        bounds_size.width / f64::from(image_size.width),
        bounds_size.height / f64::from(image_size.height),
    );
    let target_width = f64::from(image_size.width) * scale;
    let target_height = f64::from(image_size.height) * scale;
    RustImageRenderRectF {
        x: (bounds_size.width - target_width) / 2.0,
        y: (bounds_size.height - target_height) / 2.0,
        width: target_width,
        height: target_height,
    }
}

fn rust_scaled_image_size_to_fit(
    image_size: RustImageRenderSizeF,
    bounds_size: RustImageRenderSize,
) -> RustImageRenderSize {
    if size_f_empty(image_size) || size_empty(bounds_size) {
        return empty_size();
    }

    let image_width = image_size.width;
    let image_height = image_size.height;
    if !image_width.is_finite()
        || !image_height.is_finite()
        || image_width <= 0.0
        || image_height <= 0.0
    {
        return empty_size();
    }

    let scale = min_like_cpp(
        1.0,
        min_like_cpp(
            f64::from(bounds_size.width) / image_width,
            f64::from(bounds_size.height) / image_height,
        ),
    );
    if !scale.is_finite() || scale <= 0.0 {
        return empty_size();
    }

    RustImageRenderSize {
        width: ceil_clamped_to_i32(image_width * scale, 1, bounds_size.width),
        height: ceil_clamped_to_i32(image_height * scale, 1, bounds_size.height),
    }
}

fn rust_normalized_image_document_render_context(
    context: RustImageDocumentRenderContext,
    fallback_texture_size_max: i32,
) -> RustImageDocumentRenderContext {
    RustImageDocumentRenderContext {
        device_pixel_ratio: if context.device_pixel_ratio.is_finite()
            && context.device_pixel_ratio > 0.0
        {
            context.device_pixel_ratio
        } else {
            1.0
        },
        maximum_texture_size: if context.maximum_texture_size > 0 {
            context.maximum_texture_size
        } else {
            fallback_texture_size_max
        },
    }
}

fn rust_first_display_physical_viewport_size(
    viewport_size: RustImageRenderSizeF,
    device_pixel_ratio: f64,
) -> RustImageRenderSize {
    if size_f_empty(viewport_size) || !device_pixel_ratio.is_finite() || device_pixel_ratio <= 0.0 {
        return empty_size();
    }

    let Some(width) = ceil_positive_to_i32(viewport_size.width * device_pixel_ratio) else {
        return empty_size();
    };
    let Some(height) = ceil_positive_to_i32(viewport_size.height * device_pixel_ratio) else {
        return empty_size();
    };

    RustImageRenderSize { width, height }
}

fn rust_first_display_scaled_image_size(
    image_size: RustImageRenderSize,
    physical_viewport_size: RustImageRenderSize,
) -> RustImageRenderSize {
    if size_empty(image_size) || size_empty(physical_viewport_size) {
        return empty_size();
    }

    let scaled_size = rust_scaled_image_size_to_fit(
        RustImageRenderSizeF {
            width: f64::from(image_size.width),
            height: f64::from(image_size.height),
        },
        physical_viewport_size,
    );
    if size_empty(scaled_size) || scaled_size == image_size {
        return empty_size();
    }

    scaled_size
}

fn rust_image_pixels_per_source_pixel(
    image_size: RustImageRenderSize,
    display_size: RustImageRenderSize,
) -> f64 {
    if size_empty(image_size) || size_empty(display_size) {
        return 0.0;
    }

    let scale = min_like_cpp(
        f64::from(display_size.width) / f64::from(image_size.width),
        f64::from(display_size.height) / f64::from(image_size.height),
    );
    if scale.is_finite() && scale > 0.0 {
        scale
    } else {
        0.0
    }
}

fn rust_image_tile_target_rect(
    source_rect: RustImageRenderRect,
    image_size: RustImageRenderSize,
    target_rect: RustImageRenderRectF,
) -> RustImageRenderRectF {
    rust_image_tile_target_rect_f(
        RustImageRenderRectF {
            x: f64::from(source_rect.x),
            y: f64::from(source_rect.y),
            width: f64::from(source_rect.width),
            height: f64::from(source_rect.height),
        },
        image_size,
        target_rect,
    )
}

fn rust_image_tile_target_rect_f(
    source_rect: RustImageRenderRectF,
    image_size: RustImageRenderSize,
    target_rect: RustImageRenderRectF,
) -> RustImageRenderRectF {
    if rect_f_empty(source_rect) || size_empty(image_size) || rect_f_empty(target_rect) {
        return empty_rect_f();
    }

    let image_width = f64::from(image_size.width);
    let image_height = f64::from(image_size.height);
    RustImageRenderRectF {
        x: target_rect.x + (source_rect.x / image_width) * target_rect.width,
        y: target_rect.y + (source_rect.y / image_height) * target_rect.height,
        width: (source_rect.width / image_width) * target_rect.width,
        height: (source_rect.height / image_height) * target_rect.height,
    }
}

fn rust_image_tile_texture_rect(
    level_rect: RustImageRenderRect,
    texture_level_rect: RustImageRenderRect,
) -> RustImageRenderRectF {
    if rect_empty(texture_level_rect) {
        return empty_rect_f();
    }

    let texture_width = f64::from(texture_level_rect.width);
    let texture_height = f64::from(texture_level_rect.height);
    RustImageRenderRectF {
        x: (f64::from(level_rect.x) - f64::from(texture_level_rect.x)) / texture_width,
        y: (f64::from(level_rect.y) - f64::from(texture_level_rect.y)) / texture_height,
        width: f64::from(level_rect.width) / texture_width,
        height: f64::from(level_rect.height) / texture_height,
    }
}

fn rust_static_image_fits_full_image_surface(
    static_image_valid: bool,
    image_size: RustImageRenderSize,
    preview_size: RustImageRenderSize,
    maximum_texture_size: i32,
) -> bool {
    static_image_valid
        && !size_empty(image_size)
        && image_size == preview_size
        && maximum_texture_size > 0
        && image_size.width <= maximum_texture_size
        && image_size.height <= maximum_texture_size
}

fn rust_normalized_image_rotation_degrees(degrees: i32) -> i32 {
    let normalized = degrees.rem_euclid(360);
    if normalized % 90 == 0 { normalized } else { 0 }
}

fn rust_image_rotation_clockwise(degrees: i32) -> i32 {
    rust_normalized_image_rotation_degrees(degrees + 90)
}

fn rust_image_rotation_counterclockwise(degrees: i32) -> i32 {
    rust_normalized_image_rotation_degrees(degrees - 90)
}

fn rust_image_rotation_swaps_axes(degrees: i32) -> bool {
    matches!(rust_normalized_image_rotation_degrees(degrees), 90 | 270)
}

fn rust_rotated_image_size(size: RustImageRenderSize, degrees: i32) -> RustImageRenderSize {
    if rust_image_rotation_swaps_axes(degrees) {
        RustImageRenderSize {
            width: size.height,
            height: size.width,
        }
    } else {
        size
    }
}

fn rust_rotated_image_size_f(size: RustImageRenderSizeF, degrees: i32) -> RustImageRenderSizeF {
    if rust_image_rotation_swaps_axes(degrees) {
        RustImageRenderSizeF {
            width: size.height,
            height: size.width,
        }
    } else {
        size
    }
}

fn rust_rotated_source_rect_in_target(
    source_rect: RustImageRenderRectF,
    source_size: RustImageRenderSizeF,
    target_rect: RustImageRenderRectF,
    degrees: i32,
) -> RustImageRenderRectF {
    if rect_f_empty(source_rect) || size_f_empty(source_size) || rect_f_empty(target_rect) {
        return empty_rect_f();
    }

    let source_width = source_size.width;
    let source_height = source_size.height;
    if source_width <= 0.0 || source_height <= 0.0 {
        return empty_rect_f();
    }

    let (x, y, width, height) = match rust_normalized_image_rotation_degrees(degrees) {
        90 => (
            (source_height - source_rect.y - source_rect.height) / source_height,
            source_rect.x / source_width,
            source_rect.height / source_height,
            source_rect.width / source_width,
        ),
        180 => (
            (source_width - source_rect.x - source_rect.width) / source_width,
            (source_height - source_rect.y - source_rect.height) / source_height,
            source_rect.width / source_width,
            source_rect.height / source_height,
        ),
        270 => (
            source_rect.y / source_height,
            (source_width - source_rect.x - source_rect.width) / source_width,
            source_rect.height / source_height,
            source_rect.width / source_width,
        ),
        _ => (
            source_rect.x / source_width,
            source_rect.y / source_height,
            source_rect.width / source_width,
            source_rect.height / source_height,
        ),
    };

    RustImageRenderRectF {
        x: target_rect.x + x * target_rect.width,
        y: target_rect.y + y * target_rect.height,
        width: width * target_rect.width,
        height: height * target_rect.height,
    }
}

fn rust_unrotated_visible_rect_for_rotation(
    source_display_size: RustImageRenderSizeF,
    visible_item_rect: RustImageRenderRectF,
    degrees: i32,
) -> RustImageRenderRectF {
    if size_f_empty(source_display_size) || rect_f_empty(visible_item_rect) {
        return empty_rect_f();
    }

    match rust_normalized_image_rotation_degrees(degrees) {
        90 => RustImageRenderRectF {
            x: visible_item_rect.y,
            y: source_display_size.height - visible_item_rect.x - visible_item_rect.width,
            width: visible_item_rect.height,
            height: visible_item_rect.width,
        },
        180 => RustImageRenderRectF {
            x: source_display_size.width - visible_item_rect.x - visible_item_rect.width,
            y: source_display_size.height - visible_item_rect.y - visible_item_rect.height,
            width: visible_item_rect.width,
            height: visible_item_rect.height,
        },
        270 => RustImageRenderRectF {
            x: source_display_size.width - visible_item_rect.y - visible_item_rect.height,
            y: visible_item_rect.x,
            width: visible_item_rect.height,
            height: visible_item_rect.width,
        },
        _ => visible_item_rect,
    }
}

fn rust_image_texture_coordinate_transform(
    texture_rect: RustImageRenderRectF,
    degrees: i32,
) -> RustImageTextureCoordinateTransform {
    let left = texture_rect.x;
    let top = texture_rect.y;
    let right = texture_rect.x + texture_rect.width;
    let bottom = texture_rect.y + texture_rect.height;
    let width = texture_rect.width;
    let height = texture_rect.height;

    match rust_normalized_image_rotation_degrees(degrees) {
        90 => transform(point(left, bottom), point(0.0, -height), point(width, 0.0)),
        180 => transform(
            point(right, bottom),
            point(-width, 0.0),
            point(0.0, -height),
        ),
        270 => transform(point(right, top), point(0.0, height), point(-width, 0.0)),
        _ => transform(point(left, top), point(width, 0.0), point(0.0, height)),
    }
}

fn min_like_cpp(left: f64, right: f64) -> f64 {
    if right < left { right } else { left }
}

fn ceil_clamped_to_i32(value: f64, minimum: i32, maximum: i32) -> i32 {
    let rounded = value.ceil();
    if rounded <= f64::from(minimum) {
        minimum
    } else if rounded >= f64::from(maximum) {
        maximum
    } else {
        rounded as i32
    }
}

fn ceil_positive_to_i32(value: f64) -> Option<i32> {
    if !value.is_finite() {
        return None;
    }

    let rounded = value.ceil();
    if rounded <= 0.0 {
        None
    } else if rounded >= f64::from(i32::MAX) {
        Some(i32::MAX)
    } else {
        Some(rounded as i32)
    }
}

fn size_empty(size: RustImageRenderSize) -> bool {
    size.width <= 0 || size.height <= 0
}

fn size_f_empty(size: RustImageRenderSizeF) -> bool {
    size.width <= 0.0 || size.height <= 0.0
}

fn rect_empty(rect: RustImageRenderRect) -> bool {
    rect.width <= 0 || rect.height <= 0
}

fn rect_f_empty(rect: RustImageRenderRectF) -> bool {
    rect.width <= 0.0 || rect.height <= 0.0
}

fn empty_size() -> RustImageRenderSize {
    RustImageRenderSize {
        width: -1,
        height: -1,
    }
}

fn empty_rect_f() -> RustImageRenderRectF {
    RustImageRenderRectF {
        x: 0.0,
        y: 0.0,
        width: 0.0,
        height: 0.0,
    }
}

fn point(x: f64, y: f64) -> RustImageRenderPointF {
    RustImageRenderPointF { x, y }
}

fn transform(
    origin: RustImageRenderPointF,
    x_axis: RustImageRenderPointF,
    y_axis: RustImageRenderPointF,
) -> RustImageTextureCoordinateTransform {
    RustImageTextureCoordinateTransform {
        origin,
        x_axis,
        y_axis,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn size(width: i32, height: i32) -> RustImageRenderSize {
        RustImageRenderSize { width, height }
    }

    fn size_f(width: f64, height: f64) -> RustImageRenderSizeF {
        RustImageRenderSizeF { width, height }
    }

    fn rect(x: i32, y: i32, width: i32, height: i32) -> RustImageRenderRect {
        RustImageRenderRect {
            x,
            y,
            width,
            height,
        }
    }

    fn rect_f(x: f64, y: f64, width: f64, height: f64) -> RustImageRenderRectF {
        RustImageRenderRectF {
            x,
            y,
            width,
            height,
        }
    }

    #[test]
    fn target_rect_fits_and_centers_image_in_bounds() {
        let rect = rust_image_target_rect(size(1000, 500), size_f(400.0, 400.0));

        assert_eq!(rect.x, 0.0);
        assert_eq!(rect.y, 100.0);
        assert_eq!(rect.width, 400.0);
        assert_eq!(rect.height, 200.0);
    }

    #[test]
    fn scaled_size_preserves_aspect_ratio_without_upscaling() {
        assert_eq!(
            rust_scaled_image_size_to_fit(size_f(4000.0, 2000.0), size(1000, 1000)),
            size(1000, 500)
        );
        assert_eq!(
            rust_scaled_image_size_to_fit(size_f(333.0, 100.0), size(200, 200)),
            size(200, 61)
        );
        assert_eq!(
            rust_scaled_image_size_to_fit(size_f(200.0, 100.0), size(1000, 1000)),
            size(200, 100)
        );
        assert_eq!(
            rust_scaled_image_size_to_fit(size_f(0.5, 0.5), size(100, 100)),
            size(1, 1)
        );
    }

    #[test]
    fn scaled_size_rejects_invalid_input() {
        assert_eq!(
            rust_scaled_image_size_to_fit(size_f(0.0, 100.0), size(100, 100)),
            empty_size()
        );
        assert_eq!(
            rust_scaled_image_size_to_fit(size_f(f64::NAN, 100.0), size(100, 100)),
            empty_size()
        );
        assert_eq!(
            rust_scaled_image_size_to_fit(size_f(100.0, 100.0), size(0, 100)),
            empty_size()
        );
    }

    #[test]
    fn render_context_normalizes_invalid_values() {
        assert_eq!(
            rust_normalized_image_document_render_context(
                RustImageDocumentRenderContext {
                    device_pixel_ratio: 2.0,
                    maximum_texture_size: 4096,
                },
                1024,
            ),
            RustImageDocumentRenderContext {
                device_pixel_ratio: 2.0,
                maximum_texture_size: 4096,
            }
        );
        assert_eq!(
            rust_normalized_image_document_render_context(
                RustImageDocumentRenderContext {
                    device_pixel_ratio: f64::NAN,
                    maximum_texture_size: 0,
                },
                1024,
            ),
            RustImageDocumentRenderContext {
                device_pixel_ratio: 1.0,
                maximum_texture_size: 1024,
            }
        );
    }

    #[test]
    fn first_display_physical_viewport_uses_ceiled_device_pixels() {
        assert_eq!(
            rust_first_display_physical_viewport_size(size_f(400.25, 300.0), 2.0),
            size(801, 600)
        );
        assert_eq!(
            rust_first_display_physical_viewport_size(size_f(400.0, 300.0), 0.0),
            empty_size()
        );
        assert_eq!(
            rust_first_display_physical_viewport_size(size_f(0.0, 300.0), 2.0),
            empty_size()
        );
    }

    #[test]
    fn first_display_scaled_image_size_downscales_without_full_size_decode() {
        assert_eq!(
            rust_first_display_scaled_image_size(size(1600, 1200), size(400, 300)),
            size(400, 300)
        );
        assert_eq!(
            rust_first_display_scaled_image_size(size(200, 100), size(400, 300)),
            empty_size()
        );
        assert_eq!(
            rust_first_display_scaled_image_size(size(1600, 1200), size(0, 300)),
            empty_size()
        );
    }

    #[test]
    fn image_pixels_per_source_pixel_uses_limiting_axis() {
        assert_eq!(
            rust_image_pixels_per_source_pixel(size(1600, 1200), size(400, 300)),
            0.25
        );
        assert_eq!(
            rust_image_pixels_per_source_pixel(size(1600, 1200), size(800, 300)),
            0.25
        );
        assert_eq!(
            rust_image_pixels_per_source_pixel(size(1600, 1200), size(0, 300)),
            0.0
        );
    }

    #[test]
    fn tile_target_rect_maps_source_rect_into_image_target() {
        let target = rust_image_tile_target_rect(
            rect(256, 512, 256, 256),
            size(1024, 1024),
            rect_f(10.0, 20.0, 1000.0, 500.0),
        );

        assert_eq!(target, rect_f(260.0, 270.0, 250.0, 125.0));
    }

    #[test]
    fn tile_texture_rect_maps_level_rect_into_texture_coordinates() {
        let texture =
            rust_image_tile_texture_rect(rect(256, 128, 512, 256), rect(128, 0, 1024, 512));

        assert_eq!(texture, rect_f(0.125, 0.25, 0.5, 0.5));
    }

    #[test]
    fn tile_rects_reject_invalid_geometry() {
        assert_eq!(
            rust_image_tile_target_rect(
                rect(0, 0, 0, 512),
                size(1024, 1024),
                rect_f(10.0, 20.0, 1000.0, 500.0)
            ),
            empty_rect_f()
        );
        assert_eq!(
            rust_image_tile_texture_rect(rect(0, 0, 512, 512), rect(0, 0, 0, 512)),
            empty_rect_f()
        );
    }

    #[test]
    fn static_image_full_surface_policy_requires_valid_matching_sizes_within_texture_limit() {
        assert!(rust_static_image_fits_full_image_surface(
            true,
            size(512, 256),
            size(512, 256),
            512,
        ));
        assert!(!rust_static_image_fits_full_image_surface(
            false,
            size(512, 256),
            size(512, 256),
            512,
        ));
        assert!(!rust_static_image_fits_full_image_surface(
            true,
            size(512, 256),
            size(256, 128),
            512,
        ));
        assert!(!rust_static_image_fits_full_image_surface(
            true,
            size(513, 256),
            size(513, 256),
            512,
        ));
        assert!(!rust_static_image_fits_full_image_surface(
            true,
            size(512, 256),
            size(512, 256),
            0,
        ));
    }

    #[test]
    fn rotation_degrees_normalize_to_quarter_turns() {
        assert_eq!(rust_normalized_image_rotation_degrees(0), 0);
        assert_eq!(rust_normalized_image_rotation_degrees(450), 90);
        assert_eq!(rust_normalized_image_rotation_degrees(-90), 270);
        assert_eq!(rust_normalized_image_rotation_degrees(45), 0);
        assert_eq!(rust_image_rotation_clockwise(270), 0);
        assert_eq!(rust_image_rotation_counterclockwise(0), 270);
    }

    #[test]
    fn axis_swap_policy_controls_rotated_sizes() {
        assert!(rust_image_rotation_swaps_axes(90));
        assert!(rust_image_rotation_swaps_axes(270));
        assert!(!rust_image_rotation_swaps_axes(180));
        assert_eq!(rust_rotated_image_size(size(640, 480), 90), size(480, 640));
        assert_eq!(
            rust_rotated_image_size_f(size_f(640.0, 480.0), 180),
            size_f(640.0, 480.0)
        );
    }

    #[test]
    fn rotated_source_rect_maps_into_target_space() {
        let source_rect = rect_f(250.0, 100.0, 500.0, 200.0);
        let source_size = size_f(1000.0, 500.0);

        assert_eq!(
            rust_rotated_source_rect_in_target(
                source_rect,
                source_size,
                rect_f(10.0, 20.0, 500.0, 1000.0),
                90
            ),
            rect_f(210.0, 270.0, 200.0, 500.0)
        );
        assert_eq!(
            rust_rotated_source_rect_in_target(
                source_rect,
                source_size,
                rect_f(10.0, 20.0, 1000.0, 500.0),
                180
            ),
            rect_f(260.0, 220.0, 500.0, 200.0)
        );
        assert_eq!(
            rust_rotated_source_rect_in_target(
                source_rect,
                source_size,
                rect_f(10.0, 20.0, 500.0, 1000.0),
                270
            ),
            rect_f(110.0, 270.0, 200.0, 500.0)
        );
    }

    #[test]
    fn visible_rect_mapping_returns_unrotated_source_rect() {
        assert_eq!(
            rust_unrotated_visible_rect_for_rotation(
                size_f(1000.0, 500.0),
                rect_f(10.0, 20.0, 100.0, 200.0),
                90
            ),
            rect_f(20.0, 390.0, 200.0, 100.0)
        );
        assert_eq!(
            rust_unrotated_visible_rect_for_rotation(
                size_f(1000.0, 500.0),
                rect_f(10.0, 20.0, 100.0, 200.0),
                270
            ),
            rect_f(780.0, 10.0, 200.0, 100.0)
        );
    }

    #[test]
    fn texture_coordinate_transform_rotates_axes() {
        assert_eq!(
            rust_image_texture_coordinate_transform(rect_f(0.25, 0.5, 0.5, 0.25), 90),
            transform(point(0.25, 0.75), point(0.0, -0.25), point(0.5, 0.0))
        );
        assert_eq!(
            rust_image_texture_coordinate_transform(rect_f(0.25, 0.5, 0.5, 0.25), 270),
            transform(point(0.75, 0.5), point(0.0, 0.25), point(-0.5, 0.0))
        );
    }
}
