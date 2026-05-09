// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const PREDECODE_PREVIOUS_IMAGE_COUNT: usize = 2;
const PREDECODE_NEXT_IMAGE_COUNT: usize = 4;

use crate::navigationindex::{
    NavigationDirection as CoreNavigationDirection, NavigationIndex as CoreNavigationIndex,
    adjacent_navigation_index as core_adjacent_navigation_index,
    navigation_index_for_matches as core_navigation_index_for_matches,
};

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

    #[derive(Clone, Copy)]
    struct RustPageNavigationUpdate {
        insert_current_url: bool,
        current_index: i32,
    }

    extern "Rust" {
        #[cxx_name = "rustPredecodeWindowImageIndices"]
        fn rust_predecode_window_image_indices(
            candidate_count: usize,
            current: RustNavigationIndex,
        ) -> Vec<usize>;

        #[cxx_name = "rustAdjacentNavigationCandidateIndex"]
        fn rust_adjacent_navigation_candidate_index(
            candidate_count: usize,
            current: RustNavigationIndex,
            direction: RustNavigationDirection,
        ) -> RustNavigationIndex;

        #[cxx_name = "rustCurrentNavigationIndex"]
        fn rust_current_navigation_index(matches_current: Vec<u8>) -> RustNavigationIndex;

        #[cxx_name = "rustPageNavigationStateUpdate"]
        fn rust_page_navigation_state_update(
            current: RustNavigationIndex,
            current_url_valid: bool,
        ) -> RustPageNavigationUpdate;

        #[cxx_name = "rustPageNavigationTargetIndex"]
        fn rust_page_navigation_target_index(
            image_count: usize,
            current_index: i32,
            page_number: i32,
        ) -> RustNavigationIndex;
    }
}

use ffi::{RustNavigationDirection, RustNavigationIndex, RustPageNavigationUpdate};

fn rust_predecode_window_image_indices(
    candidate_count: usize,
    current: RustNavigationIndex,
) -> Vec<usize> {
    let mut indices = Vec::new();
    if !current.found || current.index >= candidate_count {
        return indices;
    }

    let append_offset = |indices: &mut Vec<usize>, offset: isize| {
        let Some(target_index) = current.index.checked_add_signed(offset) else {
            return;
        };
        if target_index >= candidate_count {
            return;
        }

        indices.push(target_index);
    };

    append_offset(&mut indices, 0);
    for offset in 1..=PREDECODE_NEXT_IMAGE_COUNT {
        append_offset(&mut indices, offset as isize);
        if offset <= PREDECODE_PREVIOUS_IMAGE_COUNT {
            append_offset(&mut indices, -(offset as isize));
        }
    }
    indices
}

fn rust_adjacent_navigation_candidate_index(
    candidate_count: usize,
    current: RustNavigationIndex,
    direction: RustNavigationDirection,
) -> RustNavigationIndex {
    let Some(direction) = core_navigation_direction(direction) else {
        return missing_index();
    };

    rust_navigation_index(core_adjacent_navigation_index(
        candidate_count,
        core_navigation_index(current),
        direction,
    ))
}

fn rust_current_navigation_index(matches_current: Vec<u8>) -> RustNavigationIndex {
    rust_navigation_index(core_navigation_index_for_matches(
        matches_current.into_iter().map(|flag| flag != 0),
    ))
}

fn rust_page_navigation_state_update(
    current: RustNavigationIndex,
    current_url_valid: bool,
) -> RustPageNavigationUpdate {
    if current.found {
        return RustPageNavigationUpdate {
            insert_current_url: false,
            current_index: i32::try_from(current.index).unwrap_or(i32::MAX),
        };
    }

    if current_url_valid {
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

fn rust_page_navigation_target_index(
    image_count: usize,
    current_index: i32,
    page_number: i32,
) -> RustNavigationIndex {
    let Some(page_index_number) = page_number.checked_sub(1) else {
        return missing_index();
    };
    let Ok(page_index) = usize::try_from(page_index_number) else {
        return missing_index();
    };
    if page_index >= image_count {
        return missing_index();
    }
    if let Ok(current) = usize::try_from(current_index)
        && page_index == current
    {
        return missing_index();
    }

    found_index(page_index)
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

fn core_navigation_direction(
    direction: RustNavigationDirection,
) -> Option<CoreNavigationDirection> {
    match direction {
        RustNavigationDirection::Previous => Some(CoreNavigationDirection::Previous),
        RustNavigationDirection::Next => Some(CoreNavigationDirection::Next),
        _ => None,
    }
}

fn core_navigation_index(index: RustNavigationIndex) -> CoreNavigationIndex {
    CoreNavigationIndex {
        found: index.found,
        index: index.index,
    }
}

fn rust_navigation_index(index: CoreNavigationIndex) -> RustNavigationIndex {
    RustNavigationIndex {
        found: index.found,
        index: index.index,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn adjacent_navigation_candidate_index_moves_without_wrapping() {
        assert_eq!(
            rust_adjacent_navigation_candidate_index(
                3,
                found_index(1),
                RustNavigationDirection::Previous
            ),
            found_index(0)
        );
        assert_eq!(
            rust_adjacent_navigation_candidate_index(
                3,
                found_index(1),
                RustNavigationDirection::Next
            ),
            found_index(2)
        );
        assert_eq!(
            rust_adjacent_navigation_candidate_index(
                3,
                found_index(0),
                RustNavigationDirection::Previous
            ),
            missing_index()
        );
        assert_eq!(
            rust_adjacent_navigation_candidate_index(
                3,
                found_index(2),
                RustNavigationDirection::Next
            ),
            missing_index()
        );
    }

    #[test]
    fn current_navigation_index_uses_first_match_flag() {
        assert_eq!(rust_current_navigation_index(vec![0, 1, 1]), found_index(1));
        assert_eq!(rust_current_navigation_index(vec![0, 0]), missing_index());
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
}
