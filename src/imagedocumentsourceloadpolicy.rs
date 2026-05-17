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
    enum RustImageDocumentRightToLeftReadingReset {
        Keep = 0,
        ResetInactive = 1,
        ResetActive = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentRightToLeftReadingTransition {
        Keep = 0,
        Reset = 1,
        ResetAndNotifyBeforeSourceState = 2,
        ResetAndNotifyAfterOpen = 3,
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
        right_to_left_reading_reset: RustImageDocumentRightToLeftReadingReset,
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
    RustImageDocumentRightToLeftReadingReset, RustImageDocumentRightToLeftReadingTransition,
    RustImageDocumentSourceLoadKind, RustImageDocumentSourceLoadPlan,
    RustImageDocumentSourceLoadPolicyInput, RustImageDocumentSourceLoadUrlTarget,
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
    plan.right_to_left_reading_transition =
        right_to_left_reading_transition(input.right_to_left_reading_reset, false);
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
    plan.right_to_left_reading_transition =
        right_to_left_reading_transition(input.right_to_left_reading_reset, true);
    plan.clear_secondary_page = true;
    plan.loading_container_navigation_url =
        RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation;
    plan.source_url = RustImageDocumentSourceLoadUrlTarget::RequestedSource;
    plan.begin_open = true;
    plan
}

fn right_to_left_reading_transition(
    reset: RustImageDocumentRightToLeftReadingReset,
    replacement_source: bool,
) -> RustImageDocumentRightToLeftReadingTransition {
    match reset {
        RustImageDocumentRightToLeftReadingReset::Keep => {
            RustImageDocumentRightToLeftReadingTransition::Keep
        }
        RustImageDocumentRightToLeftReadingReset::ResetInactive => {
            RustImageDocumentRightToLeftReadingTransition::Reset
        }
        RustImageDocumentRightToLeftReadingReset::ResetActive => {
            if replacement_source {
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen
            } else {
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState
            }
        }
        _ => RustImageDocumentRightToLeftReadingTransition::Keep,
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
                RustImageDocumentRightToLeftReadingReset::Keep,
                false,
            )),
            source_load_plan(
                false,
                false,
                RustImageDocumentRightToLeftReadingTransition::Keep,
                false,
                RustImageDocumentSourceLoadUrlTarget::Empty,
                RustImageDocumentSourceLoadUrlTarget::Unchanged,
                RustImageDocumentSourceLoadUrlTarget::Unchanged,
                false,
            )
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                false,
                RustImageDocumentRightToLeftReadingReset::ResetActive,
                true,
            )),
            source_load_plan(
                false,
                true,
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState,
                false,
                RustImageDocumentSourceLoadUrlTarget::Empty,
                RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
                RustImageDocumentSourceLoadUrlTarget::Unchanged,
                false,
            )
        );
    }

    #[test]
    fn replacement_source_load_plan_begins_open_and_clears_competing_state() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                RustImageDocumentRightToLeftReadingReset::Keep,
                false,
            )),
            source_load_plan(
                true,
                false,
                RustImageDocumentRightToLeftReadingTransition::Keep,
                true,
                RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
                RustImageDocumentSourceLoadUrlTarget::Unchanged,
                RustImageDocumentSourceLoadUrlTarget::RequestedSource,
                true,
            )
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                false,
                RustImageDocumentRightToLeftReadingReset::ResetActive,
                true,
            )),
            source_load_plan(
                true,
                true,
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen,
                true,
                RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
                RustImageDocumentSourceLoadUrlTarget::Unchanged,
                RustImageDocumentSourceLoadUrlTarget::RequestedSource,
                true,
            )
        );
    }

    fn source_load_policy_input(
        load_kind: RustImageDocumentSourceLoadKind,
        preserve_two_page_spread_transition: bool,
        right_to_left_reading_reset: RustImageDocumentRightToLeftReadingReset,
        has_requested_container_navigation_url: bool,
    ) -> RustImageDocumentSourceLoadPolicyInput {
        RustImageDocumentSourceLoadPolicyInput {
            load_kind,
            preserve_two_page_spread_transition,
            right_to_left_reading_reset,
            has_requested_container_navigation_url,
        }
    }

    fn source_load_plan(
        cancel_navigation_and_predecode: bool,
        finish_spread_transition: bool,
        right_to_left_reading_transition: RustImageDocumentRightToLeftReadingTransition,
        clear_secondary_page: bool,
        loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget,
        container_navigation_url: RustImageDocumentSourceLoadUrlTarget,
        source_url: RustImageDocumentSourceLoadUrlTarget,
        begin_open: bool,
    ) -> RustImageDocumentSourceLoadPlan {
        RustImageDocumentSourceLoadPlan {
            cancel_navigation_and_predecode,
            finish_spread_transition,
            right_to_left_reading_transition,
            clear_secondary_page,
            loading_container_navigation_url,
            container_navigation_url,
            source_url,
            begin_open,
        }
    }
}
