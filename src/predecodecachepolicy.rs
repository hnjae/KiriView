// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const PREDECODE_PREFERRED_BYTE_BUDGET: i64 = 1024 * 1024 * 1024;
const PREDECODE_SYSTEM_MEMORY_DIVISOR: i64 = 8;

use crate::imagebytecost::system_memory_capped_byte_budget;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeCacheableByteCost {
        cacheable: bool,
        byte_cost: i64,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPredecodeQueuedLoadPlan {
        found: bool,
        index: usize,
        discard_count: usize,
    }

    extern "Rust" {
        #[cxx_name = "rustPredecodePreferredByteBudget"]
        fn rust_predecode_preferred_byte_budget() -> i64;

        #[cxx_name = "rustPredecodeByteBudgetForSystemMemory"]
        fn rust_predecode_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64;

        #[cxx_name = "rustPredecodeCacheableByteCost"]
        fn rust_predecode_cacheable_byte_cost(
            static_image_byte_cost: i64,
            byte_budget: i64,
        ) -> RustPredecodeCacheableByteCost;

        #[cxx_name = "rustPredecodeRetainedCachedImageIndices"]
        fn rust_predecode_retained_cached_image_indices(
            window_priorities: Vec<usize>,
            byte_costs: Vec<i64>,
            window_count: usize,
            byte_budget: i64,
        ) -> Vec<usize>;

        #[cxx_name = "rustPredecodeMissingWindowLoadIndices"]
        fn rust_predecode_missing_window_load_indices(
            displayed_matches: Vec<u8>,
            cached: Vec<u8>,
            in_flight: Vec<u8>,
        ) -> Vec<usize>;

        #[cxx_name = "rustPredecodeNextQueuedLoadPlan"]
        fn rust_predecode_next_queued_load_plan(
            valid: Vec<u8>,
            in_window: Vec<u8>,
            cached: Vec<u8>,
        ) -> RustPredecodeQueuedLoadPlan;
    }
}

use ffi::{RustPredecodeCacheableByteCost, RustPredecodeQueuedLoadPlan};

#[derive(Clone, Copy)]
struct RetainedCachedImage {
    original_index: usize,
    window_priority: usize,
    byte_cost: i64,
}

fn rust_predecode_preferred_byte_budget() -> i64 {
    PREDECODE_PREFERRED_BYTE_BUDGET
}

fn rust_predecode_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64 {
    system_memory_capped_byte_budget(
        PREDECODE_PREFERRED_BYTE_BUDGET,
        system_memory_byte_size,
        PREDECODE_SYSTEM_MEMORY_DIVISOR,
    )
}

fn rust_predecode_cacheable_byte_cost(
    static_image_byte_cost: i64,
    byte_budget: i64,
) -> RustPredecodeCacheableByteCost {
    if static_image_byte_cost <= 0 || static_image_byte_cost > byte_budget {
        return RustPredecodeCacheableByteCost {
            cacheable: false,
            byte_cost: 0,
        };
    }

    RustPredecodeCacheableByteCost {
        cacheable: true,
        byte_cost: static_image_byte_cost,
    }
}

fn rust_predecode_retained_cached_image_indices(
    window_priorities: Vec<usize>,
    byte_costs: Vec<i64>,
    window_count: usize,
    byte_budget: i64,
) -> Vec<usize> {
    if window_count == 0 || byte_budget <= 0 {
        return Vec::new();
    }

    let mut images: Vec<RetainedCachedImage> = window_priorities
        .into_iter()
        .zip(byte_costs)
        .enumerate()
        .filter_map(|(original_index, (window_priority, byte_cost))| {
            if window_priority >= window_count || byte_cost <= 0 {
                return None;
            }

            Some(RetainedCachedImage {
                original_index,
                window_priority,
                byte_cost,
            })
        })
        .collect();
    images.sort_by(|left, right| left.window_priority.cmp(&right.window_priority));

    let mut total_byte_cost = images
        .iter()
        .fold(0_i64, |total, image| total.saturating_add(image.byte_cost));
    while total_byte_cost > byte_budget && !images.is_empty() {
        if let Some(image) = images.pop() {
            total_byte_cost = total_byte_cost.saturating_sub(image.byte_cost);
        }
    }

    images
        .into_iter()
        .map(|image| image.original_index)
        .collect()
}

