// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageSourceLoadAction {
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
    struct RustImageSourceLoadPolicyInput {
        source_url_changed: bool,
        preserve_two_page_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_enabled: bool,
        container_navigation_url_empty: bool,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustImageSourceLoadPlan {
        actions: Vec<RustImageSourceLoadAction>,
    }

    extern "Rust" {
        #[cxx_name = "rustImageSourceLoadPlan"]
        fn rust_image_source_load_plan(
            input: RustImageSourceLoadPolicyInput,
        ) -> RustImageSourceLoadPlan;
    }
}

use ffi::{RustImageSourceLoadAction, RustImageSourceLoadPlan, RustImageSourceLoadPolicyInput};

fn rust_image_source_load_plan(input: RustImageSourceLoadPolicyInput) -> RustImageSourceLoadPlan {
    let mut plan = RustImageSourceLoadPlan {
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

fn resets_right_to_left_reading(input: RustImageSourceLoadPolicyInput) -> bool {
    input.reset_right_to_left_reading
}

fn notifies_right_to_left_reading(input: RustImageSourceLoadPolicyInput) -> bool {
    input.reset_right_to_left_reading && input.right_to_left_reading_enabled
}

fn append_initial_source_load_actions(
    plan: &mut RustImageSourceLoadPlan,
    input: RustImageSourceLoadPolicyInput,
) {
    if input.source_url_changed {
        plan.actions
            .push(RustImageSourceLoadAction::CancelNavigationAndPredecode);
    }
    if !input.preserve_two_page_spread_transition {
        plan.actions
            .push(RustImageSourceLoadAction::FinishSpreadTransition);
    }
    if resets_right_to_left_reading(input) {
        plan.actions
            .push(RustImageSourceLoadAction::ResetRightToLeftReading);
    }
}

fn append_unchanged_source_load_actions(
    plan: &mut RustImageSourceLoadPlan,
    input: RustImageSourceLoadPolicyInput,
) {
    if notifies_right_to_left_reading(input) {
        plan.actions
            .push(RustImageSourceLoadAction::NotifyRightToLeftReading);
    }
    plan.actions
        .push(RustImageSourceLoadAction::ClearLoadingContainerNavigationUrl);
    if !input.container_navigation_url_empty {
        plan.actions
            .push(RustImageSourceLoadAction::UpdateContainerNavigationUrl);
    }
}

fn append_changed_source_load_actions(
    plan: &mut RustImageSourceLoadPlan,
    input: RustImageSourceLoadPolicyInput,
) {
    plan.actions
        .push(RustImageSourceLoadAction::ClearSecondaryPage);
    plan.actions
        .push(RustImageSourceLoadAction::SetLoadingContainerNavigationUrl);
    plan.actions.push(RustImageSourceLoadAction::SetSourceUrl);
    plan.actions.push(RustImageSourceLoadAction::BeginOpen);
    if notifies_right_to_left_reading(input) {
        plan.actions
            .push(RustImageSourceLoadAction::NotifyRightToLeftReading);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn source_load_input(
        source_url_changed: bool,
        preserve_two_page_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_enabled: bool,
        container_navigation_url_empty: bool,
    ) -> RustImageSourceLoadPolicyInput {
        RustImageSourceLoadPolicyInput {
            source_url_changed,
            preserve_two_page_spread_transition,
            reset_right_to_left_reading,
            right_to_left_reading_enabled,
            container_navigation_url_empty,
        }
    }

    #[test]
    fn source_load_plan_routes_unchanged_and_replacement_loads() {
        let unchanged =
            rust_image_source_load_plan(source_load_input(false, false, true, true, false));
        assert_eq!(
            unchanged.actions,
            vec![
                RustImageSourceLoadAction::FinishSpreadTransition,
                RustImageSourceLoadAction::ResetRightToLeftReading,
                RustImageSourceLoadAction::NotifyRightToLeftReading,
                RustImageSourceLoadAction::ClearLoadingContainerNavigationUrl,
                RustImageSourceLoadAction::UpdateContainerNavigationUrl,
            ]
        );

        let replacement =
            rust_image_source_load_plan(source_load_input(true, true, false, true, true));
        assert_eq!(
            replacement.actions,
            vec![
                RustImageSourceLoadAction::CancelNavigationAndPredecode,
                RustImageSourceLoadAction::ClearSecondaryPage,
                RustImageSourceLoadAction::SetLoadingContainerNavigationUrl,
                RustImageSourceLoadAction::SetSourceUrl,
                RustImageSourceLoadAction::BeginOpen,
            ]
        );

        let inactive_reset_replacement =
            rust_image_source_load_plan(source_load_input(true, true, true, false, true));
        assert_eq!(
            inactive_reset_replacement.actions,
            vec![
                RustImageSourceLoadAction::CancelNavigationAndPredecode,
                RustImageSourceLoadAction::ResetRightToLeftReading,
                RustImageSourceLoadAction::ClearSecondaryPage,
                RustImageSourceLoadAction::SetLoadingContainerNavigationUrl,
                RustImageSourceLoadAction::SetSourceUrl,
                RustImageSourceLoadAction::BeginOpen,
            ]
        );

        let resetting_replacement =
            rust_image_source_load_plan(source_load_input(true, true, true, true, true));
        assert_eq!(
            resetting_replacement.actions,
            vec![
                RustImageSourceLoadAction::CancelNavigationAndPredecode,
                RustImageSourceLoadAction::ResetRightToLeftReading,
                RustImageSourceLoadAction::ClearSecondaryPage,
                RustImageSourceLoadAction::SetLoadingContainerNavigationUrl,
                RustImageSourceLoadAction::SetSourceUrl,
                RustImageSourceLoadAction::BeginOpen,
                RustImageSourceLoadAction::NotifyRightToLeftReading,
            ]
        );
    }
}
