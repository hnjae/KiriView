// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const SCAN_STEP_VIEWPORT_RATIO: f64 = 0.75;
const POSITION_EPSILON: f64 = 0.001;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy)]
    struct RustPointF {
        x: f64,
        y: f64,
    }

    #[derive(Clone, Copy)]
    struct RustSize {
        width: i32,
        height: i32,
    }

    #[derive(Clone, Copy)]
    struct RustSizeF {
        width: f64,
        height: f64,
    }

    #[derive(Clone, Copy)]
    struct RustRectF {
        x: f64,
        y: f64,
        width: f64,
        height: f64,
    }

    extern "Rust" {
        #[cxx_name = "rustImageViewportImageRect"]
        fn rust_image_viewport_image_rect(
            viewport_size: RustSizeF,
            display_size: RustSizeF,
        ) -> RustRectF;

        #[cxx_name = "rustImageViewportMaximumContentPosition"]
        fn rust_image_viewport_maximum_content_position(
            viewport_size: RustSizeF,
            image_rect: RustRectF,
        ) -> RustPointF;

        #[cxx_name = "rustImageViewportClampedContentPosition"]
        fn rust_image_viewport_clamped_content_position(
            viewport_size: RustSizeF,
            image_rect: RustRectF,
            content_position: RustPointF,
        ) -> RustPointF;

        #[cxx_name = "rustImageViewportPanPosition"]
        fn rust_image_viewport_pan_position(
            viewport_size: RustSizeF,
            image_rect: RustRectF,
            content_position: RustPointF,
            delta: RustPointF,
        ) -> RustPointF;

        #[cxx_name = "rustImageViewportNextZScanPosition"]
        fn rust_image_viewport_next_z_scan_position(
            viewport_size: RustSizeF,
            image_rect: RustRectF,
            content_position: RustPointF,
        ) -> RustPointF;

        #[cxx_name = "rustImageViewportPreviousZScanPosition"]
        fn rust_image_viewport_previous_z_scan_position(
            viewport_size: RustSizeF,
            image_rect: RustRectF,
            content_position: RustPointF,
        ) -> RustPointF;

        #[cxx_name = "rustImageViewportFinalZScanPosition"]
        fn rust_image_viewport_final_z_scan_position(
            viewport_size: RustSizeF,
            image_rect: RustRectF,
        ) -> RustPointF;

        #[cxx_name = "rustImageViewportPointInsideImage"]
        fn rust_image_viewport_point_inside_image(
            content_position: RustPointF,
            viewport_point: RustPointF,
            image_rect: RustRectF,
        ) -> bool;

        #[cxx_name = "rustImageViewportContentPositionForZoom"]
        fn rust_image_viewport_content_position_for_zoom(
            viewport_size: RustSizeF,
            current_display_size: RustSizeF,
            next_display_size: RustSizeF,
            content_position: RustPointF,
            viewport_anchor_point: RustPointF,
        ) -> RustPointF;

        #[cxx_name = "rustImageViewportDisplaySizeForZoom"]
        fn rust_image_viewport_display_size_for_zoom(
            image_size: RustSize,
            zoom_percent: f64,
            device_pixel_ratio: f64,
        ) -> RustSizeF;
    }
}

use ffi::{RustPointF, RustRectF, RustSize, RustSizeF};

fn normalized_length(length: f64) -> f64 {
    if length.is_finite() {
        length.max(0.0)
    } else {
        0.0
    }
}

fn normalized_size(size: RustSizeF) -> RustSizeF {
    RustSizeF {
        width: normalized_length(size.width),
        height: normalized_length(size.height),
    }
}

fn normalized_point(point: RustPointF, fallback: RustPointF) -> RustPointF {
    RustPointF {
        x: if point.x.is_finite() {
            point.x
        } else {
            fallback.x
        },
        y: if point.y.is_finite() {
            point.y
        } else {
            fallback.y
        },
    }
}

fn clamped_value(value: f64, minimum: f64, maximum: f64) -> f64 {
    let value = if value.is_finite() { value } else { minimum };
    value.clamp(minimum, maximum)
}

