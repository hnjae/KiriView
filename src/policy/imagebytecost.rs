// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pub(crate) const ESTIMATED_RGBA_BYTES_PER_PIXEL: i64 = 4;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    extern "Rust" {
        #[cxx_name = "rustEstimatedRgbaByteCost"]
        fn rust_estimated_rgba_byte_cost(width: i32, height: i32) -> i64;
    }
}

pub(crate) fn estimated_rgba_byte_cost(width: i32, height: i32) -> Option<i64> {
    if width <= 0 || height <= 0 {
        return Some(0);
    }

    let pixels = i64::from(width).checked_mul(i64::from(height))?;
    pixels.checked_mul(ESTIMATED_RGBA_BYTES_PER_PIXEL)
}

fn rust_estimated_rgba_byte_cost(width: i32, height: i32) -> i64 {
    estimated_rgba_byte_cost(width, height).unwrap_or(i64::MAX)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn estimated_rgba_byte_cost_handles_empty_positive_and_overflow_sizes() {
        assert_eq!(estimated_rgba_byte_cost(0, 10), Some(0));
        assert_eq!(estimated_rgba_byte_cost(-1, 10), Some(0));
        assert_eq!(estimated_rgba_byte_cost(10, 3), Some(120));
        assert_eq!(estimated_rgba_byte_cost(i32::MAX, i32::MAX), None);
        assert_eq!(rust_estimated_rgba_byte_cost(i32::MAX, i32::MAX), i64::MAX);
    }
}
