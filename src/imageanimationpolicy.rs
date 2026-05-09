// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const DEFAULT_ANIMATION_FRAME_DELAY_MS: i32 = 100;
const MINIMUM_ANIMATION_FRAME_DELAY_MS: i32 = 10;
const MILLISECONDS_PER_SECOND: u64 = 1000;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustAnimationLoopState {
        loop_count: i32,
        completed_loops: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustAnimationLoopAdvance {
        should_continue: bool,
        completed_loops: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustDecodedAnimationAdvance {
        frame_available: bool,
        frame_index: usize,
        completed_loops: i32,
        schedule_next_frame: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustNormalizedAnimationFrameDelay"]
        fn rust_normalized_animation_frame_delay(delay_ms: i32) -> i32;

        #[cxx_name = "rustHeifFrameDelay"]
        fn rust_heif_frame_delay(duration: u32, timescale: u32) -> i32;

        #[cxx_name = "rustAnimationHasRemainingLoops"]
        fn rust_animation_has_remaining_loops(state: RustAnimationLoopState) -> bool;

        #[cxx_name = "rustAnimationAdvanceLoop"]
        fn rust_animation_advance_loop(state: RustAnimationLoopState) -> RustAnimationLoopAdvance;

        #[cxx_name = "rustDecodedAnimationAdvance"]
        fn rust_decoded_animation_advance(
            frame_count: usize,
            frame_index: usize,
            state: RustAnimationLoopState,
        ) -> RustDecodedAnimationAdvance;
    }
}

use ffi::{RustAnimationLoopAdvance, RustAnimationLoopState, RustDecodedAnimationAdvance};

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

fn rust_animation_has_remaining_loops(state: RustAnimationLoopState) -> bool {
    state.loop_count < 0 || state.completed_loops < state.loop_count
}

fn rust_animation_advance_loop(state: RustAnimationLoopState) -> RustAnimationLoopAdvance {
    if !rust_animation_has_remaining_loops(state) {
        return RustAnimationLoopAdvance {
            should_continue: false,
            completed_loops: state.completed_loops,
        };
    }

    RustAnimationLoopAdvance {
        should_continue: true,
        completed_loops: state.completed_loops.saturating_add(1),
    }
}

fn rust_decoded_animation_advance(
    frame_count: usize,
    frame_index: usize,
    state: RustAnimationLoopState,
) -> RustDecodedAnimationAdvance {
    if frame_count == 0 || frame_index >= frame_count {
        return stopped_decoded_animation_advance(state.completed_loops);
    }

    let next_frame_index = frame_index + 1;
    let (frame_index, completed_loops) = if next_frame_index >= frame_count {
        let loop_advance = rust_animation_advance_loop(state);
        if !loop_advance.should_continue {
            return stopped_decoded_animation_advance(loop_advance.completed_loops);
        }

        (0, loop_advance.completed_loops)
    } else {
        (next_frame_index, state.completed_loops)
    };

    let next_state = RustAnimationLoopState {
        loop_count: state.loop_count,
        completed_loops,
    };
    RustDecodedAnimationAdvance {
        frame_available: true,
        frame_index,
        completed_loops,
        schedule_next_frame: frame_index + 1 < frame_count
            || rust_animation_has_remaining_loops(next_state),
    }
}

fn stopped_decoded_animation_advance(completed_loops: i32) -> RustDecodedAnimationAdvance {
    RustDecodedAnimationAdvance {
        frame_available: false,
        frame_index: 0,
        completed_loops,
        schedule_next_frame: false,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn loop_state(loop_count: i32, completed_loops: i32) -> RustAnimationLoopState {
        RustAnimationLoopState {
            loop_count,
            completed_loops,
        }
    }

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

    #[test]
    fn loop_policy_tracks_finite_and_infinite_remaining_loops() {
        assert!(rust_animation_has_remaining_loops(loop_state(-1, 99)));
        assert!(rust_animation_has_remaining_loops(loop_state(2, 1)));
        assert!(!rust_animation_has_remaining_loops(loop_state(2, 2)));
    }

    #[test]
    fn loop_advance_increments_only_when_more_loops_are_available() {
        let advance = rust_animation_advance_loop(loop_state(2, 1));
        assert!(advance.should_continue);
        assert_eq!(advance.completed_loops, 2);

        let stop = rust_animation_advance_loop(loop_state(2, 2));
        assert!(!stop.should_continue);
        assert_eq!(stop.completed_loops, 2);
    }

    #[test]
    fn decoded_animation_advance_steps_to_next_frame() {
        let advance = rust_decoded_animation_advance(3, 0, loop_state(0, 0));

        assert!(advance.frame_available);
        assert_eq!(advance.frame_index, 1);
        assert_eq!(advance.completed_loops, 0);
        assert!(advance.schedule_next_frame);
    }

    #[test]
    fn decoded_animation_advance_loops_after_last_frame_when_available() {
        let advance = rust_decoded_animation_advance(2, 1, loop_state(1, 0));

        assert!(advance.frame_available);
        assert_eq!(advance.frame_index, 0);
        assert_eq!(advance.completed_loops, 1);
        assert!(advance.schedule_next_frame);
    }

    #[test]
    fn decoded_animation_advance_stops_after_last_frame_without_remaining_loops() {
        let advance = rust_decoded_animation_advance(2, 1, loop_state(0, 0));

        assert!(!advance.frame_available);
        assert_eq!(advance.completed_loops, 0);
        assert!(!advance.schedule_next_frame);
    }

    #[test]
    fn decoded_animation_advance_rejects_missing_or_invalid_frame_state() {
        assert!(!rust_decoded_animation_advance(0, 0, loop_state(0, 0)).frame_available);
        assert!(!rust_decoded_animation_advance(2, 2, loop_state(0, 0)).frame_available);
    }
}
