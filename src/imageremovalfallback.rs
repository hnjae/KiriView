// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::navigationindex::{
    NavigationDirection, NavigationIndex as CoreNavigationIndex, adjacent_navigation_index,
    navigation_index_for_matches,
};

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageRemovalFallbackIndex {
        found: bool,
        index: usize,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageRemovalFallbackCandidateIndices {
        preferred: RustImageRemovalFallbackIndex,
        fallback: RustImageRemovalFallbackIndex,
    }

    extern "Rust" {
        #[cxx_name = "rustImageRemovalFallbackCandidateIndices"]
        fn rust_image_removal_fallback_candidate_indices(
            matches_current: Vec<u8>,
        ) -> RustImageRemovalFallbackCandidateIndices;
    }
}

use ffi::{RustImageRemovalFallbackCandidateIndices, RustImageRemovalFallbackIndex};

fn rust_image_removal_fallback_candidate_indices(
    matches_current: Vec<u8>,
) -> RustImageRemovalFallbackCandidateIndices {
    let candidate_count = matches_current.len();
    let current = navigation_index_for_matches(matches_current.into_iter().map(|flag| flag != 0));
    if !current.found {
        return missing_fallback_candidates();
    }

    RustImageRemovalFallbackCandidateIndices {
        preferred: rust_fallback_index(adjacent_navigation_index(
            candidate_count,
            current,
            NavigationDirection::Next,
        )),
        fallback: rust_fallback_index(adjacent_navigation_index(
            candidate_count,
            current,
            NavigationDirection::Previous,
        )),
    }
}

fn missing_fallback_candidates() -> RustImageRemovalFallbackCandidateIndices {
    RustImageRemovalFallbackCandidateIndices {
        preferred: missing_fallback_index(),
        fallback: missing_fallback_index(),
    }
}

fn rust_fallback_index(index: CoreNavigationIndex) -> RustImageRemovalFallbackIndex {
    RustImageRemovalFallbackIndex {
        found: index.found,
        index: index.index,
    }
}

fn missing_fallback_index() -> RustImageRemovalFallbackIndex {
    RustImageRemovalFallbackIndex {
        found: false,
        index: 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn found_index(index: usize) -> RustImageRemovalFallbackIndex {
        RustImageRemovalFallbackIndex { found: true, index }
    }

    #[test]
    fn fallback_candidates_prefer_next_then_previous() {
        let selection = rust_image_removal_fallback_candidate_indices(vec![0, 1, 0]);

        assert_eq!(selection.preferred, found_index(2));
        assert_eq!(selection.fallback, found_index(0));
    }

    #[test]
    fn fallback_candidates_keep_previous_when_current_is_last() {
        let selection = rust_image_removal_fallback_candidate_indices(vec![0, 0, 1]);

        assert_eq!(selection.preferred, missing_fallback_index());
        assert_eq!(selection.fallback, found_index(1));
    }

    #[test]
    fn fallback_candidates_are_missing_without_current_match() {
        assert_eq!(
            rust_image_removal_fallback_candidate_indices(vec![0, 0]),
            missing_fallback_candidates()
        );
    }
}
