// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenBoolTarget {
        Unchanged = 0,
        False = 1,
        True = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenStatusTarget {
        Unchanged = 0,
        Null = 1,
        Loading = 2,
        Ready = 3,
        Error = 4,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenErrorStringTarget {
        Unchanged = 0,
        Clear = 1,
        Provided = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenUrlTarget {
        Unchanged = 0,
        Empty = 1,
        SessionImage = 2,
        SessionContainerNavigation = 3,
        DerivedContainerNavigation = 4,
        Container = 5,
        Displayed = 6,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenDisplayedLocationTarget {
        Unchanged = 0,
        SessionLocation = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenLoadErrorKind {
        SourceLoad = 0,
        ContainerNavigation = 1,
        Animation = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenEffect {
        ClearImage = 0,
        ResetZoom = 1,
        UpdatePageNavigation = 2,
        ScheduleAdjacentImagePredecode = 3,
        PrepareFailedContainer = 4,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenRightToLeftReadingNotification {
        None = 0,
        BeforeOpen = 1,
        AfterOpen = 2,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenSourceLoadRequest {
        source_url_changed: bool,
        preserve_two_page_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_enabled: bool,
        container_navigation_url_empty: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenBeginSourceLoadRequest {
        has_image: bool,
        loading_container_navigation_url_empty: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenSuccessfulImageLoadRequest {
        request_container_navigation_url_empty: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenLoadErrorRequest {
        kind: RustImageOpenLoadErrorKind,
        container_navigation_url_empty: bool,
        has_image: bool,
        displayed_url_empty: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenSourceLoadPlan {
        finish_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_notification: RustImageOpenRightToLeftReadingNotification,
        clear_loading_container_navigation_url: bool,
        update_container_navigation_url: bool,
        cancel_navigation_and_predecode: bool,
        clear_secondary_page: bool,
        set_loading_container_navigation_url: bool,
        set_source_url: bool,
        begin_open: bool,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustImageOpenTransition {
        source_url: RustImageOpenUrlTarget,
        displayed_location: RustImageOpenDisplayedLocationTarget,
        container_navigation_url: RustImageOpenUrlTarget,
        loading: RustImageOpenBoolTarget,
        status: RustImageOpenStatusTarget,
        error_string: RustImageOpenErrorStringTarget,
        clear_loading_container_navigation_url: bool,
        effects: Vec<RustImageOpenEffect>,
    }

    extern "Rust" {
        #[cxx_name = "rustImageOpenSourceLoadPlan"]
        fn rust_image_open_source_load_plan(
            request: RustImageOpenSourceLoadRequest,
        ) -> RustImageOpenSourceLoadPlan;

        #[cxx_name = "rustImageOpenBeginSourceLoad"]
        fn rust_image_open_begin_source_load(
            request: RustImageOpenBeginSourceLoadRequest,
        ) -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishEmptySourceLoad"]
        fn rust_image_open_finish_empty_source_load() -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishSuccessfulImageLoad"]
        fn rust_image_open_finish_successful_image_load(
            request: RustImageOpenSuccessfulImageLoadRequest,
        ) -> RustImageOpenTransition;

        #[cxx_name = "rustImageOpenFinishLoadWithError"]
        fn rust_image_open_finish_load_with_error(
            request: RustImageOpenLoadErrorRequest,
        ) -> RustImageOpenTransition;
    }
}

use ffi::{
    RustImageOpenBeginSourceLoadRequest, RustImageOpenBoolTarget,
    RustImageOpenDisplayedLocationTarget, RustImageOpenEffect, RustImageOpenErrorStringTarget,
    RustImageOpenLoadErrorKind, RustImageOpenLoadErrorRequest,
    RustImageOpenRightToLeftReadingNotification, RustImageOpenSourceLoadPlan,
    RustImageOpenSourceLoadRequest, RustImageOpenStatusTarget,
    RustImageOpenSuccessfulImageLoadRequest, RustImageOpenTransition, RustImageOpenUrlTarget,
};

fn rust_image_open_source_load_plan(
    request: RustImageOpenSourceLoadRequest,
) -> RustImageOpenSourceLoadPlan {
    let mut plan = empty_source_load_plan();
    plan.finish_spread_transition = !request.preserve_two_page_spread_transition;
    plan.reset_right_to_left_reading = request.reset_right_to_left_reading;
    let notify_right_to_left_reading =
        request.reset_right_to_left_reading && request.right_to_left_reading_enabled;
    plan.right_to_left_reading_notification = if !notify_right_to_left_reading {
        RustImageOpenRightToLeftReadingNotification::None
    } else if request.source_url_changed {
        RustImageOpenRightToLeftReadingNotification::AfterOpen
    } else {
        RustImageOpenRightToLeftReadingNotification::BeforeOpen
    };

    if !request.source_url_changed {
        plan.clear_loading_container_navigation_url = true;
        plan.update_container_navigation_url = !request.container_navigation_url_empty;
        return plan;
    }

    plan.cancel_navigation_and_predecode = true;
    plan.clear_secondary_page = true;
    plan.set_loading_container_navigation_url = true;
    plan.set_source_url = true;
    plan.begin_open = true;
    plan
}

fn rust_image_open_begin_source_load(
    request: RustImageOpenBeginSourceLoadRequest,
) -> RustImageOpenTransition {
    let mut transition = empty_transition();

    if !request.has_image && request.loading_container_navigation_url_empty {
        transition.container_navigation_url = RustImageOpenUrlTarget::Empty;
    }

    transition.loading = RustImageOpenBoolTarget::True;
    if request.has_image {
        transition.status = RustImageOpenStatusTarget::Ready;
    } else {
        transition.effects.push(RustImageOpenEffect::ClearImage);
        transition.effects.push(RustImageOpenEffect::ResetZoom);
        transition.status = RustImageOpenStatusTarget::Loading;
    }

    transition
}

fn rust_image_open_finish_empty_source_load() -> RustImageOpenTransition {
    let mut transition = tracked_load_finished_transition();
    transition.effects.push(RustImageOpenEffect::ClearImage);
    transition.effects.push(RustImageOpenEffect::ResetZoom);
    transition.container_navigation_url = RustImageOpenUrlTarget::Empty;
    transition.status = RustImageOpenStatusTarget::Null;
    transition
}

fn rust_image_open_finish_successful_image_load(
    request: RustImageOpenSuccessfulImageLoadRequest,
) -> RustImageOpenTransition {
    let mut transition = tracked_load_finished_transition();
    transition.source_url = RustImageOpenUrlTarget::SessionImage;
    transition.displayed_location = RustImageOpenDisplayedLocationTarget::SessionLocation;
    transition.container_navigation_url = if request.request_container_navigation_url_empty {
        RustImageOpenUrlTarget::DerivedContainerNavigation
    } else {
        RustImageOpenUrlTarget::SessionContainerNavigation
    };
    transition.error_string = RustImageOpenErrorStringTarget::Clear;
    transition.status = RustImageOpenStatusTarget::Ready;
    transition
        .effects
        .push(RustImageOpenEffect::UpdatePageNavigation);
    transition
        .effects
        .push(RustImageOpenEffect::ScheduleAdjacentImagePredecode);
    transition
}

fn container_navigation_load_error_transition() -> RustImageOpenTransition {
    let mut transition = tracked_load_error_transition();
    transition.source_url = RustImageOpenUrlTarget::Container;
    transition.container_navigation_url = RustImageOpenUrlTarget::Container;
    transition.effects.push(RustImageOpenEffect::ClearImage);
    transition
        .effects
        .push(RustImageOpenEffect::PrepareFailedContainer);
    transition
}

fn replacement_load_error_transition(displayed_url_empty: bool) -> RustImageOpenTransition {
    let mut transition = tracked_load_error_transition();
    transition.status = RustImageOpenStatusTarget::Ready;
    if !displayed_url_empty {
        transition.source_url = RustImageOpenUrlTarget::Displayed;
    }
    transition
        .effects
        .push(RustImageOpenEffect::UpdatePageNavigation);
    transition
        .effects
        .push(RustImageOpenEffect::ScheduleAdjacentImagePredecode);
    transition
}

fn initial_load_error_transition() -> RustImageOpenTransition {
    cleared_load_error_transition(false)
}

fn animation_load_error_transition() -> RustImageOpenTransition {
    cleared_load_error_transition(true)
}

fn rust_image_open_finish_load_with_error(
    request: RustImageOpenLoadErrorRequest,
) -> RustImageOpenTransition {
    match request.kind {
        RustImageOpenLoadErrorKind::ContainerNavigation => {
            return container_navigation_load_error_transition();
        }
        RustImageOpenLoadErrorKind::Animation => {
            return animation_load_error_transition();
        }
        RustImageOpenLoadErrorKind::SourceLoad => {}
        _ => {}
    }

    if !request.container_navigation_url_empty {
        return container_navigation_load_error_transition();
    }
    if request.has_image {
        return replacement_load_error_transition(request.displayed_url_empty);
    }

    initial_load_error_transition()
}

fn cleared_load_error_transition(reset_zoom: bool) -> RustImageOpenTransition {
    let mut transition = tracked_load_error_transition();
    transition.effects.push(RustImageOpenEffect::ClearImage);
    if reset_zoom {
        transition.effects.push(RustImageOpenEffect::ResetZoom);
    }
    transition.container_navigation_url = RustImageOpenUrlTarget::Empty;
    transition
}

fn tracked_load_finished_transition() -> RustImageOpenTransition {
    let mut transition = empty_transition();
    transition.clear_loading_container_navigation_url = true;
    transition.loading = RustImageOpenBoolTarget::False;
    transition
}

fn tracked_load_error_transition() -> RustImageOpenTransition {
    let mut transition = tracked_load_finished_transition();
    transition.error_string = RustImageOpenErrorStringTarget::Provided;
    transition.status = RustImageOpenStatusTarget::Error;
    transition
}

fn empty_source_load_plan() -> RustImageOpenSourceLoadPlan {
    RustImageOpenSourceLoadPlan {
        finish_spread_transition: false,
        reset_right_to_left_reading: false,
        right_to_left_reading_notification: RustImageOpenRightToLeftReadingNotification::None,
        clear_loading_container_navigation_url: false,
        update_container_navigation_url: false,
        cancel_navigation_and_predecode: false,
        clear_secondary_page: false,
        set_loading_container_navigation_url: false,
        set_source_url: false,
        begin_open: false,
    }
}

fn empty_transition() -> RustImageOpenTransition {
    RustImageOpenTransition {
        source_url: RustImageOpenUrlTarget::Unchanged,
        displayed_location: RustImageOpenDisplayedLocationTarget::Unchanged,
        container_navigation_url: RustImageOpenUrlTarget::Unchanged,
        loading: RustImageOpenBoolTarget::Unchanged,
        status: RustImageOpenStatusTarget::Unchanged,
        error_string: RustImageOpenErrorStringTarget::Unchanged,
        clear_loading_container_navigation_url: false,
        effects: Vec::new(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn has_effect(transition: &RustImageOpenTransition, effect: RustImageOpenEffect) -> bool {
        transition.effects.contains(&effect)
    }

    fn source_load_request(
        source_url_changed: bool,
        preserve_two_page_spread_transition: bool,
        reset_right_to_left_reading: bool,
        right_to_left_reading_enabled: bool,
        container_navigation_url_empty: bool,
    ) -> RustImageOpenSourceLoadRequest {
        RustImageOpenSourceLoadRequest {
            source_url_changed,
            preserve_two_page_spread_transition,
            reset_right_to_left_reading,
            right_to_left_reading_enabled,
            container_navigation_url_empty,
        }
    }

    fn begin_source_load_request(
        has_image: bool,
        loading_container_navigation_url_empty: bool,
    ) -> RustImageOpenBeginSourceLoadRequest {
        RustImageOpenBeginSourceLoadRequest {
            has_image,
            loading_container_navigation_url_empty,
        }
    }

    fn successful_image_load_request(
        request_container_navigation_url_empty: bool,
    ) -> RustImageOpenSuccessfulImageLoadRequest {
        RustImageOpenSuccessfulImageLoadRequest {
            request_container_navigation_url_empty,
        }
    }

    fn load_error_request(
        container_navigation_url_empty: bool,
        has_image: bool,
        displayed_url_empty: bool,
    ) -> RustImageOpenLoadErrorRequest {
        RustImageOpenLoadErrorRequest {
            kind: RustImageOpenLoadErrorKind::SourceLoad,
            container_navigation_url_empty,
            has_image,
            displayed_url_empty,
        }
    }

    fn load_error_kind_request(kind: RustImageOpenLoadErrorKind) -> RustImageOpenLoadErrorRequest {
        RustImageOpenLoadErrorRequest {
            kind,
            container_navigation_url_empty: true,
            has_image: false,
            displayed_url_empty: true,
        }
    }

    #[test]
    fn first_source_load_clears_image_and_enters_loading() {
        let transition = rust_image_open_begin_source_load(begin_source_load_request(false, true));

        assert_eq!(
            transition.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(transition.loading, RustImageOpenBoolTarget::True);
        assert_eq!(transition.status, RustImageOpenStatusTarget::Loading);
        assert_eq!(
            transition.effects,
            vec![
                RustImageOpenEffect::ClearImage,
                RustImageOpenEffect::ResetZoom
            ]
        );
    }

    #[test]
    fn unchanged_source_load_clears_loading_and_optionally_updates_container() {
        let plan =
            rust_image_open_source_load_plan(source_load_request(false, false, true, true, false));

        assert!(plan.finish_spread_transition);
        assert!(plan.reset_right_to_left_reading);
        assert_eq!(
            plan.right_to_left_reading_notification,
            RustImageOpenRightToLeftReadingNotification::BeforeOpen
        );
        assert!(plan.clear_loading_container_navigation_url);
        assert!(plan.update_container_navigation_url);
        assert!(!plan.cancel_navigation_and_predecode);
        assert!(!plan.clear_secondary_page);
        assert!(!plan.set_loading_container_navigation_url);
        assert!(!plan.set_source_url);
        assert!(!plan.begin_open);
    }

    #[test]
    fn changed_source_load_starts_new_load_without_container_navigation_update() {
        let plan =
            rust_image_open_source_load_plan(source_load_request(true, false, false, true, false));

        assert!(plan.finish_spread_transition);
        assert!(!plan.reset_right_to_left_reading);
        assert_eq!(
            plan.right_to_left_reading_notification,
            RustImageOpenRightToLeftReadingNotification::None
        );
        assert!(!plan.clear_loading_container_navigation_url);
        assert!(!plan.update_container_navigation_url);
        assert!(plan.cancel_navigation_and_predecode);
        assert!(plan.clear_secondary_page);
        assert!(plan.set_loading_container_navigation_url);
        assert!(plan.set_source_url);
        assert!(plan.begin_open);
    }

    #[test]
    fn source_load_plan_can_preserve_active_spread_transition() {
        let plan =
            rust_image_open_source_load_plan(source_load_request(true, true, true, true, true));

        assert!(!plan.finish_spread_transition);
        assert!(plan.reset_right_to_left_reading);
        assert_eq!(
            plan.right_to_left_reading_notification,
            RustImageOpenRightToLeftReadingNotification::AfterOpen
        );
        assert!(plan.begin_open);
    }

    #[test]
    fn source_load_plan_notifies_only_when_right_to_left_reading_was_enabled() {
        let plan =
            rust_image_open_source_load_plan(source_load_request(false, false, true, false, false));

        assert!(plan.reset_right_to_left_reading);
        assert_eq!(
            plan.right_to_left_reading_notification,
            RustImageOpenRightToLeftReadingNotification::None
        );
    }

    #[test]
    fn replacement_source_load_preserves_image_and_enters_ready() {
        let transition = rust_image_open_begin_source_load(begin_source_load_request(true, true));

        assert_eq!(
            transition.container_navigation_url,
            RustImageOpenUrlTarget::Unchanged
        );
        assert_eq!(transition.loading, RustImageOpenBoolTarget::True);
        assert_eq!(transition.status, RustImageOpenStatusTarget::Ready);
        assert!(!has_effect(&transition, RustImageOpenEffect::ClearImage));
        assert!(!has_effect(&transition, RustImageOpenEffect::ResetZoom));
    }

    #[test]
    fn successful_load_uses_session_targets_and_clears_error() {
        let transition =
            rust_image_open_finish_successful_image_load(successful_image_load_request(false));

        assert_eq!(transition.source_url, RustImageOpenUrlTarget::SessionImage);
        assert_eq!(
            transition.displayed_location,
            RustImageOpenDisplayedLocationTarget::SessionLocation
        );
        assert_eq!(
            transition.container_navigation_url,
            RustImageOpenUrlTarget::SessionContainerNavigation
        );
        assert_eq!(
            transition.error_string,
            RustImageOpenErrorStringTarget::Clear
        );
        assert_eq!(transition.loading, RustImageOpenBoolTarget::False);
        assert_eq!(transition.status, RustImageOpenStatusTarget::Ready);
        assert!(transition.clear_loading_container_navigation_url);
        assert_eq!(
            transition.effects,
            vec![
                RustImageOpenEffect::UpdatePageNavigation,
                RustImageOpenEffect::ScheduleAdjacentImagePredecode
            ]
        );
    }

    #[test]
    fn replacement_failure_restores_displayed_source_and_schedules_predecode() {
        let transition = replacement_load_error_transition(false);

        assert_eq!(transition.source_url, RustImageOpenUrlTarget::Displayed);
        assert_eq!(
            transition.error_string,
            RustImageOpenErrorStringTarget::Provided
        );
        assert_eq!(transition.loading, RustImageOpenBoolTarget::False);
        assert_eq!(transition.status, RustImageOpenStatusTarget::Ready);
        assert!(has_effect(
            &transition,
            RustImageOpenEffect::UpdatePageNavigation
        ));
        assert!(has_effect(
            &transition,
            RustImageOpenEffect::ScheduleAdjacentImagePredecode
        ));
    }

    #[test]
    fn initial_and_animation_errors_share_clear_policy_but_only_animation_resets_zoom() {
        let initial = rust_image_open_finish_load_with_error(load_error_request(true, false, true));
        let animation = rust_image_open_finish_load_with_error(load_error_kind_request(
            RustImageOpenLoadErrorKind::Animation,
        ));

        assert!(has_effect(&initial, RustImageOpenEffect::ClearImage));
        assert!(!has_effect(&initial, RustImageOpenEffect::ResetZoom));
        assert_eq!(
            initial.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(initial.status, RustImageOpenStatusTarget::Error);

        assert_eq!(
            animation.effects,
            vec![
                RustImageOpenEffect::ClearImage,
                RustImageOpenEffect::ResetZoom
            ]
        );
        assert_eq!(
            animation.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(animation.status, RustImageOpenStatusTarget::Error);
    }

    #[test]
    fn routed_load_failure_returns_the_selected_error_transition() {
        let container =
            rust_image_open_finish_load_with_error(load_error_request(false, true, false));
        assert_eq!(container.source_url, RustImageOpenUrlTarget::Container);
        assert!(has_effect(&container, RustImageOpenEffect::ClearImage));
        assert!(has_effect(
            &container,
            RustImageOpenEffect::PrepareFailedContainer
        ));
        assert_eq!(container.status, RustImageOpenStatusTarget::Error);

        let replacement =
            rust_image_open_finish_load_with_error(load_error_request(true, true, false));
        assert_eq!(replacement.source_url, RustImageOpenUrlTarget::Displayed);
        assert!(has_effect(
            &replacement,
            RustImageOpenEffect::UpdatePageNavigation
        ));
        assert!(has_effect(
            &replacement,
            RustImageOpenEffect::ScheduleAdjacentImagePredecode
        ));
        assert_eq!(replacement.status, RustImageOpenStatusTarget::Ready);

        let initial = rust_image_open_finish_load_with_error(load_error_request(true, false, true));
        assert_eq!(initial.source_url, RustImageOpenUrlTarget::Unchanged);
        assert!(has_effect(&initial, RustImageOpenEffect::ClearImage));
        assert_eq!(
            initial.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(initial.status, RustImageOpenStatusTarget::Error);

        let explicit_container = rust_image_open_finish_load_with_error(load_error_kind_request(
            RustImageOpenLoadErrorKind::ContainerNavigation,
        ));
        assert_eq!(
            explicit_container.source_url,
            RustImageOpenUrlTarget::Container
        );
        assert!(has_effect(
            &explicit_container,
            RustImageOpenEffect::PrepareFailedContainer
        ));
    }
}
