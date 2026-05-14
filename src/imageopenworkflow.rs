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
    RustImageOpenLoadErrorKind, RustImageOpenLoadErrorRequest, RustImageOpenStatusTarget,
    RustImageOpenSuccessfulImageLoadRequest, RustImageOpenTransition, RustImageOpenUrlTarget,
};

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