fn rust_predecode_missing_window_load_indices(
    displayed_matches: Vec<u8>,
    cached: Vec<u8>,
    in_flight: Vec<u8>,
) -> Vec<usize> {
    let flag_count = displayed_matches
        .len()
        .min(cached.len())
        .min(in_flight.len());
    (0..flag_count)
        .filter(|&index| {
            !flag_at(&displayed_matches, index)
                && !flag_at(&cached, index)
                && !flag_at(&in_flight, index)
        })
        .collect()
}

fn rust_predecode_next_queued_load_plan(
    valid: Vec<u8>,
    in_window: Vec<u8>,
    cached: Vec<u8>,
) -> RustPredecodeQueuedLoadPlan {
    let flag_count = valid.len().min(in_window.len()).min(cached.len());
    for index in 0..flag_count {
        if flag_at(&valid, index) && flag_at(&in_window, index) && !flag_at(&cached, index) {
            return queued_load_plan(index);
        }
    }

    missing_queued_load_plan(flag_count)
}

fn flag_at(flags: &[u8], index: usize) -> bool {
    flags.get(index).copied().unwrap_or(0) != 0
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
    fn cacheable_byte_cost_rejects_invalid_or_oversized_images() {
        assert_eq!(
            rust_predecode_cacheable_byte_cost(10, 10),
            RustPredecodeCacheableByteCost {
                cacheable: true,
                byte_cost: 10,
            }
        );
        assert_eq!(
            rust_predecode_cacheable_byte_cost(0, 10),
            RustPredecodeCacheableByteCost {
                cacheable: false,
                byte_cost: 0,
            }
        );
        assert_eq!(
            rust_predecode_cacheable_byte_cost(11, 10),
            RustPredecodeCacheableByteCost {
                cacheable: false,
                byte_cost: 0,
            }
        );
    }

    #[test]
    fn retained_cached_image_indices_drop_images_outside_window_and_sort_by_priority() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(
                vec![2, 3, 0, 1],
                vec![10, 10, 10, 10],
                3,
                100,
            ),
            vec![2, 3, 0]
        );
    }

    #[test]
    fn retained_cached_image_indices_trim_lowest_priority_images_to_budget() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(vec![2, 0, 1], vec![50, 60, 60], 3, 120,),
            vec![1, 2]
        );
    }

    #[test]
    fn retained_cached_image_indices_preserve_priority_prefix_when_trimming() {
        assert_eq!(
            rust_predecode_retained_cached_image_indices(vec![0, 1, 2], vec![100, 100, 1], 3, 101,),
            vec![0]
        );
    }

    #[test]
    fn missing_window_load_indices_skip_displayed_cached_and_in_flight_urls() {
        assert_eq!(
            rust_predecode_missing_window_load_indices(
                vec![1, 0, 0, 0, 0],
                vec![0, 1, 0, 0, 0],
                vec![0, 0, 1, 0, 0],
            ),
            vec![3, 4]
        );
    }

    #[test]
    fn next_queued_load_plan_skips_invalid_stale_and_cached_requests() {
        assert_eq!(
            rust_predecode_next_queued_load_plan(
                vec![0, 1, 1, 1],
                vec![1, 0, 1, 1],
                vec![0, 0, 1, 0],
            ),
            queued_load_plan(3)
        );
        assert_eq!(
            rust_predecode_next_queued_load_plan(vec![0, 1], vec![1, 0], vec![0, 0],),
            missing_queued_load_plan(2)
        );
    }
}
