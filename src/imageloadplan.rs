// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageLoadArchiveDocumentTarget {
        None = 0,
        SourceArchive = 1,
        ContainerArchive = 2,
        DisplayedArchive = 3,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageLoadPlan {
        archive_document: RustImageLoadArchiveDocumentTarget,
        requires_archive_listing: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustImageLoadPlan"]
        fn rust_image_load_plan(
            source_archive_available: bool,
            container_archive_contains_source: bool,
            displayed_archive_contains_source: bool,
        ) -> RustImageLoadPlan;
    }
}

use ffi::{RustImageLoadArchiveDocumentTarget, RustImageLoadPlan};

fn rust_image_load_plan(
    source_archive_available: bool,
    container_archive_contains_source: bool,
    displayed_archive_contains_source: bool,
) -> RustImageLoadPlan {
    if source_archive_available {
        return image_load_plan(RustImageLoadArchiveDocumentTarget::SourceArchive, true);
    }

    if container_archive_contains_source {
        return image_load_plan(RustImageLoadArchiveDocumentTarget::ContainerArchive, false);
    }

    if displayed_archive_contains_source {
        return image_load_plan(RustImageLoadArchiveDocumentTarget::DisplayedArchive, false);
    }

    image_load_plan(RustImageLoadArchiveDocumentTarget::None, false)
}

fn image_load_plan(
    archive_document: RustImageLoadArchiveDocumentTarget,
    requires_archive_listing: bool,
) -> RustImageLoadPlan {
    RustImageLoadPlan {
        archive_document,
        requires_archive_listing,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn source_archive_requires_listing_and_wins_over_existing_contexts() {
        let plan = rust_image_load_plan(true, true, true);

        assert_eq!(
            plan.archive_document,
            RustImageLoadArchiveDocumentTarget::SourceArchive
        );
        assert!(plan.requires_archive_listing);
    }

    #[test]
    fn container_archive_restores_context_before_displayed_context() {
        let plan = rust_image_load_plan(false, true, true);

        assert_eq!(
            plan.archive_document,
            RustImageLoadArchiveDocumentTarget::ContainerArchive
        );
        assert!(!plan.requires_archive_listing);
    }

    #[test]
    fn displayed_archive_context_is_kept_for_interior_images() {
        let plan = rust_image_load_plan(false, false, true);

        assert_eq!(
            plan.archive_document,
            RustImageLoadArchiveDocumentTarget::DisplayedArchive
        );
        assert!(!plan.requires_archive_listing);
    }

    #[test]
    fn missing_archive_context_plans_direct_image_load() {
        let plan = rust_image_load_plan(false, false, false);

        assert_eq!(
            plan.archive_document,
            RustImageLoadArchiveDocumentTarget::None
        );
        assert!(!plan.requires_archive_listing);
    }
}
