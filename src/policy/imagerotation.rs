// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageRotationPointF {
        x: f64,
        y: f64,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageRotationRectF {
        x: f64,
        y: f64,
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageRotationSize {
        width: i32,
        height: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageRotationSizeF {
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy, Debug, PartialEq)]
    struct RustImageTextureCoordinateTransform {
        origin: RustImageRotationPointF,
        x_axis: RustImageRotationPointF,
        y_axis: RustImageRotationPointF,
    }

    extern "Rust" {
        #[cxx_name = "rustNormalizedImageRotationDegrees"]
        fn rust_normalized_image_rotation_degrees(degrees: i32) -> i32;

        #[cxx_name = "rustImageRotationClockwise"]
        fn rust_image_rotation_clockwise(degrees: i32) -> i32;

        #[cxx_name = "rustImageRotationCounterclockwise"]
        fn rust_image_rotation_counterclockwise(degrees: i32) -> i32;

        #[cxx_name = "rustImageRotationSwapsAxes"]
        fn rust_image_rotation_swaps_axes(degrees: i32) -> bool;

        #[cxx_name = "rustRotatedImageSize"]
        fn rust_rotated_image_size(
            size: RustImageRotationSize,
            degrees: i32,
        ) -> RustImageRotationSize;

        #[cxx_name = "rustRotatedImageSizeF"]
        fn rust_rotated_image_size_f(
            size: RustImageRotationSizeF,
            degrees: i32,
        ) -> RustImageRotationSizeF;

        #[cxx_name = "rustRotatedSourceRectInTarget"]
        fn rust_rotated_source_rect_in_target(
            source_rect: RustImageRotationRectF,
            source_size: RustImageRotationSizeF,
            target_rect: RustImageRotationRectF,
            degrees: i32,
        ) -> RustImageRotationRectF;

        #[cxx_name = "rustUnrotatedVisibleRectForRotation"]
        fn rust_unrotated_visible_rect_for_rotation(
            source_display_size: RustImageRotationSizeF,
            visible_item_rect: RustImageRotationRectF,
            degrees: i32,
        ) -> RustImageRotationRectF;

        #[cxx_name = "rustImageTextureCoordinateTransform"]
        fn rust_image_texture_coordinate_transform(
            texture_rect: RustImageRotationRectF,
            degrees: i32,
        ) -> RustImageTextureCoordinateTransform;
    }
}

use ffi::{
    RustImageRotationPointF, RustImageRotationRectF, RustImageRotationSize, RustImageRotationSizeF,
    RustImageTextureCoordinateTransform,
};

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

fn rust_rotated_image_size(size: RustImageRotationSize, degrees: i32) -> RustImageRotationSize {
    if rust_image_rotation_swaps_axes(degrees) {
        RustImageRotationSize {
            width: size.height,
            height: size.width,
        }
    } else {
        size
    }
}

fn rust_rotated_image_size_f(size: RustImageRotationSizeF, degrees: i32) -> RustImageRotationSizeF {
    if rust_image_rotation_swaps_axes(degrees) {
        RustImageRotationSizeF {
            width: size.height,
            height: size.width,
        }
    } else {
        size
    }
}

fn rust_rotated_source_rect_in_target(
    source_rect: RustImageRotationRectF,
    source_size: RustImageRotationSizeF,
    target_rect: RustImageRotationRectF,
    degrees: i32,
) -> RustImageRotationRectF {
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

    RustImageRotationRectF {
        x: target_rect.x + x * target_rect.width,
        y: target_rect.y + y * target_rect.height,
        width: width * target_rect.width,
        height: height * target_rect.height,
    }
}

fn rust_unrotated_visible_rect_for_rotation(
    source_display_size: RustImageRotationSizeF,
    visible_item_rect: RustImageRotationRectF,
    degrees: i32,
) -> RustImageRotationRectF {
    if size_f_empty(source_display_size) || rect_f_empty(visible_item_rect) {
        return empty_rect_f();
    }

    match rust_normalized_image_rotation_degrees(degrees) {
        90 => RustImageRotationRectF {
            x: visible_item_rect.y,
            y: source_display_size.height - visible_item_rect.x - visible_item_rect.width,
            width: visible_item_rect.height,
            height: visible_item_rect.width,
        },
        180 => RustImageRotationRectF {
            x: source_display_size.width - visible_item_rect.x - visible_item_rect.width,
            y: source_display_size.height - visible_item_rect.y - visible_item_rect.height,
            width: visible_item_rect.width,
            height: visible_item_rect.height,
        },
        270 => RustImageRotationRectF {
            x: source_display_size.width - visible_item_rect.y - visible_item_rect.height,
            y: visible_item_rect.x,
            width: visible_item_rect.height,
            height: visible_item_rect.width,
        },
        _ => visible_item_rect,
    }
}

fn rust_image_texture_coordinate_transform(
    texture_rect: RustImageRotationRectF,
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

fn size_f_empty(size: RustImageRotationSizeF) -> bool {
    size.width <= 0.0 || size.height <= 0.0
}

fn rect_f_empty(rect: RustImageRotationRectF) -> bool {
    rect.width <= 0.0 || rect.height <= 0.0
}

fn empty_rect_f() -> RustImageRotationRectF {
    RustImageRotationRectF {
        x: 0.0,
        y: 0.0,
        width: 0.0,
        height: 0.0,
    }
}

fn point(x: f64, y: f64) -> RustImageRotationPointF {
    RustImageRotationPointF { x, y }
}

fn transform(
    origin: RustImageRotationPointF,
    x_axis: RustImageRotationPointF,
    y_axis: RustImageRotationPointF,
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

    fn size(width: i32, height: i32) -> RustImageRotationSize {
        RustImageRotationSize { width, height }
    }

    fn size_f(width: f64, height: f64) -> RustImageRotationSizeF {
        RustImageRotationSizeF { width, height }
    }

    fn rect_f(x: f64, y: f64, width: f64, height: f64) -> RustImageRotationRectF {
        RustImageRotationRectF {
            x,
            y,
            width,
            height,
        }
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
    fn source_rect_mapping_accounts_for_rotation() {
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
