// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentSourceLoadKind {
        CurrentSource = 0,
        ReplacementSource = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentRightToLeftReadingTransition {
        Keep = 0,
        ResetAndNotifyBeforeSourceState = 1,
        ResetAndNotifyAfterOpen = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentSourceLoadUrlTarget {
        Unchanged = 0,
        Empty = 1,
        RequestedContainerNavigation = 2,
        RequestedSource = 3,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentSourceLoadPolicyInput {
        load_kind: RustImageDocumentSourceLoadKind,
        preserve_two_page_spread_transition: bool,
        right_to_left_reading_enabled: bool,
        source_within_displayed_comic_book_archive: bool,
        has_requested_container_navigation_url: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentSourceLoadPlan {
        cancel_navigation_and_predecode: bool,
        finish_spread_transition: bool,
        right_to_left_reading_transition: RustImageDocumentRightToLeftReadingTransition,
        clear_secondary_page: bool,
        loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget,
        container_navigation_url: RustImageDocumentSourceLoadUrlTarget,
        source_url: RustImageDocumentSourceLoadUrlTarget,
        begin_open: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustImageDocumentSourceLoadPlan"]
        fn rust_image_document_source_load_plan(
            input: RustImageDocumentSourceLoadPolicyInput,
        ) -> RustImageDocumentSourceLoadPlan;
    }
}

use ffi::{
    RustImageDocumentRightToLeftReadingTransition, RustImageDocumentSourceLoadKind,
    RustImageDocumentSourceLoadPlan, RustImageDocumentSourceLoadPolicyInput,
    RustImageDocumentSourceLoadUrlTarget,
};

fn rust_image_document_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    match input.load_kind {
        RustImageDocumentSourceLoadKind::CurrentSource => current_source_load_plan(input),
        RustImageDocumentSourceLoadKind::ReplacementSource => replacement_source_load_plan(input),
        _ => current_source_load_plan(input),
    }
}

fn current_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    let mut plan = empty_source_load_plan();
    plan.finish_spread_transition = !input.preserve_two_page_spread_transition;
    plan.right_to_left_reading_transition = right_to_left_reading_transition(input, false);
    plan.loading_container_navigation_url = RustImageDocumentSourceLoadUrlTarget::Empty;
    if input.has_requested_container_navigation_url {
        plan.container_navigation_url =
            RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation;
    }
    plan
}

fn replacement_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    let mut plan = empty_source_load_plan();
    plan.cancel_navigation_and_predecode = true;
    plan.finish_spread_transition = !input.preserve_two_page_spread_transition;
    plan.right_to_left_reading_transition = right_to_left_reading_transition(input, true);
    plan.clear_secondary_page = true;
    plan.loading_container_navigation_url =
        RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation;
    plan.source_url = RustImageDocumentSourceLoadUrlTarget::RequestedSource;
    plan.begin_open = true;
    plan
}

fn right_to_left_reading_transition(
    input: RustImageDocumentSourceLoadPolicyInput,
    replacement_source: bool,
) -> RustImageDocumentRightToLeftReadingTransition {
    if !input.right_to_left_reading_enabled
        || input.has_requested_container_navigation_url
        || input.source_within_displayed_comic_book_archive
    {
        return RustImageDocumentRightToLeftReadingTransition::Keep;
    }

    if replacement_source {
        RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen
    } else {
        RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState
    }
}

fn empty_source_load_plan() -> RustImageDocumentSourceLoadPlan {
    RustImageDocumentSourceLoadPlan {
        cancel_navigation_and_predecode: false,
        finish_spread_transition: false,
        right_to_left_reading_transition: RustImageDocumentRightToLeftReadingTransition::Keep,
        clear_secondary_page: false,
        loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
        container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
        source_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
        begin_open: false,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn current_source_load_plan_uses_reading_and_container_snapshots() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                true,
                false,
                false,
                false,
            )),
            RustImageDocumentSourceLoadPlan {
                loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Empty,
                ..empty_source_load_plan()
            }
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                false,
                true,
                false,
                true,
            )),
            RustImageDocumentSourceLoadPlan {
                finish_spread_transition: true,
                loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Empty,
                container_navigation_url:
                    RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
                ..empty_source_load_plan()
            }
        );
    }

    #[test]
    fn replacement_source_load_plan_begins_open_and_clears_competing_state() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                false,
                false,
                false,
            )),
            RustImageDocumentSourceLoadPlan {
                cancel_navigation_and_predecode: true,
                clear_secondary_page: true,
                loading_container_navigation_url:
                    RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
                source_url: RustImageDocumentSourceLoadUrlTarget::RequestedSource,
                begin_open: true,
                ..empty_source_load_plan()
            }
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                false,
                true,
                false,
                true,
            )),
            RustImageDocumentSourceLoadPlan {
                cancel_navigation_and_predecode: true,
                finish_spread_transition: true,
                clear_secondary_page: true,
                loading_container_navigation_url:
                    RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
                source_url: RustImageDocumentSourceLoadUrlTarget::RequestedSource,
                begin_open: true,
                ..empty_source_load_plan()
            }
        );
    }

    #[test]
    fn right_to_left_reading_reset_is_decided_from_source_load_context() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                true,
                true,
                false,
                false,
            ))
            .right_to_left_reading_transition,
            RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                true,
                false,
                false,
            ))
            .right_to_left_reading_transition,
            RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                true,
                true,
                false,
            ))
            .right_to_left_reading_transition,
            RustImageDocumentRightToLeftReadingTransition::Keep
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                true,
                false,
                true,
            ))
            .right_to_left_reading_transition,
            RustImageDocumentRightToLeftReadingTransition::Keep
        );
    }

    fn source_load_policy_input(
        load_kind: RustImageDocumentSourceLoadKind,
        preserve_two_page_spread_transition: bool,
        right_to_left_reading_enabled: bool,
        source_within_displayed_comic_book_archive: bool,
        has_requested_container_navigation_url: bool,
    ) -> RustImageDocumentSourceLoadPolicyInput {
        RustImageDocumentSourceLoadPolicyInput {
            load_kind,
            preserve_two_page_spread_transition,
            right_to_left_reading_enabled,
            source_within_displayed_comic_book_archive,
            has_requested_container_navigation_url,
        }
    }
}
