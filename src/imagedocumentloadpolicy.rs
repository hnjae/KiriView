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
    struct RustImageDocumentSourceLoadPolicyInput {
        load_kind: RustImageDocumentSourceLoadKind,
        preserve_two_page_spread_transition: bool,
        right_to_left_reading_reset: RustImageDocumentRightToLeftReadingReset,
        has_requested_container_navigation_url: bool,
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
    RustImageDocumentRightToLeftReadingReset, RustImageDocumentSourceLoadAction,
    RustImageDocumentSourceLoadKind, RustImageDocumentSourceLoadPlan,
    RustImageDocumentSourceLoadPolicyInput,
};

fn rust_image_document_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    let mut plan = RustImageDocumentSourceLoadPlan {
        actions: Vec::new(),
    };

    append_initial_source_load_actions(&mut plan, input);
    if replaces_source(input) {
        append_changed_source_load_actions(&mut plan, input);
    } else {
        append_unchanged_source_load_actions(&mut plan, input);
    }

    plan
}

fn replaces_source(input: RustImageDocumentSourceLoadPolicyInput) -> bool {
    input.load_kind == RustImageDocumentSourceLoadKind::ReplacementSource
}

fn resets_right_to_left_reading(input: RustImageDocumentSourceLoadPolicyInput) -> bool {
    input.right_to_left_reading_reset != RustImageDocumentRightToLeftReadingReset::Keep
}

fn notifies_right_to_left_reading(input: RustImageDocumentSourceLoadPolicyInput) -> bool {
    input.right_to_left_reading_reset == RustImageDocumentRightToLeftReadingReset::ResetActive
}

fn append_initial_source_load_actions(
    plan: &mut RustImageDocumentSourceLoadPlan,
    input: RustImageDocumentSourceLoadPolicyInput,
) {
    if replaces_source(input) {
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
    if input.has_requested_container_navigation_url {
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

    #[test]
    fn source_load_plan_routes_unchanged_and_replacement_loads() {
        let unchanged = rust_image_document_source_load_plan(source_load_input(
            RustImageDocumentSourceLoadKind::CurrentSource,
            false,
            RustImageDocumentRightToLeftReadingReset::ResetActive,
            true,
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

        let replacement = rust_image_document_source_load_plan(source_load_input(
            RustImageDocumentSourceLoadKind::ReplacementSource,
            true,
            RustImageDocumentRightToLeftReadingReset::Keep,
            false,
        ));
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

        let inactive_reset_replacement = rust_image_document_source_load_plan(source_load_input(
            RustImageDocumentSourceLoadKind::ReplacementSource,
            true,
            RustImageDocumentRightToLeftReadingReset::ResetInactive,
            false,
        ));
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

        let resetting_replacement = rust_image_document_source_load_plan(source_load_input(
            RustImageDocumentSourceLoadKind::ReplacementSource,
            true,
            RustImageDocumentRightToLeftReadingReset::ResetActive,
            false,
        ));
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
