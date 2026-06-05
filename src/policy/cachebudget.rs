// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustLruCacheRetentionEntry {
        byte_cost: i64,
        last_use: u64,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustLruCacheRetainedEntry {
        original_index: usize,
        byte_cost: i64,
    }

    extern "Rust" {
        #[cxx_name = "rustLruCacheRetentionPlan"]
        fn rust_lru_cache_retention_plan(
            entries: Vec<RustLruCacheRetentionEntry>,
            byte_budget: i64,
        ) -> Vec<RustLruCacheRetainedEntry>;

        #[cxx_name = "rustDisplayImageCacheByteBudgetForSystemMemory"]
        fn rust_display_image_cache_byte_budget_for_system_memory(
            system_memory_byte_size: i64,
            preferred_byte_budget: i64,
        ) -> i64;

        #[cxx_name = "rustPredecodeCachePreferredByteBudget"]
        fn rust_predecode_cache_preferred_byte_budget() -> i64;

        #[cxx_name = "rustPredecodeCacheByteBudgetForSystemMemory"]
        fn rust_predecode_cache_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64;

        #[cxx_name = "rustThumbnailCachePreferredByteBudget"]
        fn rust_thumbnail_cache_preferred_byte_budget() -> i64;

        #[cxx_name = "rustThumbnailCacheByteBudgetForSystemMemory"]
        fn rust_thumbnail_cache_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64;
    }
}

const DISPLAY_IMAGE_CACHE_SYSTEM_MEMORY_DIVISOR: i64 = 16;
const PREDECODE_CACHE_PREFERRED_BYTE_BUDGET: i64 = 1024 * 1024 * 1024;
const PREDECODE_CACHE_SYSTEM_MEMORY_DIVISOR: i64 = 8;
const THUMBNAIL_CACHE_PREFERRED_BYTE_BUDGET: i64 = 64 * 1024 * 1024;
const THUMBNAIL_CACHE_SYSTEM_MEMORY_DIVISOR: i64 = 64;

#[derive(Clone, Copy)]
struct IndexedLruCacheRetentionEntry {
    original_index: usize,
    byte_cost: i64,
    last_use: u64,
}

pub(crate) fn system_memory_capped_byte_budget(
    preferred_byte_budget: i64,
    system_memory_byte_size: i64,
    memory_divisor: i64,
) -> i64 {
    if preferred_byte_budget <= 0 {
        return 0;
    }
    if system_memory_byte_size <= 0 || memory_divisor <= 0 {
        return preferred_byte_budget;
    }

    preferred_byte_budget.min(system_memory_byte_size / memory_divisor)
}

use ffi::{RustLruCacheRetainedEntry, RustLruCacheRetentionEntry};

fn rust_lru_cache_retention_plan(
    entries: Vec<RustLruCacheRetentionEntry>,
    byte_budget: i64,
) -> Vec<RustLruCacheRetainedEntry> {
    lru_cache_retention_plan(entries, byte_budget)
}

fn rust_display_image_cache_byte_budget_for_system_memory(
    system_memory_byte_size: i64,
    preferred_byte_budget: i64,
) -> i64 {
    display_image_cache_byte_budget_for_system_memory(
        system_memory_byte_size,
        preferred_byte_budget,
    )
}

fn rust_predecode_cache_preferred_byte_budget() -> i64 {
    predecode_cache_preferred_byte_budget()
}

fn rust_predecode_cache_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64 {
    predecode_cache_byte_budget_for_system_memory(system_memory_byte_size)
}

fn rust_thumbnail_cache_preferred_byte_budget() -> i64 {
    thumbnail_cache_preferred_byte_budget()
}

fn rust_thumbnail_cache_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64 {
    thumbnail_cache_byte_budget_for_system_memory(system_memory_byte_size)
}

pub(crate) fn lru_cache_retention_plan(
    entries: Vec<RustLruCacheRetentionEntry>,
    byte_budget: i64,
) -> Vec<RustLruCacheRetainedEntry> {
    if byte_budget <= 0 {
        return Vec::new();
    }

    let mut entries: Vec<IndexedLruCacheRetentionEntry> = entries
        .into_iter()
        .enumerate()
        .filter_map(|(original_index, entry)| {
            if entry.byte_cost <= 0 {
                return None;
            }

            Some(IndexedLruCacheRetentionEntry {
                original_index,
                byte_cost: entry.byte_cost,
                last_use: entry.last_use,
            })
        })
        .collect();
    let mut total_byte_cost = entries
        .iter()
        .fold(0_i64, |total, entry| total.saturating_add(entry.byte_cost));

    while total_byte_cost > byte_budget && !entries.is_empty() {
        let Some(oldest_index) = entries
            .iter()
            .enumerate()
            .min_by_key(|(_, entry)| entry.last_use)
            .map(|(index, _)| index)
        else {
            break;
        };
        let removed = entries.remove(oldest_index);
        total_byte_cost = total_byte_cost.saturating_sub(removed.byte_cost);
    }

    entries
        .into_iter()
        .map(|entry| RustLruCacheRetainedEntry {
            original_index: entry.original_index,
            byte_cost: entry.byte_cost,
        })
        .collect()
}

