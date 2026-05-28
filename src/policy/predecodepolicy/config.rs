// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pub(super) const PREDECODE_BIASED_NAVIGATION_MSEC: i64 = 450;
pub(super) const PREDECODE_SCRUBBING_NAVIGATION_MSEC: i64 = 120;
pub(super) const PREDECODE_NEUTRAL_NAVIGATION_MSEC: i64 = 800;
pub(super) const PREDECODE_SCRUBBING_PAGE_JUMP: i32 = 3;

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