fn axis_pannable(maximum: f64) -> bool {
    maximum > POSITION_EPSILON
}

fn scan_step_length(viewport_length: f64) -> f64 {
    normalized_length(viewport_length) * SCAN_STEP_VIEWPORT_RATIO
}

fn next_axis_scan_position(position: f64, step: f64, maximum: f64) -> f64 {
    let current = clamped_value(position, 0.0, maximum);
    if !axis_pannable(maximum) || step <= POSITION_EPSILON || current >= maximum - POSITION_EPSILON
    {
        return current;
    }

    let next_position = (current / step).floor().mul_add(step, step);
    maximum.min(next_position)
}

fn previous_axis_scan_position(position: f64, step: f64, maximum: f64) -> f64 {
    let current = clamped_value(position, 0.0, maximum);
    if !axis_pannable(maximum) || step <= POSITION_EPSILON || current <= POSITION_EPSILON {
        return current;
    }

    let previous_position = ((current / step).ceil() - 1.0) * step;
    0.0_f64.max(previous_position)
}

fn rust_image_viewport_image_rect(viewport_size: RustSizeF, display_size: RustSizeF) -> RustRectF {
    let viewport = normalized_size(viewport_size);
    let display = normalized_size(display_size);
    RustRectF {
        x: 0.0_f64.max((viewport.width - display.width) / 2.0),
        y: 0.0_f64.max((viewport.height - display.height) / 2.0),
        width: display.width,
        height: display.height,
    }
}

fn rust_image_viewport_maximum_content_position(
    viewport_size: RustSizeF,
    image_rect: RustRectF,
) -> RustPointF {
    let viewport = normalized_size(viewport_size);
    RustPointF {
        x: 0.0_f64.max(viewport.width.max(image_rect.x + image_rect.width) - viewport.width),
        y: 0.0_f64.max(viewport.height.max(image_rect.y + image_rect.height) - viewport.height),
    }
}

fn rust_image_viewport_clamped_content_position(
    viewport_size: RustSizeF,
    image_rect: RustRectF,
    content_position: RustPointF,
) -> RustPointF {
    let maximum = rust_image_viewport_maximum_content_position(viewport_size, image_rect);
    RustPointF {
        x: clamped_value(content_position.x, 0.0, maximum.x),
        y: clamped_value(content_position.y, 0.0, maximum.y),
    }
}

fn rust_image_viewport_pan_position(
    viewport_size: RustSizeF,
    image_rect: RustRectF,
    content_position: RustPointF,
    delta: RustPointF,
) -> RustPointF {
    rust_image_viewport_clamped_content_position(
        viewport_size,
        image_rect,
        RustPointF {
            x: content_position.x + delta.x,
            y: content_position.y + delta.y,
        },
    )
}

fn rust_image_viewport_next_z_scan_position(
    viewport_size: RustSizeF,
    image_rect: RustRectF,
    content_position: RustPointF,
) -> RustPointF {
    let viewport = normalized_size(viewport_size);
    let maximum = rust_image_viewport_maximum_content_position(viewport, image_rect);
    let current =
        rust_image_viewport_clamped_content_position(viewport, image_rect, content_position);

    if axis_pannable(maximum.x) {
        let next_x =
            next_axis_scan_position(current.x, scan_step_length(viewport.width), maximum.x);
        if next_x != current.x {
            return RustPointF {
                x: next_x,
                y: current.y,
            };
        }
    }

    if axis_pannable(maximum.y) {
        let next_y =
            next_axis_scan_position(current.y, scan_step_length(viewport.height), maximum.y);
        if next_y != current.y {
            return RustPointF {
                x: if axis_pannable(maximum.x) {
                    0.0
                } else {
                    current.x
                },
                y: next_y,
            };
        }
    }

    current
}

