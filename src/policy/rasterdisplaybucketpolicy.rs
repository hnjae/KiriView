// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const DISPLAY_IMAGE_QUALITY_EXACT: i32 = 0;
const DISPLAY_IMAGE_QUALITY_BOUNDED_DETAIL: i32 = 3;
const DISPLAY_IMAGE_QUALITY_UNSUPPORTED: i32 = 4;
const DISPLAY_IMAGE_QUALITY_FAILED: i32 = 5;
const DISPLAY_BYTES_PER_PIXEL: i64 = 4;

use crate::imagerendergeometry::image_rotation_swaps_axes;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, PartialEq, Eq)]
    enum RustRasterDisplayBucketStatus {
        Exact = 0,
        FirstDisplaySufficient = 1,
        RefinementNeeded = 2,
        BoundedDetail = 3,
        UnsupportedTooLarge = 4,
        Failed = 5,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustRasterDisplayBucketKey {
        raster_width: i32,
        raster_height: i32,
        exact: bool,
        maximum_texture_size: i32,
        display_image_byte_budget: i64,
    }

    #[derive(Clone, Copy)]
    struct RustRasterDisplayBucketInput {
        original_width: i32,
        original_height: i32,
        current_raster_width: i32,
        current_raster_height: i32,
        current_quality: i32,
        display_width: f64,
        display_height: f64,
        visible_x: f64,
        visible_y: f64,
        visible_width: f64,
        visible_height: f64,
        device_pixel_ratio: f64,
        rotation_degrees: i32,
        maximum_texture_size: i32,
        display_image_byte_budget: i64,
    }

    #[derive(Clone, Copy)]
    struct RustRasterDisplayBucketDecision {
        status: RustRasterDisplayBucketStatus,
        quality: i32,
        bucket_key: RustRasterDisplayBucketKey,
        refinement_required: bool,
        current_image_retained: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustRasterDisplayBucketDecision"]
        fn rust_raster_display_bucket_decision(
            input: RustRasterDisplayBucketInput,
        ) -> RustRasterDisplayBucketDecision;
    }
}

use ffi::{
    RustRasterDisplayBucketDecision, RustRasterDisplayBucketInput, RustRasterDisplayBucketKey,
    RustRasterDisplayBucketStatus,
};

#[derive(Clone, Copy)]
struct Size {
    width: i32,
    height: i32,
}

impl Size {
    fn empty() -> Size {
        Size {
            width: 0,
            height: 0,
        }
    }

    fn is_empty(self) -> bool {
        self.width <= 0 || self.height <= 0
    }
}

fn rust_raster_display_bucket_decision(
    input: RustRasterDisplayBucketInput,
) -> RustRasterDisplayBucketDecision {
    let original_size = Size {
        width: input.original_width,
        height: input.original_height,
    };
    let current_size = Size {
        width: input.current_raster_width,
        height: input.current_raster_height,
    };
    if original_size.is_empty() || !display_demand_is_valid(input) {
        return decision(
            RustRasterDisplayBucketStatus::Failed,
            DISPLAY_IMAGE_QUALITY_FAILED,
            empty_bucket_key(input),
            false,
            false,
        );
    }

    let desired_size = desired_bucket_size(original_size, input);
    if desired_size.is_empty() {
        return decision(
            RustRasterDisplayBucketStatus::Failed,
            DISPLAY_IMAGE_QUALITY_FAILED,
            empty_bucket_key(input),
            false,
            false,
        );
    }

    const NO_REFINEMENT: bool = false;
    const REFINEMENT: bool = true;
    let has_current = !current_size.is_empty();
    let current_is_safe = has_current && size_is_safe(current_size, input);
    let current_is_exact = current_size.width >= original_size.width
        && current_size.height >= original_size.height
        && input.current_quality == DISPLAY_IMAGE_QUALITY_EXACT;

    if current_is_exact && current_is_safe {
        return decision(
            RustRasterDisplayBucketStatus::Exact,
            DISPLAY_IMAGE_QUALITY_EXACT,
            bucket_key(original_size, original_size, input),
            NO_REFINEMENT,
            true,
        );
    }

    if size_is_safe(desired_size, input) {
        if has_current && current_satisfies(current_size, desired_size) && current_is_safe {
            return decision(
                RustRasterDisplayBucketStatus::FirstDisplaySufficient,
                input.current_quality,
                bucket_key(current_size, original_size, input),
                NO_REFINEMENT,
                true,
            );
        }

        return decision(
            RustRasterDisplayBucketStatus::RefinementNeeded,
            DISPLAY_IMAGE_QUALITY_EXACT,
            bucket_key(desired_size, original_size, input),
            REFINEMENT,
            has_current,
        );
    }

    if current_is_safe {
        return decision(
            RustRasterDisplayBucketStatus::BoundedDetail,
            DISPLAY_IMAGE_QUALITY_BOUNDED_DETAIL,
            bucket_key(current_size, original_size, input),
            NO_REFINEMENT,
            true,
        );
    }

    if let Some(bounded_size) = bounded_bucket_size(original_size, desired_size, input) {
        return decision(
            RustRasterDisplayBucketStatus::RefinementNeeded,
            DISPLAY_IMAGE_QUALITY_BOUNDED_DETAIL,
            bucket_key(bounded_size, original_size, input),
            REFINEMENT,
            false,
        );
    }

    decision(
        RustRasterDisplayBucketStatus::UnsupportedTooLarge,
        DISPLAY_IMAGE_QUALITY_UNSUPPORTED,
        empty_bucket_key(input),
        NO_REFINEMENT,
        false,
    )
}