pub(crate) fn display_image_cache_byte_budget_for_system_memory(
    system_memory_byte_size: i64,
    preferred_byte_budget: i64,
) -> i64 {
    system_memory_capped_byte_budget(
        preferred_byte_budget,
        system_memory_byte_size,
        DISPLAY_IMAGE_CACHE_SYSTEM_MEMORY_DIVISOR,
    )
}

pub(crate) fn predecode_cache_preferred_byte_budget() -> i64 {
    PREDECODE_CACHE_PREFERRED_BYTE_BUDGET
}

pub(crate) fn predecode_cache_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64 {
    system_memory_capped_byte_budget(
        predecode_cache_preferred_byte_budget(),
        system_memory_byte_size,
        PREDECODE_CACHE_SYSTEM_MEMORY_DIVISOR,
    )
}

pub(crate) fn thumbnail_cache_preferred_byte_budget() -> i64 {
    THUMBNAIL_CACHE_PREFERRED_BYTE_BUDGET
}

pub(crate) fn thumbnail_cache_byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64 {
    system_memory_capped_byte_budget(
        thumbnail_cache_preferred_byte_budget(),
        system_memory_byte_size,
        THUMBNAIL_CACHE_SYSTEM_MEMORY_DIVISOR,
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lru_cache_retention_drops_oldest_entries_until_within_budget() {
        assert_eq!(
            retained_indices(lru_cache_retention_plan(
                vec![
                    retention_entry(16, 3),
                    retention_entry(16, 1),
                    retention_entry(16, 2)
                ],
                32
            )),
            vec![0, 2]
        );
        assert_eq!(
            retained_indices(lru_cache_retention_plan(
                vec![
                    retention_entry(60, 3),
                    retention_entry(50, 2),
                    retention_entry(40, 1)
                ],
                100
            )),
            vec![0]
        );
    }

    #[test]
    fn lru_cache_retention_rejects_invalid_costs_and_budgets() {
        assert_eq!(
            retained_indices(lru_cache_retention_plan(
                vec![
                    retention_entry(0, 1),
                    retention_entry(10, 2),
                    retention_entry(-1, 3)
                ],
                100
            )),
            vec![1]
        );
        assert!(lru_cache_retention_plan(vec![retention_entry(10, 1)], 0).is_empty());
    }

    #[test]
    fn lru_cache_retention_reports_retained_byte_costs() {
        assert_eq!(
            lru_cache_retention_plan(
                vec![
                    retention_entry(18, 3),
                    retention_entry(16, 1),
                    retention_entry(14, 2)
                ],
                32
            ),
            vec![retained_entry(0, 18), retained_entry(2, 14)]
        );
    }

    #[test]
    fn display_image_cache_byte_budget_uses_preferred_limit_and_system_memory_cap() {
        let preferred = 512 * 1024 * 1024;

        assert_eq!(
            display_image_cache_byte_budget_for_system_memory(0, preferred),
            preferred
        );
        assert_eq!(
            display_image_cache_byte_budget_for_system_memory(preferred, preferred),
            preferred / DISPLAY_IMAGE_CACHE_SYSTEM_MEMORY_DIVISOR
        );
        assert_eq!(
            display_image_cache_byte_budget_for_system_memory(preferred * 32, preferred),
            preferred
        );
        assert_eq!(
            display_image_cache_byte_budget_for_system_memory(preferred, -1),
            0
        );
    }

    #[test]
    fn predecode_cache_byte_budget_uses_preferred_limit_and_system_memory_cap() {
        let preferred = predecode_cache_preferred_byte_budget();

        assert_eq!(predecode_cache_byte_budget_for_system_memory(0), preferred);
        assert_eq!(
            predecode_cache_byte_budget_for_system_memory(preferred),
            preferred / PREDECODE_CACHE_SYSTEM_MEMORY_DIVISOR
        );
        assert_eq!(
            predecode_cache_byte_budget_for_system_memory(preferred * 16),
            preferred
        );
    }

    #[test]
    fn thumbnail_cache_byte_budget_uses_preferred_limit_and_system_memory_cap() {
        let preferred = thumbnail_cache_preferred_byte_budget();

        assert_eq!(thumbnail_cache_byte_budget_for_system_memory(0), preferred);
        assert_eq!(
            thumbnail_cache_byte_budget_for_system_memory(preferred),
            preferred / THUMBNAIL_CACHE_SYSTEM_MEMORY_DIVISOR
        );
        assert_eq!(
            thumbnail_cache_byte_budget_for_system_memory(preferred * 128),
            preferred
        );
    }

    fn retention_entry(byte_cost: i64, last_use: u64) -> RustLruCacheRetentionEntry {
        RustLruCacheRetentionEntry {
            byte_cost,
            last_use,
        }
    }

    fn retained_entry(original_index: usize, byte_cost: i64) -> RustLruCacheRetainedEntry {
        RustLruCacheRetainedEntry {
            original_index,
            byte_cost,
        }
    }

    fn retained_indices(entries: Vec<RustLruCacheRetainedEntry>) -> Vec<usize> {
        entries
            .into_iter()
            .map(|entry| entry.original_index)
            .collect()
    }
}
