// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum ImageOpenBoolTarget {
        Unchanged = 0,
        False = 1,
        True = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum ImageOpenStatusTarget {
        Unchanged = 0,
        Null = 1,
        Loading = 2,
        Ready = 3,
        Error = 4,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum ImageOpenErrorStringTarget {
        Unchanged = 0,
        Clear = 1,
        Provided = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum ImageOpenUrlTarget {
        Unchanged = 0,
        Empty = 1,
        SessionImage = 2,
        SessionContainerNavigation = 3,
        DerivedContainerNavigation = 4,
        Container = 5,
        Displayed = 6,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum ImageOpenEffect {
        ClearImage = 0,
        ResetZoom = 1,
        UpdatePageNavigation = 2,
        ScheduleAdjacentImagePredecode = 3,
        PrepareFailedContainer = 4,
    }

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
    struct ImageOpenBeginSourceLoadRequest {
        has_image: bool,
        loading_container_navigation_url_empty: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct ImageOpenSuccessfulImageLoadRequest {
        request_container_navigation_url_empty: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct ImageOpenSourceLoadErrorRequest {
        container_navigation_url_empty: bool,
        has_image: bool,
        displayed_url_empty: bool,
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
    struct ImageOpenTransition {
        source_url: ImageOpenUrlTarget,
        set_displayed_location_from_session: bool,
        container_navigation_url: ImageOpenUrlTarget,
        loading: ImageOpenBoolTarget,
        status: ImageOpenStatusTarget,
        error_string: ImageOpenErrorStringTarget,
        clear_loading_container_navigation_url: bool,
        effects: Vec<ImageOpenEffect>,
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

        #[cxx_name = "rustImageOpenBeginSourceLoad"]
        fn rust_image_open_begin_source_load(
            request: ImageOpenBeginSourceLoadRequest,
        ) -> ImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishEmptySourceLoad"]
        fn rust_image_open_finish_empty_source_load() -> ImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishSuccessfulImageLoad"]
        fn rust_image_open_finish_successful_image_load(
            request: ImageOpenSuccessfulImageLoadRequest,
        ) -> ImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishSourceLoadWithError"]
        fn rust_image_open_finish_source_load_with_error(
            request: ImageOpenSourceLoadErrorRequest,
        ) -> ImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishContainerNavigationLoadWithError"]
        fn rust_image_open_finish_container_navigation_load_with_error() -> ImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishAnimationLoadWithError"]
        fn rust_image_open_finish_animation_load_with_error() -> ImageOpenTransition;
    }
}

use ffi::{
    ImageOpenBeginSourceLoadRequest, ImageOpenBoolTarget, ImageOpenEffect,
    ImageOpenErrorStringTarget, ImageOpenSourceLoadErrorRequest, ImageOpenStatusTarget,
    ImageOpenSuccessfulImageLoadRequest, ImageOpenTransition, ImageOpenUrlTarget,
    RustImageSourceLoadAction, RustImageSourceLoadPlan, RustImageSourceLoadPolicyInput,
};

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

fn rust_image_open_begin_source_load(
    request: ImageOpenBeginSourceLoadRequest,
) -> ImageOpenTransition {
    let mut transition = empty_transition();

    if !request.has_image && request.loading_container_navigation_url_empty {
        transition.container_navigation_url = ImageOpenUrlTarget::Empty;
    }

    transition.loading = ImageOpenBoolTarget::True;
    if request.has_image {
        transition.status = ImageOpenStatusTarget::Ready;
    } else {
        transition.effects.push(ImageOpenEffect::ClearImage);
        transition.effects.push(ImageOpenEffect::ResetZoom);
        transition.status = ImageOpenStatusTarget::Loading;
    }

    transition
}

fn rust_image_open_finish_empty_source_load() -> ImageOpenTransition {
    let mut transition = tracked_load_finished_transition();
    transition.effects.push(ImageOpenEffect::ClearImage);
    transition.effects.push(ImageOpenEffect::ResetZoom);
    transition.container_navigation_url = ImageOpenUrlTarget::Empty;
    transition.status = ImageOpenStatusTarget::Null;
    transition
}

fn rust_image_open_finish_successful_image_load(
    request: ImageOpenSuccessfulImageLoadRequest,
) -> ImageOpenTransition {
    let mut transition = tracked_load_finished_transition();
    transition.source_url = ImageOpenUrlTarget::SessionImage;
    transition.set_displayed_location_from_session = true;
    transition.container_navigation_url = if request.request_container_navigation_url_empty {
        ImageOpenUrlTarget::DerivedContainerNavigation
    } else {
        ImageOpenUrlTarget::SessionContainerNavigation
    };
    transition.error_string = ImageOpenErrorStringTarget::Clear;
    transition.status = ImageOpenStatusTarget::Ready;
    transition
        .effects
        .push(ImageOpenEffect::UpdatePageNavigation);
    transition
        .effects
        .push(ImageOpenEffect::ScheduleAdjacentImagePredecode);
    transition
}

fn container_navigation_load_error_transition() -> ImageOpenTransition {
    let mut transition = tracked_load_error_transition();
    transition.source_url = ImageOpenUrlTarget::Container;
    transition.container_navigation_url = ImageOpenUrlTarget::Container;
    transition.effects.push(ImageOpenEffect::ClearImage);
    transition
        .effects
        .push(ImageOpenEffect::PrepareFailedContainer);
    transition
}

fn replacement_load_error_transition(displayed_url_empty: bool) -> ImageOpenTransition {
    let mut transition = tracked_load_error_transition();
    transition.status = ImageOpenStatusTarget::Ready;
    if !displayed_url_empty {
        transition.source_url = ImageOpenUrlTarget::Displayed;
    }
    transition
        .effects
        .push(ImageOpenEffect::UpdatePageNavigation);
    transition
        .effects
        .push(ImageOpenEffect::ScheduleAdjacentImagePredecode);
    transition
}

fn initial_load_error_transition() -> ImageOpenTransition {
    cleared_load_error_transition(false)
}

fn animation_load_error_transition() -> ImageOpenTransition {
    cleared_load_error_transition(true)
}

fn rust_image_open_finish_source_load_with_error(
    request: ImageOpenSourceLoadErrorRequest,
) -> ImageOpenTransition {
    if !request.container_navigation_url_empty {
        return container_navigation_load_error_transition();
    }
    if request.has_image {
        return replacement_load_error_transition(request.displayed_url_empty);
    }

    initial_load_error_transition()
}

fn rust_image_open_finish_container_navigation_load_with_error() -> ImageOpenTransition {
    container_navigation_load_error_transition()
}

fn rust_image_open_finish_animation_load_with_error() -> ImageOpenTransition {
    animation_load_error_transition()
}

fn cleared_load_error_transition(reset_zoom: bool) -> ImageOpenTransition {
    let mut transition = tracked_load_error_transition();
    transition.effects.push(ImageOpenEffect::ClearImage);
    if reset_zoom {
        transition.effects.push(ImageOpenEffect::ResetZoom);
    }
    transition.container_navigation_url = ImageOpenUrlTarget::Empty;
    transition
}

fn tracked_load_finished_transition() -> ImageOpenTransition {
    let mut transition = empty_transition();
    transition.clear_loading_container_navigation_url = true;
    transition.loading = ImageOpenBoolTarget::False;
    transition
}

fn tracked_load_error_transition() -> ImageOpenTransition {
    let mut transition = tracked_load_finished_transition();
    transition.error_string = ImageOpenErrorStringTarget::Provided;
    transition.status = ImageOpenStatusTarget::Error;
    transition
}

fn empty_transition() -> ImageOpenTransition {
    ImageOpenTransition {
        source_url: ImageOpenUrlTarget::Unchanged,
        set_displayed_location_from_session: false,
        container_navigation_url: ImageOpenUrlTarget::Unchanged,
        loading: ImageOpenBoolTarget::Unchanged,
        status: ImageOpenStatusTarget::Unchanged,
        error_string: ImageOpenErrorStringTarget::Unchanged,
        clear_loading_container_navigation_url: false,
        effects: Vec::new(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn has_effect(transition: &ImageOpenTransition, effect: ImageOpenEffect) -> bool {
        transition.effects.contains(&effect)
    }

    fn begin_source_load_request(
        has_image: bool,
        loading_container_navigation_url_empty: bool,
    ) -> ImageOpenBeginSourceLoadRequest {
        ImageOpenBeginSourceLoadRequest {
            has_image,
            loading_container_navigation_url_empty,
        }
    }

    fn successful_image_load_request(
        request_container_navigation_url_empty: bool,
    ) -> ImageOpenSuccessfulImageLoadRequest {
        ImageOpenSuccessfulImageLoadRequest {
            request_container_navigation_url_empty,
        }
    }

    fn source_load_error_request(
        container_navigation_url_empty: bool,
        has_image: bool,
        displayed_url_empty: bool,
    ) -> ImageOpenSourceLoadErrorRequest {
        ImageOpenSourceLoadErrorRequest {
            container_navigation_url_empty,
            has_image,
            displayed_url_empty,
        }
    }

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

    #[test]
    fn first_source_load_clears_image_and_enters_loading() {
        let transition = rust_image_open_begin_source_load(begin_source_load_request(false, true));

        assert_eq!(
            transition.container_navigation_url,
            ImageOpenUrlTarget::Empty
        );
        assert_eq!(transition.loading, ImageOpenBoolTarget::True);
        assert_eq!(transition.status, ImageOpenStatusTarget::Loading);
        assert_eq!(
            transition.effects,
            vec![ImageOpenEffect::ClearImage, ImageOpenEffect::ResetZoom]
        );
    }

    #[test]
    fn replacement_source_load_preserves_image_and_enters_ready() {
        let transition = rust_image_open_begin_source_load(begin_source_load_request(true, true));

        assert_eq!(
            transition.container_navigation_url,
            ImageOpenUrlTarget::Unchanged
        );
        assert_eq!(transition.loading, ImageOpenBoolTarget::True);
        assert_eq!(transition.status, ImageOpenStatusTarget::Ready);
        assert!(!has_effect(&transition, ImageOpenEffect::ClearImage));
        assert!(!has_effect(&transition, ImageOpenEffect::ResetZoom));
    }

    #[test]
    fn successful_load_uses_session_targets_and_clears_error() {
        let transition =
            rust_image_open_finish_successful_image_load(successful_image_load_request(false));

        assert_eq!(transition.source_url, ImageOpenUrlTarget::SessionImage);
        assert!(transition.set_displayed_location_from_session);
        assert_eq!(
            transition.container_navigation_url,
            ImageOpenUrlTarget::SessionContainerNavigation
        );
        assert_eq!(transition.error_string, ImageOpenErrorStringTarget::Clear);
        assert_eq!(transition.loading, ImageOpenBoolTarget::False);
        assert_eq!(transition.status, ImageOpenStatusTarget::Ready);
        assert!(transition.clear_loading_container_navigation_url);
        assert_eq!(
            transition.effects,
            vec![
                ImageOpenEffect::UpdatePageNavigation,
                ImageOpenEffect::ScheduleAdjacentImagePredecode
            ]
        );
    }

    #[test]
    fn replacement_failure_restores_displayed_source_and_schedules_predecode() {
        let transition = replacement_load_error_transition(false);

        assert_eq!(transition.source_url, ImageOpenUrlTarget::Displayed);
        assert_eq!(
            transition.error_string,
            ImageOpenErrorStringTarget::Provided
        );
        assert_eq!(transition.loading, ImageOpenBoolTarget::False);
        assert_eq!(transition.status, ImageOpenStatusTarget::Ready);
        assert!(has_effect(
            &transition,
            ImageOpenEffect::UpdatePageNavigation
        ));
        assert!(has_effect(
            &transition,
            ImageOpenEffect::ScheduleAdjacentImagePredecode
        ));
    }

    #[test]
    fn initial_and_animation_errors_share_clear_policy_but_only_animation_resets_zoom() {
        let initial = rust_image_open_finish_source_load_with_error(source_load_error_request(
            true, false, true,
        ));
        let animation = rust_image_open_finish_animation_load_with_error();

        assert!(has_effect(&initial, ImageOpenEffect::ClearImage));
        assert!(!has_effect(&initial, ImageOpenEffect::ResetZoom));
        assert_eq!(initial.container_navigation_url, ImageOpenUrlTarget::Empty);
        assert_eq!(initial.status, ImageOpenStatusTarget::Error);

        assert_eq!(
            animation.effects,
            vec![ImageOpenEffect::ClearImage, ImageOpenEffect::ResetZoom]
        );
        assert_eq!(
            animation.container_navigation_url,
            ImageOpenUrlTarget::Empty
        );
        assert_eq!(animation.status, ImageOpenStatusTarget::Error);
    }

    #[test]
    fn routed_load_failure_returns_the_selected_error_transition() {
        let container = rust_image_open_finish_source_load_with_error(source_load_error_request(
            false, true, false,
        ));
        assert_eq!(container.source_url, ImageOpenUrlTarget::Container);
        assert!(has_effect(&container, ImageOpenEffect::ClearImage));
        assert!(has_effect(
            &container,
            ImageOpenEffect::PrepareFailedContainer
        ));
        assert_eq!(container.status, ImageOpenStatusTarget::Error);

        let replacement = rust_image_open_finish_source_load_with_error(source_load_error_request(
            true, true, false,
        ));
        assert_eq!(replacement.source_url, ImageOpenUrlTarget::Displayed);
        assert!(has_effect(
            &replacement,
            ImageOpenEffect::UpdatePageNavigation
        ));
        assert!(has_effect(
            &replacement,
            ImageOpenEffect::ScheduleAdjacentImagePredecode
        ));
        assert_eq!(replacement.status, ImageOpenStatusTarget::Ready);

        let initial = rust_image_open_finish_source_load_with_error(source_load_error_request(
            true, false, true,
        ));
        assert_eq!(initial.source_url, ImageOpenUrlTarget::Unchanged);
        assert!(has_effect(&initial, ImageOpenEffect::ClearImage));
        assert_eq!(initial.container_navigation_url, ImageOpenUrlTarget::Empty);
        assert_eq!(initial.status, ImageOpenStatusTarget::Error);

        let explicit_container = rust_image_open_finish_container_navigation_load_with_error();
        assert_eq!(explicit_container.source_url, ImageOpenUrlTarget::Container);
        assert!(has_effect(
            &explicit_container,
            ImageOpenEffect::PrepareFailedContainer
        ));
    }
}