fn rust_image_viewport_previous_z_scan_position(
    viewport_size: RustSizeF,
    image_rect: RustRectF,
    content_position: RustPointF,
) -> RustPointF {
    let viewport = normalized_size(viewport_size);
    let maximum = rust_image_viewport_maximum_content_position(viewport, image_rect);
    let current =
        rust_image_viewport_clamped_content_position(viewport, image_rect, content_position);

    if axis_pannable(maximum.x) {
        let previous_x =
            previous_axis_scan_position(current.x, scan_step_length(viewport.width), maximum.x);
        if previous_x != current.x {
            return RustPointF {
                x: previous_x,
                y: current.y,
            };
        }
    }

    if axis_pannable(maximum.y) {
        let previous_y =
            previous_axis_scan_position(current.y, scan_step_length(viewport.height), maximum.y);
        if previous_y != current.y {
            return RustPointF {
                x: if axis_pannable(maximum.x) {
                    maximum.x
                } else {
                    current.x
                },
                y: previous_y,
            };
        }
    }

    current
}

fn rust_image_viewport_final_z_scan_position(
    viewport_size: RustSizeF,
    image_rect: RustRectF,
) -> RustPointF {
    rust_image_viewport_maximum_content_position(viewport_size, image_rect)
}

fn rust_image_viewport_point_inside_image(
    content_position: RustPointF,
    viewport_point: RustPointF,
    image_rect: RustRectF,
) -> bool {
    if image_rect.width <= 0.0 || image_rect.height <= 0.0 {
        return false;
    }

    let x = content_position.x + viewport_point.x;
    let y = content_position.y + viewport_point.y;
    x >= image_rect.x
        && x <= image_rect.x + image_rect.width
        && y >= image_rect.y
        && y <= image_rect.y + image_rect.height
}

fn rust_image_viewport_content_position_for_zoom(
    viewport_size: RustSizeF,
    current_display_size: RustSizeF,
    next_display_size: RustSizeF,
    content_position: RustPointF,
    viewport_anchor_point: RustPointF,
) -> RustPointF {
    let viewport = normalized_size(viewport_size);
    let current_image_rect = rust_image_viewport_image_rect(viewport, current_display_size);
    let fallback_anchor = RustPointF {
        x: viewport.width / 2.0,
        y: viewport.height / 2.0,
    };
    let anchor = normalized_point(viewport_anchor_point, fallback_anchor);
    let clamped_anchor = RustPointF {
        x: clamped_value(anchor.x, 0.0, viewport.width),
        y: clamped_value(anchor.y, 0.0, viewport.height),
    };

    let anchor_image_x = content_position.x + clamped_anchor.x - current_image_rect.x;
    let anchor_image_y = content_position.y + clamped_anchor.y - current_image_rect.y;
    let anchor_ratio_x = if current_image_rect.width > 0.0 {
        clamped_value(anchor_image_x / current_image_rect.width, 0.0, 1.0)
    } else {
        0.5
    };
    let anchor_ratio_y = if current_image_rect.height > 0.0 {
        clamped_value(anchor_image_y / current_image_rect.height, 0.0, 1.0)
    } else {
        0.5
    };

    let next_image_rect = rust_image_viewport_image_rect(viewport, next_display_size);
    rust_image_viewport_clamped_content_position(
        viewport,
        next_image_rect,
        RustPointF {
            x: next_image_rect.x + anchor_ratio_x * next_image_rect.width - clamped_anchor.x,
            y: next_image_rect.y + anchor_ratio_y * next_image_rect.height - clamped_anchor.y,
        },
    )
}

fn rust_image_viewport_display_size_for_zoom(
    image_size: RustSize,
    zoom_percent: f64,
    device_pixel_ratio: f64,
) -> RustSizeF {
    if image_size.width <= 0
        || image_size.height <= 0
        || !zoom_percent.is_finite()
        || zoom_percent <= 0.0
        || !device_pixel_ratio.is_finite()
        || device_pixel_ratio <= 0.0
    {
        return RustSizeF {
            width: 0.0,
            height: 0.0,
        };
    }

    let scale = zoom_percent / (device_pixel_ratio * 100.0);
    RustSizeF {
        width: f64::from(image_size.width) * scale,
        height: f64::from(image_size.height) * scale,
    }
}
