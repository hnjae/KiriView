// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageSpreadSecondaryPageDecision {
        PrimaryOnly = 0,
        LoadNext = 1,
        KeepCurrentSecondary = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageSpreadSecondaryPageRefreshState {
        two_page_mode_active: bool,
        current_page_number: i32,
        image_count: i32,
        primary_page_is_wide: bool,
        next_page_available: bool,
        next_page_is_wide: bool,
        current_secondary_matches_next: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageSpreadSecondaryPageRefreshPlan {
        decision: RustImageSpreadSecondaryPageDecision,
        target_page_number: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageSpreadReadingAvailability {
        has_image: bool,
        has_displayed_image: bool,
        displayed_document_is_comic_book: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageSpreadTwoPageModeChange {
        changed: bool,
        reset_spread_zoom: bool,
        finish_transition: bool,
        clear_secondary_page: bool,
        restore_primary_zoom: bool,
        refresh_secondary_page: bool,
        notify_two_page_mode: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustImageSpreadSecondaryPageRefreshPlan"]
        fn rust_image_spread_secondary_page_refresh_plan(
            state: RustImageSpreadSecondaryPageRefreshState,
        ) -> RustImageSpreadSecondaryPageRefreshPlan;

        #[cxx_name = "rustImageSpreadReadingControlsAvailable"]
        fn rust_image_spread_reading_controls_available(
            availability: RustImageSpreadReadingAvailability,
        ) -> bool;

        #[cxx_name = "rustImageSpreadTwoPageModeChange"]
        fn rust_image_spread_two_page_mode_change(
            current_enabled: bool,
            next_enabled: bool,
            secondary_page_visible: bool,
        ) -> RustImageSpreadTwoPageModeChange;
    }
}

use ffi::{
    RustImageSpreadReadingAvailability, RustImageSpreadSecondaryPageDecision,
    RustImageSpreadSecondaryPageRefreshPlan, RustImageSpreadSecondaryPageRefreshState,
    RustImageSpreadTwoPageModeChange,
};

fn rust_image_spread_secondary_page_refresh_plan(
    state: RustImageSpreadSecondaryPageRefreshState,
) -> RustImageSpreadSecondaryPageRefreshPlan {
    let Some(next_page_number) = state.current_page_number.checked_add(1) else {
        return secondary_page_refresh_plan(RustImageSpreadSecondaryPageDecision::PrimaryOnly, 0);
    };
    if !state.two_page_mode_active
        || state.current_page_number == 1
        || state.primary_page_is_wide
        || next_page_number <= 1
        || next_page_number > state.image_count
        || !state.next_page_available
        || state.next_page_is_wide
    {
        return secondary_page_refresh_plan(RustImageSpreadSecondaryPageDecision::PrimaryOnly, 0);
    }

    if state.current_secondary_matches_next {
        secondary_page_refresh_plan(
            RustImageSpreadSecondaryPageDecision::KeepCurrentSecondary,
            next_page_number,
        )
    } else {
        secondary_page_refresh_plan(
            RustImageSpreadSecondaryPageDecision::LoadNext,
            next_page_number,
        )
    }
}

fn rust_image_spread_reading_controls_available(
    availability: RustImageSpreadReadingAvailability,
) -> bool {
    availability.has_image
        && availability.has_displayed_image
        && availability.displayed_document_is_comic_book
}

fn rust_image_spread_two_page_mode_change(
    current_enabled: bool,
    next_enabled: bool,
    secondary_page_visible: bool,
) -> RustImageSpreadTwoPageModeChange {
    if current_enabled == next_enabled {
        return two_page_mode_change(false, false, false, false, false, false, false);
    }

    let disabling = !next_enabled;
    two_page_mode_change(
        true,
        next_enabled,
        disabling,
        disabling,
        disabling && secondary_page_visible,
        true,
        true,
    )
}

fn two_page_mode_change(
    changed: bool,
    reset_spread_zoom: bool,
    finish_transition: bool,
    clear_secondary_page: bool,
    restore_primary_zoom: bool,
    refresh_secondary_page: bool,
    notify_two_page_mode: bool,
) -> RustImageSpreadTwoPageModeChange {
    RustImageSpreadTwoPageModeChange {
        changed,
        reset_spread_zoom,
        finish_transition,
        clear_secondary_page,
        restore_primary_zoom,
        refresh_secondary_page,
        notify_two_page_mode,
    }
}

fn secondary_page_refresh_plan(
    decision: RustImageSpreadSecondaryPageDecision,
    target_page_number: i32,
) -> RustImageSpreadSecondaryPageRefreshPlan {
    RustImageSpreadSecondaryPageRefreshPlan {
        decision,
        target_page_number,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn secondary_refresh_state(
        two_page_mode_active: bool,
        current_page_number: i32,
        image_count: i32,
        primary_page_is_wide: bool,
        next_page_available: bool,
        next_page_is_wide: bool,
        current_secondary_matches_next: bool,
    ) -> RustImageSpreadSecondaryPageRefreshState {
        RustImageSpreadSecondaryPageRefreshState {
            two_page_mode_active,
            current_page_number,
            image_count,
            primary_page_is_wide,
            next_page_available,
            next_page_is_wide,
            current_secondary_matches_next,
        }
    }

    fn reading_availability(
        has_image: bool,
        has_displayed_image: bool,
        displayed_document_is_comic_book: bool,
    ) -> RustImageSpreadReadingAvailability {
        RustImageSpreadReadingAvailability {
            has_image,
            has_displayed_image,
            displayed_document_is_comic_book,
        }
    }

    #[test]
    fn secondary_page_refresh_plan_selects_primary_keep_or_load() {
        assert_eq!(
            rust_image_spread_secondary_page_refresh_plan(secondary_refresh_state(
                true, 1, 4, false, true, false, false
            )),
            secondary_page_refresh_plan(RustImageSpreadSecondaryPageDecision::PrimaryOnly, 0)
        );
        assert_eq!(
            rust_image_spread_secondary_page_refresh_plan(secondary_refresh_state(
                true, 2, 4, true, true, false, false
            )),
            secondary_page_refresh_plan(RustImageSpreadSecondaryPageDecision::PrimaryOnly, 0)
        );
        assert_eq!(
            rust_image_spread_secondary_page_refresh_plan(secondary_refresh_state(
                true, 2, 4, false, true, true, false
            )),
            secondary_page_refresh_plan(RustImageSpreadSecondaryPageDecision::PrimaryOnly, 0)
        );
        assert_eq!(
            rust_image_spread_secondary_page_refresh_plan(secondary_refresh_state(
                true, 2, 4, false, true, false, true
            )),
            secondary_page_refresh_plan(
                RustImageSpreadSecondaryPageDecision::KeepCurrentSecondary,
                3
            )
        );
        assert_eq!(
            rust_image_spread_secondary_page_refresh_plan(secondary_refresh_state(
                true, 2, 4, false, true, false, false
            )),
            secondary_page_refresh_plan(RustImageSpreadSecondaryPageDecision::LoadNext, 3)
        );
    }

    #[test]
    fn reading_controls_require_displayed_comic_archive_image() {
        assert!(!rust_image_spread_reading_controls_available(
            reading_availability(false, true, true)
        ));
        assert!(!rust_image_spread_reading_controls_available(
            reading_availability(true, false, true)
        ));
        assert!(!rust_image_spread_reading_controls_available(
            reading_availability(true, true, false)
        ));
        assert!(rust_image_spread_reading_controls_available(
            reading_availability(true, true, true)
        ));
    }

    #[test]
    fn two_page_mode_change_plans_enable_disable_side_effects() {
        assert_eq!(
            rust_image_spread_two_page_mode_change(false, false, true),
            two_page_mode_change(false, false, false, false, false, false, false)
        );
        assert_eq!(
            rust_image_spread_two_page_mode_change(false, true, false),
            two_page_mode_change(true, true, false, false, false, true, true)
        );
        assert_eq!(
            rust_image_spread_two_page_mode_change(true, false, false),
            two_page_mode_change(true, false, true, true, false, true, true)
        );
        assert_eq!(
            rust_image_spread_two_page_mode_change(true, false, true),
            two_page_mode_change(true, false, true, true, true, true, true)
        );
    }
}
