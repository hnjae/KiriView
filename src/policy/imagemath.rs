// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pub(crate) fn display_size_for_zoom_percent(
    image_width: i32,
    image_height: i32,
    zoom_percent: f64,
    device_pixel_ratio: f64,
) -> (f64, f64) {
    if image_width <= 0
        || image_height <= 0
        || !zoom_percent.is_finite()
        || zoom_percent <= 0.0
        || !device_pixel_ratio.is_finite()
        || device_pixel_ratio <= 0.0
    {
        return (0.0, 0.0);
    }

    let scale = zoom_percent / (device_pixel_ratio * 100.0);
    (
        f64::from(image_width) * scale,
        f64::from(image_height) * scale,
    )
}
