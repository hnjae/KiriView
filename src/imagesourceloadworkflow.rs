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
    struct RustImageSourceLoadRequest {
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
            request: RustImageSourceLoadRequest,
        ) -> RustImageSourceLoadPlan;
    }
}

use ffi::{RustImageSourceLoadAction, RustImageSourceLoadPlan, RustImageSourceLoadRequest};

fn rust_image_source_load_plan(request: RustImageSourceLoadRequest) -> RustImageSourceLoadPlan {
    let mut plan = empty_source_load_plan();
    if request.source_url_changed {
        plan.actions
            .push(RustImageSourceLoadAction::CancelNavigationAndPredecode);
    }
    if !request.preserve_two_page_spread_transition {
        plan.actions
            .push(RustImageSourceLoadAction::FinishSpreadTransition);
    }
    if request.reset_right_to_left_reading {
        plan.actions
            .push(RustImageSourceLoadAction::ResetRightToLeftReading);
    }

    let notify_right_to_left_reading =
        request.reset_right_to_left_reading && request.right_to_left_reading_enabled;

    if !request.source_url_changed {
        if notify_right_to_left_reading {
            plan.actions
                .push(RustImageSourceLoadAction::NotifyRightToLeftReading);
        }
        plan.actions
            .push(RustImageSourceLoadAction::ClearLoadingContainerNavigationUrl);
        if !request.container_navigation_url_empty {
            plan.actions
                .push(RustImageSourceLoadAction::UpdateContainerNavigationUrl);
        }
        return plan;
    }

    plan.actions
        .push(RustImageSourceLoadAction::ClearSecondaryPage);
    plan.actions
        .push(RustImageSourceLoadAction::SetLoadingContainerNavigationUrl);
    plan.actions.push(RustImageSourceLoadAction::SetSourceUrl);
    plan.actions.push(RustImageSourceLoadAction::BeginOpen);
    if notify_right_to_left_reading {
        plan.actions
            .push(RustImageSourceLoadAction::NotifyRightToLeftReading);
    }
    plan
}

fn empty_source_load_plan() -> RustImageSourceLoadPlan {
    RustImageSourceLoadPlan {
        actions: Vec::new(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn source_load_request(
        source_url_changed: bool,
        preserve_two_page_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_enabled: bool,
        container_navigation_url_empty: bool,
    ) -> RustImageSourceLoadRequest {
        RustImageSourceLoadRequest {
            source_url_changed,
            preserve_two_page_spread_transition,
            reset_right_to_left_reading,
            right_to_left_reading_enabled,
            container_navigation_url_empty,
        }
    }

    #[test]
    fn unchanged_source_load_clears_loading_and_optionally_updates_container() {
        let plan =
            rust_image_source_load_plan(source_load_request(false, false, true, true, false));

        assert_eq!(
            plan.actions,
            vec![
                RustImageSourceLoadAction::FinishSpreadTransition,
                RustImageSourceLoadAction::ResetRightToLeftReading,
                RustImageSourceLoadAction::NotifyRightToLeftReading,
                RustImageSourceLoadAction::ClearLoadingContainerNavigationUrl,
                RustImageSourceLoadAction::UpdateContainerNavigationUrl,
            ]
        );
    }

    #[test]
    fn changed_source_load_starts_new_load_without_container_navigation_update() {
        let plan =
            rust_image_source_load_plan(source_load_request(true, false, false, true, false));

        assert_eq!(
            plan.actions,
            vec![
                RustImageSourceLoadAction::CancelNavigationAndPredecode,
                RustImageSourceLoadAction::FinishSpreadTransition,
                RustImageSourceLoadAction::ClearSecondaryPage,
                RustImageSourceLoadAction::SetLoadingContainerNavigationUrl,
                RustImageSourceLoadAction::SetSourceUrl,
                RustImageSourceLoadAction::BeginOpen,
            ]
        );
    }

    #[test]
    fn source_load_plan_can_preserve_active_spread_transition() {
        let plan = rust_image_source_load_plan(source_load_request(true, true, true, true, true));

        assert_eq!(
            plan.actions,
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

    #[test]
    fn source_load_plan_notifies_only_when_right_to_left_reading_was_enabled() {
        let plan =
            rust_image_source_load_plan(source_load_request(false, false, true, false, false));

        assert_eq!(
            plan.actions,
            vec![
                RustImageSourceLoadAction::FinishSpreadTransition,
                RustImageSourceLoadAction::ResetRightToLeftReading,
                RustImageSourceLoadAction::ClearLoadingContainerNavigationUrl,
                RustImageSourceLoadAction::UpdateContainerNavigationUrl,
            ]
        );
    }
}
