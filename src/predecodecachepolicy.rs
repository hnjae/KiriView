// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const PREDECODE_PREFERRED_BYTE_BUDGET: i64 = 1024 * 1024 * 1024;
const PREDECODE_SYSTEM_MEMORY_DIVISOR: i64 = 8;
const PREDECODE_PREVIOUS_IMAGE_COUNT: usize = 2;
const PREDECODE_NEXT_IMAGE_COUNT: usize = 4;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeQueuedLoadPlan {
        found: bool,
        index: usize,
        discard_count: usize,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeCachedImageState {
        displayed: bool,
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
    }

    extern "Rust" {
        #[cxx_name = "rustPredecodePreferredByteBudget"]
        fn rust_predecode_preferred_byte_budget() -> i64;

        #[cxx_name = "rustPredecodeByteBudgetForSystemMemory"]
        fn rust_predecode_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64;

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

        #[cxx_name = "rustPredecodeWindowImageIndices"]
        fn rust_predecode_window_image_indices(matches_current: Vec<u8>) -> Vec<usize>;

        #[cxx_name = "rustPredecodeNextQueuedLoadPlan"]
        fn rust_predecode_next_queued_load_plan(
            states: Vec<RustPredecodeQueuedLoadState>,
        ) -> RustPredecodeQueuedLoadPlan;
    }
}

use ffi::{
    RustPredecodeCachedImageState, RustPredecodeQueuedLoadPlan, RustPredecodeQueuedLoadState,
    RustPredecodeWindowLoadState,
};

#[derive(Clone, Copy)]
struct RetainedCachedImage {
    original_index: usize,
    displayed: bool,
    window_priority: usize,
    byte_cost: i64,
}

fn rust_predecode_preferred_byte_budget() -> i64 {
    PREDECODE_PREFERRED_BYTE_BUDGET
}

fn rust_predecode_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64 {
    crate::cachebudget::system_memory_capped_byte_budget(
        PREDECODE_PREFERRED_BYTE_BUDGET,
        system_memory_byte_size,
        PREDECODE_SYSTEM_MEMORY_DIVISOR,
    )
}

fn rust_predecode_retained_cached_image_indices(
    states: Vec<RustPredecodeCachedImageState>,
    window_count: usize,
    byte_budget: i64,
) -> Vec<usize> {
    if byte_budget <= 0 {
        return Vec::new();
    }

    let mut images: Vec<RetainedCachedImage> = states
        .into_iter()
        .enumerate()
        .filter_map(|(original_index, state)| {
            if (!state.displayed && state.window_priority >= window_count) || state.byte_cost <= 0 {
                return None;
            }

            Some(RetainedCachedImage {
                original_index,
                displayed: state.displayed,
                window_priority: state.window_priority,
                byte_cost: state.byte_cost,
            })
        })
        .collect();

    let mut displayed_images: Vec<RetainedCachedImage> = images
        .iter()
        .copied()
        .filter(|image| image.displayed)
        .collect();
    displayed_images.sort_by(|left, right| {
        left.window_priority
            .cmp(&right.window_priority)
            .then(left.original_index.cmp(&right.original_index))
    });

    let displayed_byte_cost = displayed_images
        .iter()
        .fold(0_i64, |total, image| total.saturating_add(image.byte_cost));
    let adjacent_byte_budget = byte_budget.saturating_sub(displayed_byte_cost);

    images.retain(|image| !image.displayed);
    images.sort_by(|left, right| {
        left.window_priority
            .cmp(&right.window_priority)
            .then(left.original_index.cmp(&right.original_index))
    });

    let mut total_byte_cost = images
        .iter()
        .fold(0_i64, |total, image| total.saturating_add(image.byte_cost));
    while total_byte_cost > adjacent_byte_budget && !images.is_empty() {
        if let Some(image) = images.pop() {
            total_byte_cost = total_byte_cost.saturating_sub(image.byte_cost);
        }
    }

    displayed_images.extend(images);
    displayed_images
        .into_iter()
        .map(|image| image.original_index)
        .collect()
}

fn rust_predecode_missing_window_load_indices(
    states: Vec<RustPredecodeWindowLoadState>,
) -> Vec<usize> {
    states
        .into_iter()
        .enumerate()
        .filter_map(|(index, state)| {
            if state.displayed || state.cached || state.in_flight {
                return None;
            }

            Some(index)
        })
        .collect()
}

fn rust_predecode_window_image_indices(matches_current: Vec<u8>) -> Vec<usize> {
    let mut indices = Vec::new();
    let candidate_count = matches_current.len();
    let Some(current_index) = matches_current.iter().position(|flag| *flag != 0) else {
        return indices;
    };

    let append_offset = |indices: &mut Vec<usize>, offset: isize| {
        let Some(target_index) = current_index.checked_add_signed(offset) else {
            return;
        };
        if target_index >= candidate_count {
            return;
        }

        indices.push(target_index);
    };

    append_offset(&mut indices, 0);
    for offset in 1..=PREDECODE_NEXT_IMAGE_COUNT {
        append_offset(&mut indices, offset as isize);
        if offset <= PREDECODE_PREVIOUS_IMAGE_COUNT {
            append_offset(&mut indices, -(offset as isize));
        }
    }
    indices
}

