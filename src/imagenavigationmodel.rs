// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

const PREDECODE_PREVIOUS_IMAGE_COUNT: usize = 2;
const PREDECODE_NEXT_IMAGE_COUNT: usize = 4;

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, PartialEq, Eq)]
    enum RustNavigationDirection {
        Previous = 0,
        Next = 1,
    }

    #[derive(Clone, Copy)]
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

        #[cxx_name = "rustPageNavigationStateUpdate"]
        fn rust_page_navigation_state_update(
            current: RustNavigationIndex,
            current_url_valid: bool,
        ) -> RustPageNavigationUpdate;
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

fn found_index(index: usize) -> RustNavigationIndex {
    RustNavigationIndex { found: true, index }
}

fn missing_index() -> RustNavigationIndex {
    RustNavigationIndex {
        found: false,
        index: 0,
    }
}
