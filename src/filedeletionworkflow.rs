// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustFileDeletionTarget {
        DisplayedImage = 0,
        ArchiveDocumentFile = 1,
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

    extern "Rust" {
        #[cxx_name = "rustFileDeletionTarget"]
        fn rust_file_deletion_target(
            displayed_location_inside_archive_document: bool,
        ) -> RustFileDeletionTarget;

        #[cxx_name = "rustFileDeletionCompletionAction"]
        fn rust_file_deletion_completion_action(
            result: RustFileDeletionResult,
        ) -> RustFileDeletionCompletionAction;
    }
}

use ffi::{RustFileDeletionCompletionAction, RustFileDeletionResult, RustFileDeletionTarget};

fn rust_file_deletion_target(
    displayed_location_inside_archive_document: bool,
) -> RustFileDeletionTarget {
    if displayed_location_inside_archive_document {
        RustFileDeletionTarget::ArchiveDocumentFile
    } else {
        RustFileDeletionTarget::DisplayedImage
    }
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn deletion_target_uses_archive_file_for_archive_document_pages() {
        assert_eq!(
            rust_file_deletion_target(false),
            RustFileDeletionTarget::DisplayedImage
        );
        assert_eq!(
            rust_file_deletion_target(true),
            RustFileDeletionTarget::ArchiveDocumentFile
        );
    }

    #[test]
    fn completion_action_routes_success_cancel_and_failure() {
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
}
