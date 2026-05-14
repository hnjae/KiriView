// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum ImageSourceLoadAction {
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
    struct ImageSourceLoadRequest {
        source_url_changed: bool,
        preserve_two_page_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_enabled: bool,
        container_navigation_url_empty: bool,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct ImageSourceLoadPlan {
        actions: Vec<ImageSourceLoadAction>,
    }

    extern "Rust" {
        #[cxx_name = "rustImageSourceLoadPlan"]
        fn rust_image_source_load_plan(request: ImageSourceLoadRequest) -> ImageSourceLoadPlan;
    }
}

use ffi::{ImageSourceLoadAction, ImageSourceLoadPlan, ImageSourceLoadRequest};

fn rust_image_source_load_plan(request: ImageSourceLoadRequest) -> ImageSourceLoadPlan {
    let mut plan = ImageSourceLoadPlan {
        actions: Vec::new(),
    };

    append_initial_load_actions(&mut plan, request);
    if request.source_url_changed {
        append_changed_source_actions(&mut plan, request);
    } else {
        append_unchanged_source_actions(&mut plan, request);
    }

    plan
}

fn resets_right_to_left_reading(request: ImageSourceLoadRequest) -> bool {
    request.reset_right_to_left_reading
}

fn notifies_right_to_left_reading(request: ImageSourceLoadRequest) -> bool {
    request.reset_right_to_left_reading && request.right_to_left_reading_enabled
}

fn append_initial_load_actions(plan: &mut ImageSourceLoadPlan, request: ImageSourceLoadRequest) {
    if request.source_url_changed {
        plan.actions
            .push(ImageSourceLoadAction::CancelNavigationAndPredecode);
    }
    if !request.preserve_two_page_spread_transition {
        plan.actions
            .push(ImageSourceLoadAction::FinishSpreadTransition);
    }
    if resets_right_to_left_reading(request) {
        plan.actions
            .push(ImageSourceLoadAction::ResetRightToLeftReading);
    }
}

fn append_unchanged_source_actions(
    plan: &mut ImageSourceLoadPlan,
    request: ImageSourceLoadRequest,
) {
    if notifies_right_to_left_reading(request) {
        plan.actions
            .push(ImageSourceLoadAction::NotifyRightToLeftReading);
    }
    plan.actions
        .push(ImageSourceLoadAction::ClearLoadingContainerNavigationUrl);
    if !request.container_navigation_url_empty {
        plan.actions
            .push(ImageSourceLoadAction::UpdateContainerNavigationUrl);
    }
}

fn append_changed_source_actions(plan: &mut ImageSourceLoadPlan, request: ImageSourceLoadRequest) {
    plan.actions.push(ImageSourceLoadAction::ClearSecondaryPage);
    plan.actions
        .push(ImageSourceLoadAction::SetLoadingContainerNavigationUrl);
    plan.actions.push(ImageSourceLoadAction::SetSourceUrl);
    plan.actions.push(ImageSourceLoadAction::BeginOpen);
    if notifies_right_to_left_reading(request) {
        plan.actions
            .push(ImageSourceLoadAction::NotifyRightToLeftReading);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn request(
        source_url_changed: bool,
        preserve_two_page_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_enabled: bool,
        container_navigation_url_empty: bool,
    ) -> ImageSourceLoadRequest {
        ImageSourceLoadRequest {
            source_url_changed,
            preserve_two_page_spread_transition,
            reset_right_to_left_reading,
            right_to_left_reading_enabled,
            container_navigation_url_empty,
        }
    }

    #[test]
    fn routes_unchanged_and_replacement_loads() {
        let unchanged = rust_image_source_load_plan(request(false, false, true, true, false));
        assert_eq!(
            unchanged.actions,
            vec![
                ImageSourceLoadAction::FinishSpreadTransition,
                ImageSourceLoadAction::ResetRightToLeftReading,
                ImageSourceLoadAction::NotifyRightToLeftReading,
                ImageSourceLoadAction::ClearLoadingContainerNavigationUrl,
                ImageSourceLoadAction::UpdateContainerNavigationUrl,
            ]
        );

        let replacement = rust_image_source_load_plan(request(true, true, false, true, true));
        assert_eq!(
            replacement.actions,
            vec![
                ImageSourceLoadAction::CancelNavigationAndPredecode,
                ImageSourceLoadAction::ClearSecondaryPage,
                ImageSourceLoadAction::SetLoadingContainerNavigationUrl,
                ImageSourceLoadAction::SetSourceUrl,
                ImageSourceLoadAction::BeginOpen,
            ]
        );

        let inactive_reset_replacement =
            rust_image_source_load_plan(request(true, true, true, false, true));
        assert_eq!(
            inactive_reset_replacement.actions,
            vec![
                ImageSourceLoadAction::CancelNavigationAndPredecode,
                ImageSourceLoadAction::ResetRightToLeftReading,
                ImageSourceLoadAction::ClearSecondaryPage,
                ImageSourceLoadAction::SetLoadingContainerNavigationUrl,
                ImageSourceLoadAction::SetSourceUrl,
                ImageSourceLoadAction::BeginOpen,
            ]
        );

        let resetting_replacement =
            rust_image_source_load_plan(request(true, true, true, true, true));
        assert_eq!(
            resetting_replacement.actions,
            vec![
                ImageSourceLoadAction::CancelNavigationAndPredecode,
                ImageSourceLoadAction::ResetRightToLeftReading,
                ImageSourceLoadAction::ClearSecondaryPage,
                ImageSourceLoadAction::SetLoadingContainerNavigationUrl,
                ImageSourceLoadAction::SetSourceUrl,
                ImageSourceLoadAction::BeginOpen,
                ImageSourceLoadAction::NotifyRightToLeftReading,
            ]
        );
    }
}
