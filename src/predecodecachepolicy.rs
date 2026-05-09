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
    }
}

use ffi::RustPredecodeCacheableByteCost;

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
}
