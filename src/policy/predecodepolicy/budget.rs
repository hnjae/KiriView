// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use super::config::{
    PREDECODE_NEUTRAL_NAVIGATION_MSEC, PREDECODE_PREFERRED_BYTE_BUDGET,
    PREDECODE_SCRUBBING_NAVIGATION_MSEC, PREDECODE_SYSTEM_MEMORY_DIVISOR,
};

pub(super) fn preferred_byte_budget() -> i64 {
    PREDECODE_PREFERRED_BYTE_BUDGET
}

pub(super) fn debounce_msec() -> i32 {
    PREDECODE_SCRUBBING_NAVIGATION_MSEC
        .try_into()
        .unwrap_or(i32::MAX)
}

pub(super) fn neutral_refresh_msec() -> i32 {
    PREDECODE_NEUTRAL_NAVIGATION_MSEC
        .try_into()
        .unwrap_or(i32::MAX)
}

pub(super) fn byte_budget_for_system_memory(system_memory_byte_size: i64) -> i64 {
    crate::cachebudget::system_memory_capped_byte_budget(
        PREDECODE_PREFERRED_BYTE_BUDGET,
        system_memory_byte_size,
        PREDECODE_SYSTEM_MEMORY_DIVISOR,
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn byte_budget_uses_preferred_limit_and_system_memory_cap() {
        let preferred = preferred_byte_budget();

        assert_eq!(byte_budget_for_system_memory(0), preferred);
        assert_eq!(
            byte_budget_for_system_memory(preferred),
            preferred / PREDECODE_SYSTEM_MEMORY_DIVISOR
        );
        assert_eq!(byte_budget_for_system_memory(preferred * 16), preferred);
    }
}
