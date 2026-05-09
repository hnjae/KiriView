// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const RGBA_BYTES_PER_PIXEL: i64 = 4;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustSystemMemoryByteSize {
        found: bool,
        byte_size: i64,
    }

    extern "Rust" {
        #[cxx_name = "rustEstimatedRgbaByteCost"]
        fn rust_estimated_rgba_byte_cost(width: i32, height: i32) -> i64;

        #[cxx_name = "rustSystemMemoryByteSize"]
        fn rust_system_memory_byte_size(
            page_count: i64,
            page_size: i64,
        ) -> RustSystemMemoryByteSize;

        #[cxx_name = "rustSystemMemoryCappedByteBudget"]
        fn rust_system_memory_capped_byte_budget(
            preferred_byte_budget: i64,
            system_memory_byte_size: i64,
            memory_divisor: i64,
        ) -> i64;
    }
}

use ffi::RustSystemMemoryByteSize;

fn rust_estimated_rgba_byte_cost(width: i32, height: i32) -> i64 {
    if width <= 0 || height <= 0 {
        return 0;
    }

    i64::from(width)
        .checked_mul(i64::from(height))
        .and_then(|pixel_count| pixel_count.checked_mul(RGBA_BYTES_PER_PIXEL))
        .unwrap_or(i64::MAX)
}

fn rust_system_memory_byte_size(page_count: i64, page_size: i64) -> RustSystemMemoryByteSize {
    if page_count <= 0 || page_size <= 0 {
        return missing_system_memory_byte_size();
    }

    RustSystemMemoryByteSize {
        found: true,
        byte_size: page_count.checked_mul(page_size).unwrap_or(i64::MAX),
    }
}

fn rust_system_memory_capped_byte_budget(
    preferred_byte_budget: i64,
    system_memory_byte_size: i64,
    memory_divisor: i64,
) -> i64 {
    system_memory_capped_byte_budget(
        preferred_byte_budget,
        system_memory_byte_size,
        memory_divisor,
    )
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

fn missing_system_memory_byte_size() -> RustSystemMemoryByteSize {
    RustSystemMemoryByteSize {
        found: false,
        byte_size: 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn estimated_rgba_byte_cost_handles_empty_sizes_and_overflow() {
        assert_eq!(rust_estimated_rgba_byte_cost(0, 10), 0);
        assert_eq!(rust_estimated_rgba_byte_cost(10, 0), 0);
        assert_eq!(rust_estimated_rgba_byte_cost(-1, 10), 0);
        assert_eq!(rust_estimated_rgba_byte_cost(10, 3), 120);
        assert_eq!(rust_estimated_rgba_byte_cost(i32::MAX, i32::MAX), i64::MAX);
    }

    #[test]
    fn system_memory_byte_size_requires_positive_page_inputs_and_saturates_overflow() {
        assert_eq!(rust_system_memory_byte_size(4, 1024).byte_size, 4096);
        assert!(rust_system_memory_byte_size(4, 1024).found);
        assert_eq!(
            rust_system_memory_byte_size(0, 1024),
            missing_system_memory_byte_size()
        );
        assert_eq!(
            rust_system_memory_byte_size(4, 0),
            missing_system_memory_byte_size()
        );
        assert_eq!(
            rust_system_memory_byte_size(i64::MAX, 2).byte_size,
            i64::MAX
        );
    }

    #[test]
    fn system_memory_capped_byte_budget_caps_preferred_budget() {
        let preferred_byte_budget = 1024;

        assert_eq!(
            system_memory_capped_byte_budget(preferred_byte_budget, 0, 8),
            preferred_byte_budget
        );
        assert_eq!(
            system_memory_capped_byte_budget(preferred_byte_budget, preferred_byte_budget, 8),
            preferred_byte_budget / 8
        );
        assert_eq!(
            system_memory_capped_byte_budget(preferred_byte_budget, preferred_byte_budget * 16, 8),
            preferred_byte_budget
        );
        assert_eq!(
            system_memory_capped_byte_budget(preferred_byte_budget, preferred_byte_budget, 0),
            preferred_byte_budget
        );
        assert_eq!(system_memory_capped_byte_budget(-1, 0, 8), 0);
    }
}
