// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageSpreadNavigationDirection {
        Previous = 0,
        Next = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageSpreadNavigationState {
        two_page_mode_active: bool,
        current_page_number: i32,
        image_count: i32,
        secondary_page_visible: bool,
        previous_page_is_wide: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageSpreadPageNavigationTarget {
        handled_by_spread: bool,
        page_number: i32,
    }

    extern "Rust" {
        #[cxx_name = "rustImageSpreadPreviousPageTarget"]
        fn rust_image_spread_previous_page_target(
            current_page_number: i32,
            secondary_page_visible: bool,
            previous_page_is_wide: bool,
        ) -> i32;

        #[cxx_name = "rustImageSpreadCurrentLastPageNumber"]
        fn rust_image_spread_current_last_page_number(
            current_page_number: i32,
            secondary_page_visible: bool,
        ) -> i32;

        #[cxx_name = "rustImageSpreadRelativePageTarget"]
        fn rust_image_spread_relative_page_target(
            current_page_number: i32,
            image_count: i32,
            offset: i32,
        ) -> i32;

        #[cxx_name = "rustImageSpreadNextPageTarget"]
        fn rust_image_spread_next_page_target(
            current_last_page_number: i32,
            image_count: i32,
        ) -> i32;

        #[cxx_name = "rustImageSpreadShouldBeginTransition"]
        fn rust_image_spread_should_begin_transition(
            two_page_mode_active: bool,
            current_page_number: i32,
            target_page_number: i32,
            image_count: i32,
        ) -> bool;

        #[cxx_name = "rustImageSpreadNavigationCurrentLastPageNumber"]
        fn rust_image_spread_navigation_current_last_page_number(
            state: RustImageSpreadNavigationState,
        ) -> i32;

        #[cxx_name = "rustImageSpreadPageNavigationTarget"]
        fn rust_image_spread_page_navigation_target(
            direction: RustImageSpreadNavigationDirection,
            state: RustImageSpreadNavigationState,
        ) -> RustImageSpreadPageNavigationTarget;

        #[cxx_name = "rustImageSpreadRelativePageNavigationTarget"]
        fn rust_image_spread_relative_page_navigation_target(
            state: RustImageSpreadNavigationState,
            offset: i32,
        ) -> i32;

        #[cxx_name = "rustImageSpreadShouldBeginNavigationTransition"]
        fn rust_image_spread_should_begin_navigation_transition(
            state: RustImageSpreadNavigationState,
            target_page_number: i32,
        ) -> bool;
    }
}

use ffi::{
    RustImageSpreadNavigationDirection, RustImageSpreadNavigationState,
    RustImageSpreadPageNavigationTarget,
};

fn rust_image_spread_previous_page_target(
    current_page_number: i32,
    secondary_page_visible: bool,
    previous_page_is_wide: bool,
) -> i32 {
    if current_page_number <= 0 {
        return 0;
    }

    if current_page_number <= 2 {
        return 1;
    }

    let offset = if secondary_page_visible && !previous_page_is_wide {
        -2
    } else {
        -1
    };
    current_page_number + offset
}

fn rust_image_spread_current_last_page_number(
    current_page_number: i32,
    secondary_page_visible: bool,
) -> i32 {
    if current_page_number <= 0 {
        return 0;
    }

    if secondary_page_visible {
        current_page_number.saturating_add(1)
    } else {
        current_page_number
    }
}

fn rust_image_spread_relative_page_target(
    current_page_number: i32,
    image_count: i32,
    offset: i32,
) -> i32 {
    let Some(target_page) = current_page_number.checked_add(offset) else {
        return 0;
    };
    if target_page < 1 || target_page > image_count {
        return 0;
    }

    target_page
}

fn rust_image_spread_next_page_target(current_last_page_number: i32, image_count: i32) -> i32 {
    let target_page_number = current_last_page_number + 1;
    if target_page_number > image_count {
        return 0;
    }

    target_page_number
}

fn rust_image_spread_should_begin_transition(
    two_page_mode_active: bool,
    current_page_number: i32,
    target_page_number: i32,
    image_count: i32,
) -> bool {
    two_page_mode_active
        && current_page_number > 0
        && target_page_number > 0
        && target_page_number <= image_count
        && target_page_number != current_page_number
}

fn rust_image_spread_navigation_current_last_page_number(
    state: RustImageSpreadNavigationState,
) -> i32 {
    rust_image_spread_current_last_page_number(
        state.current_page_number,
        state.secondary_page_visible,
    )
}

fn rust_image_spread_page_navigation_target(
    direction: RustImageSpreadNavigationDirection,
    state: RustImageSpreadNavigationState,
) -> RustImageSpreadPageNavigationTarget {
    if !state.two_page_mode_active || state.current_page_number <= 0 {
        return page_navigation_target(false, 0);
    }

    let page_number = match direction {
        RustImageSpreadNavigationDirection::Next => rust_image_spread_next_page_target(
            rust_image_spread_navigation_current_last_page_number(state),
            state.image_count,
        ),
        RustImageSpreadNavigationDirection::Previous => rust_image_spread_previous_page_target(
            state.current_page_number,
            state.secondary_page_visible,
            state.previous_page_is_wide,
        ),
        _ => 0,
    };

    page_navigation_target(true, page_number)
}

