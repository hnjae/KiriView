// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustDeletionFallbackIndex {
        found: bool,
        index: usize,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustDeletionFallbackCandidateIndices {
        preferred: RustDeletionFallbackIndex,
        fallback: RustDeletionFallbackIndex,
    }

    extern "Rust" {
        #[cxx_name = "rustDeletionFallbackCandidateIndicesForMatches"]
        fn rust_deletion_fallback_candidate_indices_for_matches(
            matches_current: Vec<u8>,
        ) -> RustDeletionFallbackCandidateIndices;
    }
}

use crate::navigationindex::{
    NavigationDirection as CoreNavigationDirection, NavigationIndex as CoreNavigationIndex,
    adjacent_navigation_index as core_adjacent_navigation_index,
    navigation_index_for_matches as core_navigation_index_for_matches,
};
use ffi::{RustDeletionFallbackCandidateIndices, RustDeletionFallbackIndex};

fn rust_deletion_fallback_candidate_indices_for_matches(
    matches_current: Vec<u8>,
) -> RustDeletionFallbackCandidateIndices {
    let candidate_count = matches_current.len();
    let current =
        core_navigation_index_for_matches(matches_current.into_iter().map(|flag| flag != 0));
    deletion_fallback_candidate_indices(candidate_count, current)
}

fn deletion_fallback_candidate_indices(
    candidate_count: usize,
    current: CoreNavigationIndex,
) -> RustDeletionFallbackCandidateIndices {
    RustDeletionFallbackCandidateIndices {
        preferred: deletion_fallback_index(core_adjacent_navigation_index(
            candidate_count,
            current,
            CoreNavigationDirection::Next,
        )),
        fallback: deletion_fallback_index(core_adjacent_navigation_index(
            candidate_count,
            current,
            CoreNavigationDirection::Previous,
        )),
    }
}

fn deletion_fallback_index(index: CoreNavigationIndex) -> RustDeletionFallbackIndex {
    RustDeletionFallbackIndex {
        found: index.found,
        index: index.index,
    }
}

#[cfg(test)]
fn found_index(index: usize) -> RustDeletionFallbackIndex {
    RustDeletionFallbackIndex { found: true, index }
}

#[cfg(test)]
fn missing_index() -> RustDeletionFallbackIndex {
    RustDeletionFallbackIndex {
        found: false,
        index: 0,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn deletion_fallback_prefers_next_then_previous_candidate() {
        let indices = deletion_fallback_candidate_indices(3, core_found_index(1));

        assert_eq!(indices.preferred, found_index(2));
        assert_eq!(indices.fallback, found_index(0));
    }

    #[test]
    fn deletion_fallback_uses_previous_when_current_is_last() {
        let indices = deletion_fallback_candidate_indices(3, core_found_index(2));

        assert_eq!(indices.preferred, missing_index());
        assert_eq!(indices.fallback, found_index(1));
    }

    #[test]
    fn deletion_fallback_rejects_missing_or_out_of_range_current() {
        assert_eq!(
            deletion_fallback_candidate_indices(3, core_missing_index()).preferred,
            missing_index()
        );

        assert_eq!(
            deletion_fallback_candidate_indices(3, core_found_index(3)).fallback,
            missing_index()
        );
    }

    #[test]
    fn deletion_fallback_candidate_indices_resolve_current_match_flags() {
        let indices = rust_deletion_fallback_candidate_indices_for_matches(vec![0, 1, 0]);

        assert_eq!(indices.preferred, found_index(2));
        assert_eq!(indices.fallback, found_index(0));
    }

    fn core_found_index(index: usize) -> CoreNavigationIndex {
        CoreNavigationIndex { found: true, index }
    }

    fn core_missing_index() -> CoreNavigationIndex {
        CoreNavigationIndex {
            found: false,
            index: 0,
        }
    }
}
