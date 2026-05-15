// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentSourceLoadAction {
        CancelNavigationAndPredecode = 0,
        FinishSpreadTransition = 1,
        ResetRightToLeftReading = 2,
        NotifyRightToLeftReading = 3,
        ClearSecondaryPage = 4,
        ClearLoadingContainerNavigationUrl = 5,
        UpdateContainerNavigationUrl = 6,
        SetLoadingContainerNavigationUrl = 7,
        SetSourceUrl = 8,
        BeginOpen = 9,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentSourceLoadPolicyInput {
        source_url_changed: bool,
        preserve_two_page_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_was_enabled: bool,
        requested_container_navigation_url_empty: bool,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustImageDocumentSourceLoadPlan {
        actions: Vec<RustImageDocumentSourceLoadAction>,
    }

    extern "Rust" {
        #[cxx_name = "rustImageDocumentSourceLoadPlan"]
        fn rust_image_document_source_load_plan(
            input: RustImageDocumentSourceLoadPolicyInput,
        ) -> RustImageDocumentSourceLoadPlan;
    }
}

use ffi::{
    RustImageDocumentSourceLoadAction, RustImageDocumentSourceLoadPlan,
    RustImageDocumentSourceLoadPolicyInput,
};

fn rust_image_document_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    let mut plan = RustImageDocumentSourceLoadPlan {
        actions: Vec::new(),
    };

    append_initial_source_load_actions(&mut plan, input);
    if input.source_url_changed {
        append_changed_source_load_actions(&mut plan, input);
    } else {
        append_unchanged_source_load_actions(&mut plan, input);
    }

    plan
}

fn resets_right_to_left_reading(input: RustImageDocumentSourceLoadPolicyInput) -> bool {
    input.reset_right_to_left_reading
}

fn notifies_right_to_left_reading(input: RustImageDocumentSourceLoadPolicyInput) -> bool {
    input.reset_right_to_left_reading && input.right_to_left_reading_was_enabled
}

fn append_initial_source_load_actions(
    plan: &mut RustImageDocumentSourceLoadPlan,
    input: RustImageDocumentSourceLoadPolicyInput,
) {
    if input.source_url_changed {
        plan.actions
            .push(RustImageDocumentSourceLoadAction::CancelNavigationAndPredecode);
    }
    if !input.preserve_two_page_spread_transition {
        plan.actions
            .push(RustImageDocumentSourceLoadAction::FinishSpreadTransition);
    }
    if resets_right_to_left_reading(input) {
        plan.actions
            .push(RustImageDocumentSourceLoadAction::ResetRightToLeftReading);
    }
}

fn append_unchanged_source_load_actions(
    plan: &mut RustImageDocumentSourceLoadPlan,
    input: RustImageDocumentSourceLoadPolicyInput,
) {
    if notifies_right_to_left_reading(input) {
        plan.actions
            .push(RustImageDocumentSourceLoadAction::NotifyRightToLeftReading);
    }
    plan.actions
        .push(RustImageDocumentSourceLoadAction::ClearLoadingContainerNavigationUrl);
    if !input.requested_container_navigation_url_empty {
        plan.actions
            .push(RustImageDocumentSourceLoadAction::UpdateContainerNavigationUrl);
    }
}

fn append_changed_source_load_actions(
    plan: &mut RustImageDocumentSourceLoadPlan,
    input: RustImageDocumentSourceLoadPolicyInput,
) {
    plan.actions
        .push(RustImageDocumentSourceLoadAction::ClearSecondaryPage);
    plan.actions
        .push(RustImageDocumentSourceLoadAction::SetLoadingContainerNavigationUrl);
    plan.actions
        .push(RustImageDocumentSourceLoadAction::SetSourceUrl);
    plan.actions
        .push(RustImageDocumentSourceLoadAction::BeginOpen);
    if notifies_right_to_left_reading(input) {
        plan.actions
            .push(RustImageDocumentSourceLoadAction::NotifyRightToLeftReading);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn source_load_input(
        source_url_changed: bool,
        preserve_two_page_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_was_enabled: bool,
        requested_container_navigation_url_empty: bool,
    ) -> RustImageDocumentSourceLoadPolicyInput {
        RustImageDocumentSourceLoadPolicyInput {
            source_url_changed,
            preserve_two_page_spread_transition,
            reset_right_to_left_reading,
            right_to_left_reading_was_enabled,
            requested_container_navigation_url_empty,
        }
    }

    #[test]
    fn source_load_plan_routes_unchanged_and_replacement_loads() {
        let unchanged = rust_image_document_source_load_plan(source_load_input(
            false, false, true, true, false,
        ));
        assert_eq!(
            unchanged.actions,
            vec![
                RustImageDocumentSourceLoadAction::FinishSpreadTransition,
                RustImageDocumentSourceLoadAction::ResetRightToLeftReading,
                RustImageDocumentSourceLoadAction::NotifyRightToLeftReading,
                RustImageDocumentSourceLoadAction::ClearLoadingContainerNavigationUrl,
                RustImageDocumentSourceLoadAction::UpdateContainerNavigationUrl,
            ]
        );

        let replacement =
            rust_image_document_source_load_plan(source_load_input(true, true, false, true, true));
        assert_eq!(
            replacement.actions,
            vec![
                RustImageDocumentSourceLoadAction::CancelNavigationAndPredecode,
                RustImageDocumentSourceLoadAction::ClearSecondaryPage,
                RustImageDocumentSourceLoadAction::SetLoadingContainerNavigationUrl,
                RustImageDocumentSourceLoadAction::SetSourceUrl,
                RustImageDocumentSourceLoadAction::BeginOpen,
            ]
        );

        let inactive_reset_replacement =
            rust_image_document_source_load_plan(source_load_input(true, true, true, false, true));
        assert_eq!(
            inactive_reset_replacement.actions,
            vec![
                RustImageDocumentSourceLoadAction::CancelNavigationAndPredecode,
                RustImageDocumentSourceLoadAction::ResetRightToLeftReading,
                RustImageDocumentSourceLoadAction::ClearSecondaryPage,
                RustImageDocumentSourceLoadAction::SetLoadingContainerNavigationUrl,
                RustImageDocumentSourceLoadAction::SetSourceUrl,
                RustImageDocumentSourceLoadAction::BeginOpen,
            ]
        );

        let resetting_replacement =
            rust_image_document_source_load_plan(source_load_input(true, true, true, true, true));
        assert_eq!(
            resetting_replacement.actions,
            vec![
                RustImageDocumentSourceLoadAction::CancelNavigationAndPredecode,
                RustImageDocumentSourceLoadAction::ResetRightToLeftReading,
                RustImageDocumentSourceLoadAction::ClearSecondaryPage,
                RustImageDocumentSourceLoadAction::SetLoadingContainerNavigationUrl,
                RustImageDocumentSourceLoadAction::SetSourceUrl,
                RustImageDocumentSourceLoadAction::BeginOpen,
                RustImageDocumentSourceLoadAction::NotifyRightToLeftReading,
            ]
        );
    }
}
