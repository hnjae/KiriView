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

    #[derive(Debug, PartialEq, Eq)]
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
    if replaces_source(input) {
        return replacement_source_load_plan(input);
    }

    current_source_load_plan(input)
}

fn replaces_source(input: RustImageDocumentSourceLoadPolicyInput) -> bool {
    input.load_kind == RustImageDocumentSourceLoadKind::ReplacementSource
}

fn current_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    RustImageDocumentSourceLoadPlan {
        cancel_navigation_and_predecode: false,
        finish_spread_transition: !input.preserve_two_page_spread_transition,
        right_to_left_reading_transition: right_to_left_reading_transition(input, false),
        clear_secondary_page: false,
        loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Empty,
        container_navigation_url: if input.has_requested_container_navigation_url {
            RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation
        } else {
            RustImageDocumentSourceLoadUrlTarget::Unchanged
        },
        source_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
        begin_open: false,
    }
}

fn replacement_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    RustImageDocumentSourceLoadPlan {
        cancel_navigation_and_predecode: true,
        finish_spread_transition: !input.preserve_two_page_spread_transition,
        right_to_left_reading_transition: right_to_left_reading_transition(input, true),
        clear_secondary_page: true,
        loading_container_navigation_url:
            RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
        container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
        source_url: RustImageDocumentSourceLoadUrlTarget::RequestedSource,
        begin_open: true,
    }
}

fn right_to_left_reading_transition(
    input: RustImageDocumentSourceLoadPolicyInput,
    replacement_source: bool,
) -> RustImageDocumentRightToLeftReadingTransition {
    match input.right_to_left_reading_reset {
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

#[cfg(test)]
mod tests {
    use super::*;

    fn source_load_input(
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

    fn current_source_plan(
        finish_spread_transition: bool,
        right_to_left_reading_transition: RustImageDocumentRightToLeftReadingTransition,
        container_navigation_url: RustImageDocumentSourceLoadUrlTarget,
    ) -> RustImageDocumentSourceLoadPlan {
        RustImageDocumentSourceLoadPlan {
            cancel_navigation_and_predecode: false,
            finish_spread_transition,
            right_to_left_reading_transition,
            clear_secondary_page: false,
            loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Empty,
            container_navigation_url,
            source_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
            begin_open: false,
        }
    }

    fn replacement_source_plan(
        finish_spread_transition: bool,
        right_to_left_reading_transition: RustImageDocumentRightToLeftReadingTransition,
    ) -> RustImageDocumentSourceLoadPlan {
        RustImageDocumentSourceLoadPlan {
            cancel_navigation_and_predecode: true,
            finish_spread_transition,
            right_to_left_reading_transition,
            clear_secondary_page: true,
            loading_container_navigation_url:
                RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
            container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
            source_url: RustImageDocumentSourceLoadUrlTarget::RequestedSource,
            begin_open: true,
        }
    }

    #[test]
    fn source_load_plan_routes_unchanged_and_replacement_loads() {
        let unchanged = rust_image_document_source_load_plan(source_load_input(
            RustImageDocumentSourceLoadKind::CurrentSource,
            false,
            RustImageDocumentRightToLeftReadingReset::ResetActive,
            true,
        ));
        assert_eq!(
            unchanged,
            current_source_plan(
                true,
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState,
                RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
            )
        );

        let replacement = rust_image_document_source_load_plan(source_load_input(
            RustImageDocumentSourceLoadKind::ReplacementSource,
            true,
            RustImageDocumentRightToLeftReadingReset::Keep,
            false,
        ));
        assert_eq!(
            replacement,
            replacement_source_plan(false, RustImageDocumentRightToLeftReadingTransition::Keep)
        );

        let inactive_reset_replacement = rust_image_document_source_load_plan(source_load_input(
            RustImageDocumentSourceLoadKind::ReplacementSource,
            true,
            RustImageDocumentRightToLeftReadingReset::ResetInactive,
            false,
        ));
        assert_eq!(
            inactive_reset_replacement,
            replacement_source_plan(false, RustImageDocumentRightToLeftReadingTransition::Reset)
        );

        let resetting_replacement = rust_image_document_source_load_plan(source_load_input(
            RustImageDocumentSourceLoadKind::ReplacementSource,
            true,
            RustImageDocumentRightToLeftReadingReset::ResetActive,
            false,
        ));
        assert_eq!(
            resetting_replacement,
            replacement_source_plan(
                false,
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen
            )
        );
    }
}
