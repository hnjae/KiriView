// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

use crate::navigationindex::{
    NavigationDirection, NavigationIndex as CoreNavigationIndex, adjacent_navigation_index,
    navigation_index_for_matches,
};

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustArchiveDocumentKind {
        Empty = 0,
        ComicBook = 1,
        General = 2,
        Directory = 3,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDeletionTarget {
        NoDeletionTarget = 0,
        DisplayedImage = 1,
        ArchiveDocument = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustFileDeletionResult {
        Succeeded = 0,
        Canceled = 1,
        Failed = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustFileDeletionCompletionAction {
        ClearDeletedImageAndOpenFallback = 0,
        Ignore = 1,
        ReportFailure = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageRemovalFallbackPlanKind {
        NoFallback = 0,
        Image = 1,
        ComicBook = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDeletionTargetInput {
        image_url_empty: bool,
        archive_document_empty: bool,
        archive_document_contains_image: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageRemovalFallbackPlanInput {
        archive_document_kind: RustArchiveDocumentKind,
        archive_document_contains_image: bool,
        has_image_candidate_context: bool,
    }

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
        #[cxx_name = "rustImageDeletionTarget"]
        fn rust_image_deletion_target(
            input: RustImageDeletionTargetInput,
        ) -> RustImageDeletionTarget;

        #[cxx_name = "rustFileDeletionCompletionAction"]
        fn rust_file_deletion_completion_action(
            result: RustFileDeletionResult,
        ) -> RustFileDeletionCompletionAction;

        #[cxx_name = "rustImageRemovalFallbackPlanKind"]
        fn rust_image_removal_fallback_plan_kind(
            input: RustImageRemovalFallbackPlanInput,
        ) -> RustImageRemovalFallbackPlanKind;

        #[cxx_name = "rustImageRemovalFallbackCandidateIndices"]
        fn rust_image_removal_fallback_candidate_indices(
            matches_current: Vec<u8>,
        ) -> RustImageRemovalFallbackCandidateIndices;
    }
}

use ffi::{
    RustArchiveDocumentKind, RustFileDeletionCompletionAction, RustFileDeletionResult,
    RustImageDeletionTarget, RustImageDeletionTargetInput,
    RustImageRemovalFallbackCandidateIndices, RustImageRemovalFallbackIndex,
    RustImageRemovalFallbackPlanInput, RustImageRemovalFallbackPlanKind,
};

fn rust_image_deletion_target(input: RustImageDeletionTargetInput) -> RustImageDeletionTarget {
    if input.image_url_empty {
        return RustImageDeletionTarget::NoDeletionTarget;
    }

    if !input.archive_document_empty && input.archive_document_contains_image {
        return RustImageDeletionTarget::ArchiveDocument;
    }

    RustImageDeletionTarget::DisplayedImage
}

fn rust_file_deletion_completion_action(
    result: RustFileDeletionResult,
) -> RustFileDeletionCompletionAction {
    match result {
        RustFileDeletionResult::Succeeded => {
            RustFileDeletionCompletionAction::ClearDeletedImageAndOpenFallback
        }
        RustFileDeletionResult::Canceled => RustFileDeletionCompletionAction::Ignore,
        RustFileDeletionResult::Failed => RustFileDeletionCompletionAction::ReportFailure,
        _ => RustFileDeletionCompletionAction::ReportFailure,
    }
}

fn rust_image_removal_fallback_plan_kind(
    input: RustImageRemovalFallbackPlanInput,
) -> RustImageRemovalFallbackPlanKind {
    if input.archive_document_contains_image {
        return match input.archive_document_kind {
            RustArchiveDocumentKind::ComicBook => RustImageRemovalFallbackPlanKind::ComicBook,
            RustArchiveDocumentKind::General | RustArchiveDocumentKind::Directory => {
                RustImageRemovalFallbackPlanKind::NoFallback
            }
            _ => RustImageRemovalFallbackPlanKind::NoFallback,
        };
    }

    if input.has_image_candidate_context {
        return RustImageRemovalFallbackPlanKind::Image;
    }

    RustImageRemovalFallbackPlanKind::NoFallback
}

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

    fn deletion_target_input(
        image_url_empty: bool,
        archive_document_empty: bool,
        archive_document_contains_image: bool,
    ) -> RustImageDeletionTargetInput {
        RustImageDeletionTargetInput {
            image_url_empty,
            archive_document_empty,
            archive_document_contains_image,
        }
    }

    fn fallback_plan_input(
        archive_document_kind: RustArchiveDocumentKind,
        archive_document_contains_image: bool,
        has_image_candidate_context: bool,
    ) -> RustImageRemovalFallbackPlanInput {
        RustImageRemovalFallbackPlanInput {
            archive_document_kind,
            archive_document_contains_image,
            has_image_candidate_context,
        }
    }

    #[test]
    fn deletion_target_uses_archive_document_only_for_contained_document_images() {
        assert_eq!(
            rust_image_deletion_target(deletion_target_input(false, false, true)),
            RustImageDeletionTarget::ArchiveDocument
        );
        assert_eq!(
            rust_image_deletion_target(deletion_target_input(false, false, false)),
            RustImageDeletionTarget::DisplayedImage
        );
        assert_eq!(
            rust_image_deletion_target(deletion_target_input(false, true, false)),
            RustImageDeletionTarget::DisplayedImage
        );
        assert_eq!(
            rust_image_deletion_target(deletion_target_input(true, false, true)),
            RustImageDeletionTarget::NoDeletionTarget
        );
    }

    #[test]
    fn completion_action_routes_deletion_results() {
        assert_eq!(
            rust_file_deletion_completion_action(RustFileDeletionResult::Succeeded),
            RustFileDeletionCompletionAction::ClearDeletedImageAndOpenFallback
        );
        assert_eq!(
            rust_file_deletion_completion_action(RustFileDeletionResult::Canceled),
            RustFileDeletionCompletionAction::Ignore
        );
        assert_eq!(
            rust_file_deletion_completion_action(RustFileDeletionResult::Failed),
            RustFileDeletionCompletionAction::ReportFailure
        );
    }

    #[test]
    fn fallback_plan_kind_selects_document_or_image_workflow() {
        assert_eq!(
            rust_image_removal_fallback_plan_kind(fallback_plan_input(
                RustArchiveDocumentKind::ComicBook,
                true,
                false,
            )),
            RustImageRemovalFallbackPlanKind::ComicBook
        );
        assert_eq!(
            rust_image_removal_fallback_plan_kind(fallback_plan_input(
                RustArchiveDocumentKind::General,
                true,
                false,
            )),
            RustImageRemovalFallbackPlanKind::NoFallback
        );
        assert_eq!(
            rust_image_removal_fallback_plan_kind(fallback_plan_input(
                RustArchiveDocumentKind::Directory,
                true,
                false,
            )),
            RustImageRemovalFallbackPlanKind::NoFallback
        );
        assert_eq!(
            rust_image_removal_fallback_plan_kind(fallback_plan_input(
                RustArchiveDocumentKind::Empty,
                false,
                true,
            )),
            RustImageRemovalFallbackPlanKind::Image
        );
        assert_eq!(
            rust_image_removal_fallback_plan_kind(fallback_plan_input(
                RustArchiveDocumentKind::Empty,
                false,
                false,
            )),
            RustImageRemovalFallbackPlanKind::NoFallback
        );
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