fn display_demand_is_valid(input: RustRasterDisplayBucketInput) -> bool {
    input.display_width.is_finite()
        && input.display_height.is_finite()
        && input.display_width > 0.0
        && input.display_height > 0.0
        && input.visible_x.is_finite()
        && input.visible_y.is_finite()
        && input.visible_width.is_finite()
        && input.visible_height.is_finite()
        && input.visible_width > 0.0
        && input.visible_height > 0.0
        && input.device_pixel_ratio.is_finite()
        && input.device_pixel_ratio > 0.0
}

fn desired_bucket_size(original_size: Size, input: RustRasterDisplayBucketInput) -> Size {
    let mut display_width = input.display_width;
    let mut display_height = input.display_height;
    if image_rotation_swaps_axes(input.rotation_degrees) {
        core::mem::swap(&mut display_width, &mut display_height);
    }

    let physical_width = display_width * input.device_pixel_ratio;
    let physical_height = display_height * input.device_pixel_ratio;
    if !physical_width.is_finite() || !physical_height.is_finite() {
        return Size::empty();
    }

    let scale = 1.0_f64.min(
        (physical_width / f64::from(original_size.width))
            .max(physical_height / f64::from(original_size.height)),
    );
    if !scale.is_finite() || scale <= 0.0 {
        return Size::empty();
    }

    Size {
        width: ceil_scaled_dimension(original_size.width, scale),
        height: ceil_scaled_dimension(original_size.height, scale),
    }
}

fn ceil_scaled_dimension(source: i32, scale: f64) -> i32 {
    ((f64::from(source) * scale).ceil() as i64).clamp(1, i64::from(source)) as i32
}

fn floor_scaled_dimension(source: i32, scale: f64) -> i32 {
    ((f64::from(source) * scale).floor() as i64).clamp(1, i64::from(source)) as i32
}

fn current_satisfies(current_size: Size, desired_size: Size) -> bool {
    !current_size.is_empty()
        && !desired_size.is_empty()
        && current_size.width >= desired_size.width
        && current_size.height >= desired_size.height
}

fn size_is_safe(size: Size, input: RustRasterDisplayBucketInput) -> bool {
    if size.is_empty()
        || input.maximum_texture_size <= 0
        || size.width > input.maximum_texture_size
        || size.height > input.maximum_texture_size
    {
        return false;
    }

    let Some(byte_cost) = image_byte_cost(size) else {
        return false;
    };
    input.display_image_byte_budget >= byte_cost
}

fn image_byte_cost(size: Size) -> Option<i64> {
    let pixels = i64::from(size.width).checked_mul(i64::from(size.height))?;
    pixels.checked_mul(DISPLAY_BYTES_PER_PIXEL)
}

