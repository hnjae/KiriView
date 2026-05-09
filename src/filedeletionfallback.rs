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
        #[cxx_name = "rustDeletionFallbackCandidateIndices"]
        fn rust_deletion_fallback_candidate_indices(
            candidate_count: usize,
            current: RustDeletionFallbackIndex,
        ) -> RustDeletionFallbackCandidateIndices;
    }
}

use ffi::{RustDeletionFallbackCandidateIndices, RustDeletionFallbackIndex};

#[derive(Clone, Copy)]
enum DeletionFallbackDirection {
    Previous,
    Next,
}

fn rust_deletion_fallback_candidate_indices(
    candidate_count: usize,
    current: RustDeletionFallbackIndex,
) -> RustDeletionFallbackCandidateIndices {
    RustDeletionFallbackCandidateIndices {
        preferred: adjacent_fallback_candidate_index(
            candidate_count,
            current,
            DeletionFallbackDirection::Next,
        ),
        fallback: adjacent_fallback_candidate_index(
            candidate_count,
            current,
            DeletionFallbackDirection::Previous,
        ),
    }
}

fn adjacent_fallback_candidate_index(
    candidate_count: usize,
    current: RustDeletionFallbackIndex,
    direction: DeletionFallbackDirection,
) -> RustDeletionFallbackIndex {
    if !current.found || current.index >= candidate_count {
        return missing_index();
    }

    match direction {
        DeletionFallbackDirection::Previous => {
            if current.index == 0 {
                missing_index()
            } else {
                found_index(current.index - 1)
            }
        }
        DeletionFallbackDirection::Next => {
            let next_index = current.index + 1;
            if next_index == candidate_count {
                missing_index()
            } else {
                found_index(next_index)
            }
        }
    }
}

fn found_index(index: usize) -> RustDeletionFallbackIndex {
    RustDeletionFallbackIndex { found: true, index }
}

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
        let indices = rust_deletion_fallback_candidate_indices(3, found_index(1));

        assert_eq!(indices.preferred, found_index(2));
        assert_eq!(indices.fallback, found_index(0));
    }

    #[test]
    fn deletion_fallback_uses_previous_when_current_is_last() {
        let indices = rust_deletion_fallback_candidate_indices(3, found_index(2));

        assert_eq!(indices.preferred, missing_index());
        assert_eq!(indices.fallback, found_index(1));
    }

    #[test]
    fn deletion_fallback_rejects_missing_or_out_of_range_current() {
        assert_eq!(
            rust_deletion_fallback_candidate_indices(3, missing_index()).preferred,
            missing_index()
        );

        assert_eq!(
            rust_deletion_fallback_candidate_indices(3, found_index(3)).fallback,
            missing_index()
        );
    }
}
