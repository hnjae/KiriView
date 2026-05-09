// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustContainerNavigationCandidateType {
        Unknown = 0,
        Directory = 1,
        ComicBookArchive = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustContainerImageSourceTarget {
        None = 0,
        Directory = 1,
        ArchiveDocument = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustContainerImageSourceError {
        Generic = 0,
        InvalidComicBookArchive = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustContainerImageSourcePlan {
        target: RustContainerImageSourceTarget,
        error: RustContainerImageSourceError,
    }

    extern "Rust" {
        #[cxx_name = "rustContainerImageSourcePlan"]
        fn rust_container_image_source_plan(
            candidate_type: RustContainerNavigationCandidateType,
            archive_document_available: bool,
            archive_document_is_comic_book: bool,
        ) -> RustContainerImageSourcePlan;
    }
}

use ffi::{
    RustContainerImageSourceError, RustContainerImageSourcePlan, RustContainerImageSourceTarget,
    RustContainerNavigationCandidateType,
};

fn rust_container_image_source_plan(
    candidate_type: RustContainerNavigationCandidateType,
    archive_document_available: bool,
    archive_document_is_comic_book: bool,
) -> RustContainerImageSourcePlan {
    match candidate_type {
        RustContainerNavigationCandidateType::Directory => container_image_source_plan(
            RustContainerImageSourceTarget::Directory,
            RustContainerImageSourceError::Generic,
        ),
        RustContainerNavigationCandidateType::ComicBookArchive => {
            if archive_document_available && archive_document_is_comic_book {
                container_image_source_plan(
                    RustContainerImageSourceTarget::ArchiveDocument,
                    RustContainerImageSourceError::Generic,
                )
            } else {
                container_image_source_plan(
                    RustContainerImageSourceTarget::None,
                    RustContainerImageSourceError::InvalidComicBookArchive,
                )
            }
        }
        _ => container_image_source_plan(
            RustContainerImageSourceTarget::None,
            RustContainerImageSourceError::Generic,
        ),
    }
}

fn container_image_source_plan(
    target: RustContainerImageSourceTarget,
    error: RustContainerImageSourceError,
) -> RustContainerImageSourcePlan {
    RustContainerImageSourcePlan { target, error }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn directory_containers_load_directory_images() {
        assert_eq!(
            rust_container_image_source_plan(
                RustContainerNavigationCandidateType::Directory,
                false,
                false,
            ),
            container_image_source_plan(
                RustContainerImageSourceTarget::Directory,
                RustContainerImageSourceError::Generic,
            )
        );
    }

    #[test]
    fn comic_book_archive_containers_require_comic_archive_context() {
        assert_eq!(
            rust_container_image_source_plan(
                RustContainerNavigationCandidateType::ComicBookArchive,
                true,
                true,
            ),
            container_image_source_plan(
                RustContainerImageSourceTarget::ArchiveDocument,
                RustContainerImageSourceError::Generic,
            )
        );
        assert_eq!(
            rust_container_image_source_plan(
                RustContainerNavigationCandidateType::ComicBookArchive,
                true,
                false,
            ),
            container_image_source_plan(
                RustContainerImageSourceTarget::None,
                RustContainerImageSourceError::InvalidComicBookArchive,
            )
        );
        assert_eq!(
            rust_container_image_source_plan(
                RustContainerNavigationCandidateType::ComicBookArchive,
                false,
                false,
            ),
            container_image_source_plan(
                RustContainerImageSourceTarget::None,
                RustContainerImageSourceError::InvalidComicBookArchive,
            )
        );
    }

    #[test]
    fn unknown_container_types_report_generic_error() {
        assert_eq!(
            rust_container_image_source_plan(
                RustContainerNavigationCandidateType::Unknown,
                false,
                false,
            ),
            container_image_source_plan(
                RustContainerImageSourceTarget::None,
                RustContainerImageSourceError::Generic,
            )
        );
    }
}
