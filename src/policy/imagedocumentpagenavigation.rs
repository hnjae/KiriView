// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, PartialEq, Eq)]
    enum RustNavigationDirection {
        Previous = 0,
        Next = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustNavigationIndex {
        found: bool,
        index: usize,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPageNavigationUpdate {
        insert_current_url: bool,
        current_index: i32,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustPageNavigationPreviewState {
        keep_known_urls: bool,
        current_index: i32,
    }

    extern "Rust" {
        #[cxx_name = "rustPageNavigationStateUpdate"]
        fn rust_page_navigation_state_update(
            current: RustNavigationIndex,
            current_url_valid: bool,
            candidate_url_count: usize,
        ) -> RustPageNavigationUpdate;

        #[cxx_name = "rustPageNavigationPreviewState"]
        fn rust_page_navigation_preview_state(
            current: RustNavigationIndex,
            current_url_valid: bool,
            known_url_count: usize,
        ) -> RustPageNavigationPreviewState;

        #[cxx_name = "rustPageNavigationTargetIndex"]
        fn rust_page_navigation_target_index(
            page_count: usize,
            current_index: i32,
            page_number: i32,
        ) -> RustNavigationIndex;

        #[cxx_name = "rustPageNavigationAdjacentTargetIndex"]
        fn rust_page_navigation_adjacent_target_index(
            page_count: usize,
            current_index: i32,
            direction: RustNavigationDirection,
        ) -> RustNavigationIndex;
    }
}

use ffi::{
    RustNavigationDirection, RustNavigationIndex, RustPageNavigationPreviewState,
    RustPageNavigationUpdate,
};

fn adjacent_navigation_index(
    candidate_count: usize,
    current: RustNavigationIndex,
    direction: RustNavigationDirection,
) -> RustNavigationIndex {
    if !current.found || current.index >= candidate_count {
        return missing_index();
    }

    match direction {
        RustNavigationDirection::Previous => {
            if current.index == 0 {
                missing_index()
            } else {
                found_index(current.index - 1)
            }
        }
        RustNavigationDirection::Next => {
            let next_index = current.index + 1;
            if next_index == candidate_count {
                missing_index()
            } else {
                found_index(next_index)
            }
        }
        _ => missing_index(),
    }
}

fn rust_page_navigation_state_update(
    current: RustNavigationIndex,
    current_url_valid: bool,
    candidate_url_count: usize,
) -> RustPageNavigationUpdate {
    if current.found {
        return RustPageNavigationUpdate {
            insert_current_url: false,
            current_index: i32::try_from(current.index).unwrap_or(i32::MAX),
        };
    }

    if current_url_valid && candidate_url_count == 0 {
        return RustPageNavigationUpdate {
            insert_current_url: true,
            current_index: 0,
        };
    }

    RustPageNavigationUpdate {
        insert_current_url: false,
        current_index: -1,
    }
}

fn rust_page_navigation_preview_state(
    current: RustNavigationIndex,
    current_url_valid: bool,
    known_url_count: usize,
) -> RustPageNavigationPreviewState {
    if current.found {
        return page_navigation_preview_state(
            true,
            i32::try_from(current.index).unwrap_or(i32::MAX),
        );
    }

    if current_url_valid && known_url_count > 0 {
        return page_navigation_preview_state(true, -1);
    }

    page_navigation_preview_state(false, -1)
}

fn rust_page_navigation_target_index(
    page_count: usize,
    current_index: i32,
    page_number: i32,
) -> RustNavigationIndex {
    let Some(page_index_number) = page_number.checked_sub(1) else {
        return missing_index();
    };
    let Ok(page_index) = usize::try_from(page_index_number) else {
        return missing_index();
    };
    if page_index >= page_count {
        return missing_index();
    }
    if let Ok(current) = usize::try_from(current_index)
        && page_index == current
    {
        return missing_index();
    }

    found_index(page_index)
}

fn rust_page_navigation_adjacent_target_index(
    page_count: usize,
    current_index: i32,
    direction: RustNavigationDirection,
) -> RustNavigationIndex {
    let Ok(index) = usize::try_from(current_index) else {
        return missing_index();
    };
    if index >= page_count {
        return missing_index();
    }

    adjacent_navigation_index(page_count, found_index(index), direction)
}

fn found_index(index: usize) -> RustNavigationIndex {
    RustNavigationIndex { found: true, index }
}

fn missing_index() -> RustNavigationIndex {
    RustNavigationIndex {
        found: false,
        index: 0,
    }
}

fn page_navigation_preview_state(
    keep_known_urls: bool,
    current_index: i32,
) -> RustPageNavigationPreviewState {
    RustPageNavigationPreviewState {
        keep_known_urls,
        current_index,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn adjacent_navigation_candidate_index_moves_without_wrapping() {
        assert_eq!(
            adjacent_navigation_index(3, found_index(1), RustNavigationDirection::Previous),
            found_index(0)
        );
        assert_eq!(
            adjacent_navigation_index(3, found_index(1), RustNavigationDirection::Next),
            found_index(2)
        );
        assert_eq!(
            adjacent_navigation_index(3, found_index(0), RustNavigationDirection::Previous),
            missing_index()
        );
        assert_eq!(
            adjacent_navigation_index(3, found_index(2), RustNavigationDirection::Next),
            missing_index()
        );
    }

    #[test]
    fn adjacent_navigation_candidate_index_rejects_missing_or_out_of_range_current() {
        assert_eq!(
            adjacent_navigation_index(3, missing_index(), RustNavigationDirection::Next),
            missing_index()
        );
        assert_eq!(
            adjacent_navigation_index(3, found_index(3), RustNavigationDirection::Previous),
            missing_index()
        );
    }

    #[test]
    fn page_navigation_target_rejects_invalid_current_and_out_of_range_pages() {
        assert_eq!(rust_page_navigation_target_index(3, 1, 0), missing_index());
        assert_eq!(rust_page_navigation_target_index(3, 1, 2), missing_index());
        assert_eq!(rust_page_navigation_target_index(3, 1, 4), missing_index());
    }

    #[test]
    fn page_navigation_target_converts_page_number_to_index() {
        assert_eq!(rust_page_navigation_target_index(3, 1, 1), found_index(0));
        assert_eq!(rust_page_navigation_target_index(3, -1, 3), found_index(2));
    }

    #[test]
    fn page_navigation_adjacent_target_uses_pending_current_index() {
        assert_eq!(
            rust_page_navigation_adjacent_target_index(3, 1, RustNavigationDirection::Previous),
            found_index(0)
        );
        assert_eq!(
            rust_page_navigation_adjacent_target_index(3, 1, RustNavigationDirection::Next),
            found_index(2)
        );
        assert_eq!(
            rust_page_navigation_adjacent_target_index(3, 0, RustNavigationDirection::Previous),
            missing_index()
        );
        assert_eq!(
            rust_page_navigation_adjacent_target_index(3, 2, RustNavigationDirection::Next),
            missing_index()
        );
        assert_eq!(
            rust_page_navigation_adjacent_target_index(0, -1, RustNavigationDirection::Next),
            missing_index()
        );
        assert_eq!(
            rust_page_navigation_adjacent_target_index(3, 9, RustNavigationDirection::Next),
            missing_index()
        );
    }

    #[test]
    fn page_navigation_preview_state_reuses_known_urls_for_known_current_url() {
        assert_eq!(
            rust_page_navigation_preview_state(found_index(2), true, 3),
            page_navigation_preview_state(true, 2)
        );
    }

    #[test]
    fn page_navigation_preview_state_keeps_pending_empty_known_list_unknown() {
        assert_eq!(
            rust_page_navigation_preview_state(missing_index(), true, 0),
            page_navigation_preview_state(false, -1)
        );
        assert_eq!(
            rust_page_navigation_preview_state(missing_index(), false, 3),
            page_navigation_preview_state(false, -1)
        );
    }

    #[test]
    fn page_navigation_preview_state_keeps_known_urls_for_temporarily_missing_current_url() {
        assert_eq!(
            rust_page_navigation_preview_state(missing_index(), true, 3),
            page_navigation_preview_state(true, -1)
        );
    }

    #[test]
    fn page_navigation_state_update_keeps_candidate_url_index_or_inserts_singleton() {
        assert_eq!(
            rust_page_navigation_state_update(found_index(2), true, 3),
            RustPageNavigationUpdate {
                insert_current_url: false,
                current_index: 2,
            }
        );
        assert_eq!(
            rust_page_navigation_state_update(missing_index(), true, 0),
            RustPageNavigationUpdate {
                insert_current_url: true,
                current_index: 0,
            }
        );
        assert_eq!(
            rust_page_navigation_state_update(missing_index(), true, 3),
            RustPageNavigationUpdate {
                insert_current_url: false,
                current_index: -1,
            }
        );
    }

    #[test]
    fn page_navigation_state_update_marks_empty_for_invalid_current_url() {
        assert_eq!(
            rust_page_navigation_state_update(missing_index(), false, 3),
            RustPageNavigationUpdate {
                insert_current_url: false,
                current_index: -1,
            }
        );
    }
}
