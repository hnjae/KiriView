// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustLruCacheRetentionEntry {
        byte_cost: i64,
        last_use: u64,
    }

    extern "Rust" {
        #[cxx_name = "rustLruCacheRetainedIndices"]
        fn rust_lru_cache_retained_indices(
            entries: Vec<RustLruCacheRetentionEntry>,
            byte_budget: i64,
        ) -> Vec<usize>;

        #[cxx_name = "rustStaticTileCacheByteBudgetForSystemMemory"]
        fn rust_static_tile_cache_byte_budget_for_system_memory(
            system_memory_byte_size: i64,
            preferred_byte_budget: i64,
        ) -> i64;
    }
}

const STATIC_TILE_CACHE_SYSTEM_MEMORY_DIVISOR: i64 = 16;

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

use ffi::RustLruCacheRetentionEntry;

fn rust_lru_cache_retained_indices(
    entries: Vec<RustLruCacheRetentionEntry>,
    byte_budget: i64,
) -> Vec<usize> {
    lru_cache_retained_indices(entries, byte_budget)
}

fn rust_static_tile_cache_byte_budget_for_system_memory(
    system_memory_byte_size: i64,
    preferred_byte_budget: i64,
) -> i64 {
    static_tile_cache_byte_budget_for_system_memory(system_memory_byte_size, preferred_byte_budget)
}

pub(crate) fn lru_cache_retained_indices(
    entries: Vec<RustLruCacheRetentionEntry>,
    byte_budget: i64,
) -> Vec<usize> {
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
        .map(|entry| entry.original_index)
        .collect()
}

pub(crate) fn static_tile_cache_byte_budget_for_system_memory(
    system_memory_byte_size: i64,
    preferred_byte_budget: i64,
) -> i64 {
    system_memory_capped_byte_budget(
        preferred_byte_budget,
        system_memory_byte_size,
        STATIC_TILE_CACHE_SYSTEM_MEMORY_DIVISOR,
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lru_cache_retention_drops_oldest_entries_until_within_budget() {
        assert_eq!(
            lru_cache_retained_indices(
                vec![
                    retention_entry(16, 3),
                    retention_entry(16, 1),
                    retention_entry(16, 2)
                ],
                32
            ),
            vec![0, 2]
        );
        assert_eq!(
            lru_cache_retained_indices(
                vec![
                    retention_entry(60, 3),
                    retention_entry(50, 2),
                    retention_entry(40, 1)
                ],
                100
            ),
            vec![0]
        );
    }

    #[test]
    fn lru_cache_retention_rejects_invalid_costs_and_budgets() {
        assert_eq!(
            lru_cache_retained_indices(
                vec![
                    retention_entry(0, 1),
                    retention_entry(10, 2),
                    retention_entry(-1, 3)
                ],
                100
            ),
            vec![1]
        );
        assert!(lru_cache_retained_indices(vec![retention_entry(10, 1)], 0).is_empty());
    }

    #[test]
    fn static_tile_cache_byte_budget_uses_preferred_limit_and_system_memory_cap() {
        let preferred = 512 * 1024 * 1024;

        assert_eq!(
            static_tile_cache_byte_budget_for_system_memory(0, preferred),
            preferred
        );
        assert_eq!(
            static_tile_cache_byte_budget_for_system_memory(preferred, preferred),
            preferred / STATIC_TILE_CACHE_SYSTEM_MEMORY_DIVISOR
        );
        assert_eq!(
            static_tile_cache_byte_budget_for_system_memory(preferred * 32, preferred),
            preferred
        );
        assert_eq!(
            static_tile_cache_byte_budget_for_system_memory(preferred, -1),
            0
        );
    }

    fn retention_entry(byte_cost: i64, last_use: u64) -> RustLruCacheRetentionEntry {
        RustLruCacheRetentionEntry {
            byte_cost,
            last_use,
        }
    }
}
