// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const DEFAULT_ANIMATION_FRAME_DELAY_MS: i32 = 100;
const MINIMUM_ANIMATION_FRAME_DELAY_MS: i32 = 10;
const MILLISECONDS_PER_SECOND: u64 = 1000;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    extern "Rust" {
        #[cxx_name = "rustNormalizedAnimationFrameDelay"]
        fn rust_normalized_animation_frame_delay(delay_ms: i32) -> i32;

        #[cxx_name = "rustHeifFrameDelay"]
        fn rust_heif_frame_delay(duration: u32, timescale: u32) -> i32;
    }
}

fn rust_normalized_animation_frame_delay(delay_ms: i32) -> i32 {
    if delay_ms < 0 {
        return DEFAULT_ANIMATION_FRAME_DELAY_MS;
    }

    delay_ms.max(MINIMUM_ANIMATION_FRAME_DELAY_MS)
}

fn rust_heif_frame_delay(duration: u32, timescale: u32) -> i32 {
    if duration == 0 || timescale == 0 {
        return 0;
    }

    let timescale = u64::from(timescale);
    let delay_ms = u64::from(duration)
        .saturating_mul(MILLISECONDS_PER_SECOND)
        .saturating_add(timescale - 1)
        / timescale;
    delay_ms.min(i32::MAX as u64) as i32
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn normalized_animation_frame_delay_uses_default_for_negative_delays() {
        assert_eq!(rust_normalized_animation_frame_delay(-1), 100);
    }

    #[test]
    fn normalized_animation_frame_delay_applies_minimum_to_short_delays() {
        assert_eq!(rust_normalized_animation_frame_delay(0), 10);
        assert_eq!(rust_normalized_animation_frame_delay(9), 10);
        assert_eq!(rust_normalized_animation_frame_delay(10), 10);
        assert_eq!(rust_normalized_animation_frame_delay(25), 25);
    }

    #[test]
    fn heif_frame_delay_rounds_up_duration_to_milliseconds() {
        assert_eq!(rust_heif_frame_delay(1, 24), 42);
        assert_eq!(rust_heif_frame_delay(3, 2), 1500);
    }

    #[test]
    fn heif_frame_delay_rejects_missing_duration_or_timescale() {
        assert_eq!(rust_heif_frame_delay(0, 24), 0);
        assert_eq!(rust_heif_frame_delay(1, 0), 0);
    }

    #[test]
    fn heif_frame_delay_clamps_to_timer_range() {
        assert_eq!(rust_heif_frame_delay(u32::MAX, 1), i32::MAX);
    }
}