fn bounded_bucket_size(
    original_size: Size,
    desired_size: Size,
    input: RustRasterDisplayBucketInput,
) -> Option<Size> {
    if input.maximum_texture_size <= 0 || input.display_image_byte_budget < DISPLAY_BYTES_PER_PIXEL
    {
        return None;
    }

    let desired_scale = (f64::from(desired_size.width) / f64::from(original_size.width))
        .max(f64::from(desired_size.height) / f64::from(original_size.height));
    let texture_scale = (f64::from(input.maximum_texture_size) / f64::from(original_size.width))
        .min(f64::from(input.maximum_texture_size) / f64::from(original_size.height));
    let max_pixels = input.display_image_byte_budget / DISPLAY_BYTES_PER_PIXEL;
    if max_pixels <= 0 {
        return None;
    }

    let original_pixels = f64::from(original_size.width) * f64::from(original_size.height);
    let budget_scale = (max_pixels as f64 / original_pixels).sqrt();
    let scale = desired_scale.min(texture_scale).min(budget_scale);
    if !scale.is_finite() || scale <= 0.0 {
        return None;
    }

    let candidate = Size {
        width: floor_scaled_dimension(original_size.width, scale),
        height: floor_scaled_dimension(original_size.height, scale),
    };
    if size_is_safe(candidate, input) {
        Some(candidate)
    } else {
        None
    }
}

fn decision(
    status: RustRasterDisplayBucketStatus,
    quality: i32,
    bucket_key: RustRasterDisplayBucketKey,
    refinement_required: bool,
    current_image_retained: bool,
) -> RustRasterDisplayBucketDecision {
    RustRasterDisplayBucketDecision {
        status,
        quality,
        bucket_key,
        refinement_required,
        current_image_retained,
    }
}

fn bucket_key(
    raster_size: Size,
    original_size: Size,
    input: RustRasterDisplayBucketInput,
) -> RustRasterDisplayBucketKey {
    RustRasterDisplayBucketKey {
        raster_width: raster_size.width,
        raster_height: raster_size.height,
        exact: raster_size.width >= original_size.width
            && raster_size.height >= original_size.height,
        maximum_texture_size: input.maximum_texture_size,
        display_image_byte_budget: input.display_image_byte_budget,
    }
}

fn empty_bucket_key(input: RustRasterDisplayBucketInput) -> RustRasterDisplayBucketKey {
    RustRasterDisplayBucketKey {
        raster_width: 0,
        raster_height: 0,
        exact: false,
        maximum_texture_size: input.maximum_texture_size,
        display_image_byte_budget: input.display_image_byte_budget,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const DISPLAY_IMAGE_QUALITY_FIRST_DISPLAY: i32 = 1;

    fn bucket_input(rotation_degrees: i32) -> RustRasterDisplayBucketInput {
        RustRasterDisplayBucketInput {
            original_width: 1000,
            original_height: 500,
            current_raster_width: 100,
            current_raster_height: 50,
            current_quality: DISPLAY_IMAGE_QUALITY_FIRST_DISPLAY,
            display_width: 300.0,
            display_height: 600.0,
            visible_x: 0.0,
            visible_y: 0.0,
            visible_width: 300.0,
            visible_height: 600.0,
            device_pixel_ratio: 1.0,
            rotation_degrees,
            maximum_texture_size: 4096,
            display_image_byte_budget: 64 * 1024 * 1024,
        }
    }

    fn assert_refines_to(rotation_degrees: i32, width: i32, height: i32) {
        let decision = rust_raster_display_bucket_decision(bucket_input(rotation_degrees));

        assert!(matches!(
            decision.status,
            RustRasterDisplayBucketStatus::RefinementNeeded
        ));
        assert_eq!(decision.quality, DISPLAY_IMAGE_QUALITY_EXACT);
        assert_eq!(decision.bucket_key.raster_width, width);
        assert_eq!(decision.bucket_key.raster_height, height);
        assert!(decision.refinement_required);
        assert!(decision.current_image_retained);
    }

    #[test]
    fn quarter_turn_rotation_swaps_axes_before_selecting_bucket_size() {
        assert_refines_to(90, 600, 300);
        assert_refines_to(450, 600, 300);
        assert_refines_to(-90, 600, 300);
    }

    #[test]
    fn non_quarter_turn_rotation_does_not_swap_bucket_axes() {
        assert_refines_to(45, 1000, 500);
    }
}