fn rust_image_spread_relative_page_navigation_target(
    state: RustImageSpreadNavigationState,
    offset: i32,
) -> i32 {
    rust_image_spread_relative_page_target(state.current_page_number, state.image_count, offset)
}

fn rust_image_spread_should_begin_navigation_transition(
    state: RustImageSpreadNavigationState,
    target_page_number: i32,
) -> bool {
    rust_image_spread_should_begin_transition(
        state.two_page_mode_active,
        state.current_page_number,
        target_page_number,
        state.image_count,
    )
}

fn page_navigation_target(
    handled_by_spread: bool,
    page_number: i32,
) -> RustImageSpreadPageNavigationTarget {
    RustImageSpreadPageNavigationTarget {
        handled_by_spread,
        page_number,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn navigation_state(
        two_page_mode_active: bool,
        current_page_number: i32,
        image_count: i32,
        secondary_page_visible: bool,
        previous_page_is_wide: bool,
    ) -> RustImageSpreadNavigationState {
        RustImageSpreadNavigationState {
            two_page_mode_active,
            current_page_number,
            image_count,
            secondary_page_visible,
            previous_page_is_wide,
        }
    }

    #[test]
    fn previous_page_target_keeps_cover_and_steps_by_spread() {
        assert_eq!(rust_image_spread_previous_page_target(0, false, false), 0);
        assert_eq!(rust_image_spread_previous_page_target(1, false, false), 1);
        assert_eq!(rust_image_spread_previous_page_target(2, false, false), 1);
        assert_eq!(rust_image_spread_previous_page_target(5, true, false), 3);
        assert_eq!(rust_image_spread_previous_page_target(5, true, true), 4);
        assert_eq!(rust_image_spread_previous_page_target(5, false, false), 4);
    }

    #[test]
    fn next_page_target_stops_after_last_page() {
        assert_eq!(rust_image_spread_next_page_target(2, 5), 3);
        assert_eq!(rust_image_spread_next_page_target(5, 5), 0);
    }

    #[test]
    fn current_last_page_tracks_secondary_page_visibility() {
        assert_eq!(rust_image_spread_current_last_page_number(0, false), 0);
        assert_eq!(rust_image_spread_current_last_page_number(2, false), 2);
        assert_eq!(rust_image_spread_current_last_page_number(2, true), 3);
    }

    #[test]
    fn relative_page_target_rejects_out_of_range_pages() {
        assert_eq!(rust_image_spread_relative_page_target(3, 5, -1), 2);
        assert_eq!(rust_image_spread_relative_page_target(3, 5, 1), 4);
        assert_eq!(rust_image_spread_relative_page_target(1, 5, -1), 0);
        assert_eq!(rust_image_spread_relative_page_target(5, 5, 1), 0);
    }

    #[test]
    fn transition_policy_requires_active_valid_different_target() {
        assert!(rust_image_spread_should_begin_transition(true, 2, 4, 6));
        assert!(!rust_image_spread_should_begin_transition(false, 2, 4, 6));
        assert!(!rust_image_spread_should_begin_transition(true, 2, 2, 6));
        assert!(!rust_image_spread_should_begin_transition(true, 2, 7, 6));
    }

    #[test]
    fn adjacent_navigation_falls_back_when_spread_is_inactive() {
        let target = rust_image_spread_page_navigation_target(
            RustImageSpreadNavigationDirection::Next,
            navigation_state(false, 3, 6, true, false),
        );

        assert_eq!(target, page_navigation_target(false, 0));
    }

    #[test]
    fn adjacent_navigation_uses_visible_spread_edges() {
        let state = navigation_state(true, 2, 6, true, false);

        assert_eq!(
            rust_image_spread_page_navigation_target(
                RustImageSpreadNavigationDirection::Next,
                state
            ),
            page_navigation_target(true, 4)
        );
        assert_eq!(
            rust_image_spread_page_navigation_target(
                RustImageSpreadNavigationDirection::Previous,
                state
            ),
            page_navigation_target(true, 1)
        );
        assert_eq!(
            rust_image_spread_navigation_current_last_page_number(state),
            3
        );
    }

    #[test]
    fn previous_navigation_accounts_for_wide_previous_page() {
        assert_eq!(
            rust_image_spread_page_navigation_target(
                RustImageSpreadNavigationDirection::Previous,
                navigation_state(true, 5, 8, true, false)
            )
            .page_number,
            3
        );
        assert_eq!(
            rust_image_spread_page_navigation_target(
                RustImageSpreadNavigationDirection::Previous,
                navigation_state(true, 5, 8, true, true)
            )
            .page_number,
            4
        );
    }

    #[test]
    fn relative_navigation_and_transitions_use_spread_state() {
        let state = navigation_state(true, 3, 5, false, false);

        assert_eq!(
            rust_image_spread_relative_page_navigation_target(state, -1),
            2
        );
        assert_eq!(
            rust_image_spread_relative_page_navigation_target(state, 1),
            4
        );
        assert!(rust_image_spread_should_begin_navigation_transition(
            state, 4
        ));
        assert!(!rust_image_spread_should_begin_navigation_transition(
            state, 6
        ));
    }
}
