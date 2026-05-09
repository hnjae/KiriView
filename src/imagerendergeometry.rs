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
    struct RustImageDocumentRenderContext {
        device_pixel_ratio: f64,
        maximum_texture_size: i32,
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

        #[cxx_name = "rustSvgRasterSize"]
        fn rust_svg_raster_size(
            display_size: RustImageRenderSizeF,
            device_pixel_ratio: f64,
            maximum_texture_size: i32,
            fallback_texture_size_max: i32,
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

        #[cxx_name = "rustImageTileTextureRect"]
        fn rust_image_tile_texture_rect(
            level_rect: RustImageRenderRect,
            texture_level_rect: RustImageRenderRect,
        ) -> RustImageRenderRectF;
    }
}

use ffi::{
    RustImageDocumentRenderContext, RustImageRenderRect, RustImageRenderRectF, RustImageRenderSize,
    RustImageRenderSizeF,
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

fn rust_svg_raster_size(
    display_size: RustImageRenderSizeF,
    device_pixel_ratio: f64,
    maximum_texture_size: i32,
    fallback_texture_size_max: i32,
) -> RustImageRenderSize {
    if size_f_empty(display_size) || !device_pixel_ratio.is_finite() || device_pixel_ratio <= 0.0 {
        return empty_size();
    }

    let maximum_size = if maximum_texture_size > 0 {
        maximum_texture_size
    } else {
        fallback_texture_size_max
    };
    rust_scaled_image_size_to_fit(
        RustImageRenderSizeF {
            width: display_size.width * device_pixel_ratio,
            height: display_size.height * device_pixel_ratio,
        },
        RustImageRenderSize {
            width: maximum_size,
            height: maximum_size,
        },
    )
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
    if rect_empty(source_rect) || size_empty(image_size) || rect_f_empty(target_rect) {
        return empty_rect_f();
    }

    let image_width = f64::from(image_size.width);
    let image_height = f64::from(image_size.height);
    RustImageRenderRectF {
        x: target_rect.x + (f64::from(source_rect.x) / image_width) * target_rect.width,
        y: target_rect.y + (f64::from(source_rect.y) / image_height) * target_rect.height,
        width: (f64::from(source_rect.width) / image_width) * target_rect.width,
        height: (f64::from(source_rect.height) / image_height) * target_rect.height,
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
    fn svg_raster_size_applies_device_scale_and_texture_limit() {
        assert_eq!(
            rust_svg_raster_size(size_f(500.0, 250.0), 2.0, 512, 1024),
            size(512, 256)
        );
        assert_eq!(
            rust_svg_raster_size(size_f(10.0, 20.0), 2.0, 0, 64),
            size(20, 40)
        );
        assert_eq!(
            rust_svg_raster_size(size_f(10.0, 20.0), 0.0, 64, 64),
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
}
