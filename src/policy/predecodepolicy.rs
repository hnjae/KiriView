// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[path = "predecodepolicy/cache_retention.rs"]
mod cache_retention;
#[path = "predecodepolicy/config.rs"]
mod config;
#[path = "predecodepolicy/load_queue.rs"]
mod load_queue;
#[path = "predecodepolicy/momentum.rs"]
mod momentum;
#[path = "predecodepolicy/schedule.rs"]
mod schedule;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeSourceProfile {
        neutral_previous_image_count: usize,
        neutral_next_image_count: usize,
        biased_direction_image_count: usize,
        parallel_limit: usize,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustPredecodeMomentumMode {
        Neutral = 0,
        NextBiased = 1,
        PrevBiased = 2,
        ScrubbingNext = 3,
        ScrubbingPrev = 4,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustPredecodeMomentumDirection {
        None = 0,
        Previous = 1,
        Next = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodePolicyInput {
        source_profile: RustPredecodeSourceProfile,
        momentum_mode: RustPredecodeMomentumMode,
        power_saver_enabled: bool,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustPredecodeSchedulePlan {
        parallel_limit: usize,
        target_indices: Vec<usize>,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeMomentumState {
        last_page_index: i32,
        last_navigation_msec: i64,
        same_direction_move_count: i32,
        last_direction: RustPredecodeMomentumDirection,
        mode: RustPredecodeMomentumMode,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeMomentumUpdateInput {
        state: RustPredecodeMomentumState,
        page_index: i32,
        monotonic_msec: i64,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeQueuedLoadPlan {
        found: bool,
        index: usize,
        discard_count: usize,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeCachedImageState {
        current_displayed: bool,
        recent_displayed: bool,
        current_displayed_priority: usize,
        recent_displayed_priority: usize,
        window_priority: usize,
        byte_cost: i64,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeWindowLoadState {
        displayed: bool,
        cached: bool,
        in_flight: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeQueuedLoadState {
        valid: bool,
        in_window: bool,
        cached: bool,
        in_flight: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustPredecodeDebounceMsec"]
        fn rust_predecode_debounce_msec() -> i32;

        #[cxx_name = "rustPredecodeNeutralRefreshMsec"]
        fn rust_predecode_neutral_refresh_msec() -> i32;

        #[cxx_name = "rustPredecodeRetainedCachedImageIndices"]
        fn rust_predecode_retained_cached_image_indices(
            states: Vec<RustPredecodeCachedImageState>,
            window_count: usize,
            byte_budget: i64,
        ) -> Vec<usize>;

        #[cxx_name = "rustPredecodeMissingWindowLoadIndices"]
        fn rust_predecode_missing_window_load_indices(
            states: Vec<RustPredecodeWindowLoadState>,
        ) -> Vec<usize>;

        #[cxx_name = "rustPredecodeSchedulePlan"]
        fn rust_predecode_schedule_plan(
            candidate_count: usize,
            current_index_found: bool,
            current_index: usize,
            input: RustPredecodePolicyInput,
        ) -> RustPredecodeSchedulePlan;

        #[cxx_name = "rustPredecodeUpdatedMomentumState"]
        fn rust_predecode_updated_momentum_state(
            input: RustPredecodeMomentumUpdateInput,
        ) -> RustPredecodeMomentumState;

        #[cxx_name = "rustPredecodeNextQueuedLoadPlan"]
        fn rust_predecode_next_queued_load_plan(
            states: Vec<RustPredecodeQueuedLoadState>,
        ) -> RustPredecodeQueuedLoadPlan;
    }
}

pub(super) use ffi::{
    RustPredecodeCachedImageState, RustPredecodeMomentumDirection, RustPredecodeMomentumMode,
    RustPredecodeMomentumState, RustPredecodeMomentumUpdateInput, RustPredecodePolicyInput,
    RustPredecodeQueuedLoadPlan, RustPredecodeQueuedLoadState, RustPredecodeSchedulePlan,
    RustPredecodeSourceProfile, RustPredecodeWindowLoadState,
};

fn rust_predecode_debounce_msec() -> i32 {
    config::debounce_msec()
}

fn rust_predecode_neutral_refresh_msec() -> i32 {
    config::neutral_refresh_msec()
}

fn rust_predecode_retained_cached_image_indices(
    states: Vec<RustPredecodeCachedImageState>,
    window_count: usize,
    byte_budget: i64,
) -> Vec<usize> {
    cache_retention::retained_cached_image_indices(states, window_count, byte_budget)
}

fn rust_predecode_missing_window_load_indices(
    states: Vec<RustPredecodeWindowLoadState>,
) -> Vec<usize> {
    load_queue::missing_window_load_indices(states)
}

fn rust_predecode_schedule_plan(
    candidate_count: usize,
    current_index_found: bool,
    current_index: usize,
    input: RustPredecodePolicyInput,
) -> RustPredecodeSchedulePlan {
    schedule::schedule_plan(candidate_count, current_index_found, current_index, input)
}

fn rust_predecode_updated_momentum_state(
    input: RustPredecodeMomentumUpdateInput,
) -> RustPredecodeMomentumState {
    momentum::updated_momentum_state(input)
}

fn rust_predecode_next_queued_load_plan(
    states: Vec<RustPredecodeQueuedLoadState>,
) -> RustPredecodeQueuedLoadPlan {
    load_queue::next_queued_load_plan(states)
}