fn rust_predecode_next_queued_load_plan(
    states: Vec<RustPredecodeQueuedLoadState>,
) -> RustPredecodeQueuedLoadPlan {
    let state_count = states.len();
    for (index, state) in states.into_iter().enumerate() {
        if state.valid && state.in_window && !state.cached {
            return queued_load_plan(index);
        }
    }

    missing_queued_load_plan(state_count)
}

fn queued_load_plan(index: usize) -> RustPredecodeQueuedLoadPlan {
    RustPredecodeQueuedLoadPlan {
        found: true,
        index,
        discard_count: index.saturating_add(1),
    }
}

fn missing_queued_load_plan(discard_count: usize) -> RustPredecodeQueuedLoadPlan {
    RustPredecodeQueuedLoadPlan {
        found: false,
        index: 0,
        discard_count,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn byte_budget_uses_preferred_limit_and_system_memory_cap() {
        let preferred = rust_predecode_preferred_byte_budget();

        assert_eq!(rust_predecode_byte_budget_for_system_memory(0), preferred);
        assert_eq!(
            rust_predecode_byte_budget_for_system_memory(preferred),
            preferred / PREDECODE_SYSTEM_MEMORY_DIVISOR
        );
        assert_eq!(
            rust_predecode_byte_budget_for_system_memory(preferred * 16),
            preferred
        );
    }

    #[test]
    fn retained_cached_image_indices_drop_images_outside_window_and_sort_by_priority() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    cached_image_state(2, 10),
                    cached_image_state(3, 10),
                    cached_image_state(0, 10),
                    cached_image_state(1, 10),
                ],
                3,
                100,
            ),
            vec![2, 3, 0]
        );
    }

    #[test]
    fn retained_cached_image_indices_trim_lowest_priority_images_to_budget() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    cached_image_state(2, 50),
                    cached_image_state(0, 60),
                    cached_image_state(1, 60),
                ],
                3,
                120,
            ),
            vec![1, 2]
        );
    }

    #[test]
    fn retained_cached_image_indices_preserve_priority_prefix_when_trimming() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    cached_image_state(0, 100),
                    cached_image_state(1, 100),
                    cached_image_state(2, 1),
                ],
                3,
                101,
            ),
            vec![0]
        );
    }

    #[test]
    fn retained_cached_image_indices_keep_displayed_images_before_adjacent_budget() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    cached_image_state(0, 60),
                    displayed_cached_image_state(1, 90),
                    cached_image_state(2, 60),
                    displayed_cached_image_state(3, 90),
                ],
                4,
                160,
            ),
            vec![1, 3]
        );
    }

    #[test]
    fn retained_cached_image_indices_keep_displayed_images_without_window() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![
                    displayed_cached_image_state(usize::MAX, 90),
                    cached_image_state(0, 10),
                ],
                0,
                80,
            ),
            vec![0]
        );
    }

    #[test]
    fn missing_window_load_indices_skip_displayed_cached_and_in_flight_urls() {
        assert_eq!(
            rust_predecode_missing_window_load_indices(vec![
                window_load_state(true, false, false),
                window_load_state(false, true, false),
                window_load_state(false, false, true),
                window_load_state(false, false, false),
                window_load_state(false, false, false),
            ]),
            vec![3, 4]
        );
    }

    #[test]
    fn window_image_indices_prioritize_current_then_adjacent_pages() {
        assert_eq!(
            rust_predecode_window_image_indices(vec![0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
            vec![5, 6, 4, 7, 3, 8, 9]
        );
    }

    #[test]
    fn window_image_indices_ignore_missing_current() {
        assert_eq!(
            rust_predecode_window_image_indices(vec![]),
            Vec::<usize>::new()
        );
        assert_eq!(
            rust_predecode_window_image_indices(vec![0, 0, 0]),
            Vec::<usize>::new()
        );
    }

    #[test]
    fn next_queued_load_plan_skips_invalid_stale_and_cached_requests() {
        assert_eq!(
            rust_predecode_next_queued_load_plan(vec![
                queued_load_state(false, true, false),
                queued_load_state(true, false, false),
                queued_load_state(true, true, true),
                queued_load_state(true, true, false),
            ]),
            queued_load_plan(3)
        );
        assert_eq!(
            rust_predecode_next_queued_load_plan(vec![
                queued_load_state(false, true, false),
                queued_load_state(true, false, false),
            ]),
            missing_queued_load_plan(2)
        );
    }

    fn window_load_state(
        displayed: bool,
        cached: bool,
        in_flight: bool,
    ) -> RustPredecodeWindowLoadState {
        RustPredecodeWindowLoadState {
            displayed,
            cached,
            in_flight,
        }
    }

    fn cached_image_state(window_priority: usize, byte_cost: i64) -> RustPredecodeCachedImageState {
        RustPredecodeCachedImageState {
            displayed: false,
            window_priority,
            byte_cost,
        }
    }

    fn displayed_cached_image_state(
        window_priority: usize,
        byte_cost: i64,
    ) -> RustPredecodeCachedImageState {
        RustPredecodeCachedImageState {
            displayed: true,
            window_priority,
            byte_cost,
        }
    }

    fn queued_load_state(
        valid: bool,
        in_window: bool,
        cached: bool,
    ) -> RustPredecodeQueuedLoadState {
        RustPredecodeQueuedLoadState {
            valid,
            in_window,
            cached,
        }
    }
}
