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
    enum ImageOpenDisplayedLocationTarget {
        Unchanged = 0,
        Session = 1,
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
    enum ImageOpenWorkflowEventKind {
        BeginSourceLoad = 0,
        FinishEmptySourceLoad = 1,
        FinishSuccessfulImageLoad = 2,
        FinishSourceLoadWithError = 3,
        FinishContainerNavigationLoadWithError = 4,
        FinishAnimationLoadWithError = 5,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct ImageOpenWorkflowEvent {
        kind: ImageOpenWorkflowEventKind,
        has_image: bool,
        loading_container_navigation_url_empty: bool,
        request_container_navigation_url_empty: bool,
        container_navigation_url_empty: bool,
        displayed_url_empty: bool,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct ImageOpenTransition {
        state_delta: ImageOpenStateDelta,
        effects: Vec<ImageOpenEffect>,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct ImageOpenStateDelta {
        source_url: ImageOpenUrlTarget,
        displayed_location: ImageOpenDisplayedLocationTarget,
        container_navigation_url: ImageOpenUrlTarget,
        loading: ImageOpenBoolTarget,
        status: ImageOpenStatusTarget,
        error_string: ImageOpenErrorStringTarget,
        clear_loading_container_navigation_url: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustImageOpenTransition"]
        fn rust_image_open_transition(event: ImageOpenWorkflowEvent) -> ImageOpenTransition;
    }
}

use ffi::{
    ImageOpenBoolTarget, ImageOpenDisplayedLocationTarget, ImageOpenEffect,
    ImageOpenErrorStringTarget, ImageOpenStateDelta, ImageOpenStatusTarget, ImageOpenTransition,
    ImageOpenUrlTarget, ImageOpenWorkflowEvent, ImageOpenWorkflowEventKind,
};

fn rust_image_open_transition(event: ImageOpenWorkflowEvent) -> ImageOpenTransition {
    match event.kind {
        ImageOpenWorkflowEventKind::BeginSourceLoad => begin_source_load_transition(event),
        ImageOpenWorkflowEventKind::FinishEmptySourceLoad => finish_empty_source_load_transition(),
        ImageOpenWorkflowEventKind::FinishSuccessfulImageLoad => {
            finish_successful_image_load_transition(event)
        }
        ImageOpenWorkflowEventKind::FinishSourceLoadWithError => {
            source_load_error_transition(event)
        }
        ImageOpenWorkflowEventKind::FinishContainerNavigationLoadWithError => {
            container_navigation_load_error_transition()
        }
        ImageOpenWorkflowEventKind::FinishAnimationLoadWithError => {
            animation_load_error_transition()
        }
        _ => empty_transition(),
    }
}

fn begin_source_load_transition(event: ImageOpenWorkflowEvent) -> ImageOpenTransition {
    let mut transition = empty_transition();

    if !event.has_image && event.loading_container_navigation_url_empty {
        set_container_navigation_url(&mut transition, ImageOpenUrlTarget::Empty);
    }

    set_loading(&mut transition, ImageOpenBoolTarget::True);
    if event.has_image {
        set_status(&mut transition, ImageOpenStatusTarget::Ready);
    } else {
        push_effect(&mut transition, ImageOpenEffect::ClearImage);
        push_effect(&mut transition, ImageOpenEffect::ResetZoom);
        set_status(&mut transition, ImageOpenStatusTarget::Loading);
    }
    transition
}

fn finish_empty_source_load_transition() -> ImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, ImageOpenEffect::ClearImage);
    push_effect(&mut transition, ImageOpenEffect::ResetZoom);
    set_tracked_load_completed(&mut transition);
    set_container_navigation_url(&mut transition, ImageOpenUrlTarget::Empty);
    set_status(&mut transition, ImageOpenStatusTarget::Null);
    transition
}

fn finish_successful_image_load_transition(event: ImageOpenWorkflowEvent) -> ImageOpenTransition {
    let mut transition = empty_transition();
    set_source_url(&mut transition, ImageOpenUrlTarget::SessionImage);
    set_displayed_location(&mut transition, ImageOpenDisplayedLocationTarget::Session);
    set_container_navigation_url(
        &mut transition,
        if event.request_container_navigation_url_empty {
            ImageOpenUrlTarget::DerivedContainerNavigation
        } else {
            ImageOpenUrlTarget::SessionContainerNavigation
        },
    );
    set_error_string(&mut transition, ImageOpenErrorStringTarget::Clear);
    set_tracked_load_completed(&mut transition);
    set_status(&mut transition, ImageOpenStatusTarget::Ready);
    push_effect(&mut transition, ImageOpenEffect::UpdatePageNavigation);
    push_effect(
        &mut transition,
        ImageOpenEffect::ScheduleAdjacentImagePredecode,
    );
    transition
}

fn container_navigation_load_error_transition() -> ImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, ImageOpenEffect::ClearImage);
    push_effect(&mut transition, ImageOpenEffect::PrepareFailedContainer);
    set_tracked_load_completed(&mut transition);
    set_container_navigation_url(&mut transition, ImageOpenUrlTarget::Container);
    set_source_url(&mut transition, ImageOpenUrlTarget::Container);
    set_error_string(&mut transition, ImageOpenErrorStringTarget::Provided);
    set_status(&mut transition, ImageOpenStatusTarget::Error);
    transition
}

fn replacement_load_error_transition(displayed_url_empty: bool) -> ImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, ImageOpenEffect::UpdatePageNavigation);
    push_effect(
        &mut transition,
        ImageOpenEffect::ScheduleAdjacentImagePredecode,
    );
    set_tracked_load_completed(&mut transition);
    if !displayed_url_empty {
        set_source_url(&mut transition, ImageOpenUrlTarget::Displayed);
    }
    set_error_string(&mut transition, ImageOpenErrorStringTarget::Provided);
    set_status(&mut transition, ImageOpenStatusTarget::Ready);
    transition
}

fn initial_load_error_transition() -> ImageOpenTransition {
    cleared_load_error_transition(false)
}

fn animation_load_error_transition() -> ImageOpenTransition {
    cleared_load_error_transition(true)
}

fn source_load_error_transition(event: ImageOpenWorkflowEvent) -> ImageOpenTransition {
    if !event.container_navigation_url_empty {
        return container_navigation_load_error_transition();
    }
    if event.has_image {
        return replacement_load_error_transition(event.displayed_url_empty);
    }

    initial_load_error_transition()
}

fn cleared_load_error_transition(reset_zoom: bool) -> ImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, ImageOpenEffect::ClearImage);
    if reset_zoom {
        push_effect(&mut transition, ImageOpenEffect::ResetZoom);
    }
    set_tracked_load_completed(&mut transition);
    set_container_navigation_url(&mut transition, ImageOpenUrlTarget::Empty);
    set_error_string(&mut transition, ImageOpenErrorStringTarget::Provided);
    set_status(&mut transition, ImageOpenStatusTarget::Error);
    transition
}

fn set_tracked_load_completed(transition: &mut ImageOpenTransition) {
    transition
        .state_delta
        .clear_loading_container_navigation_url = true;
    set_loading(transition, ImageOpenBoolTarget::False);
}

fn set_source_url(transition: &mut ImageOpenTransition, target: ImageOpenUrlTarget) {
    debug_assert_eq!(
        transition.state_delta.source_url,
        ImageOpenUrlTarget::Unchanged
    );
    transition.state_delta.source_url = target;
}

fn set_displayed_location(
    transition: &mut ImageOpenTransition,
    target: ImageOpenDisplayedLocationTarget,
) {
    debug_assert_eq!(
        transition.state_delta.displayed_location,
        ImageOpenDisplayedLocationTarget::Unchanged
    );
    transition.state_delta.displayed_location = target;
}

fn set_container_navigation_url(transition: &mut ImageOpenTransition, target: ImageOpenUrlTarget) {
    debug_assert_eq!(
        transition.state_delta.container_navigation_url,
        ImageOpenUrlTarget::Unchanged
    );
    transition.state_delta.container_navigation_url = target;
}

fn set_loading(transition: &mut ImageOpenTransition, target: ImageOpenBoolTarget) {
    debug_assert_eq!(
        transition.state_delta.loading,
        ImageOpenBoolTarget::Unchanged
    );
    transition.state_delta.loading = target;
}

fn set_status(transition: &mut ImageOpenTransition, target: ImageOpenStatusTarget) {
    debug_assert_eq!(
        transition.state_delta.status,
        ImageOpenStatusTarget::Unchanged
    );
    transition.state_delta.status = target;
}

fn set_error_string(transition: &mut ImageOpenTransition, target: ImageOpenErrorStringTarget) {
    debug_assert_eq!(
        transition.state_delta.error_string,
        ImageOpenErrorStringTarget::Unchanged
    );
    transition.state_delta.error_string = target;
}

fn push_effect(transition: &mut ImageOpenTransition, effect: ImageOpenEffect) {
    transition.effects.push(effect);
}

fn empty_transition() -> ImageOpenTransition {
    ImageOpenTransition {
        state_delta: empty_state_delta(),
        effects: Vec::new(),
    }
}

fn empty_state_delta() -> ImageOpenStateDelta {
    ImageOpenStateDelta {
        source_url: ImageOpenUrlTarget::Unchanged,
        displayed_location: ImageOpenDisplayedLocationTarget::Unchanged,
        container_navigation_url: ImageOpenUrlTarget::Unchanged,
        loading: ImageOpenBoolTarget::Unchanged,
        status: ImageOpenStatusTarget::Unchanged,
        error_string: ImageOpenErrorStringTarget::Unchanged,
        clear_loading_container_navigation_url: false,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn has_effect(transition: &ImageOpenTransition, effect: ImageOpenEffect) -> bool {
        transition.effects.contains(&effect)
    }

    fn state_delta(
        source_url: ImageOpenUrlTarget,
        displayed_location: ImageOpenDisplayedLocationTarget,
        container_navigation_url: ImageOpenUrlTarget,
        loading: ImageOpenBoolTarget,
        status: ImageOpenStatusTarget,
        error_string: ImageOpenErrorStringTarget,
        clear_loading_container_navigation_url: bool,
    ) -> ImageOpenStateDelta {
        ImageOpenStateDelta {
            source_url,
            displayed_location,
            container_navigation_url,
            loading,
            status,
            error_string,
            clear_loading_container_navigation_url,
        }
    }

    fn completed_load_delta(
        source_url: ImageOpenUrlTarget,
        displayed_location: ImageOpenDisplayedLocationTarget,
        container_navigation_url: ImageOpenUrlTarget,
        status: ImageOpenStatusTarget,
        error_string: ImageOpenErrorStringTarget,
    ) -> ImageOpenStateDelta {
        state_delta(
            source_url,
            displayed_location,
            container_navigation_url,
            ImageOpenBoolTarget::False,
            status,
            error_string,
            true,
        )
    }

    fn image_open_event(kind: ImageOpenWorkflowEventKind) -> ImageOpenWorkflowEvent {
        ImageOpenWorkflowEvent {
            kind,
            has_image: false,
            loading_container_navigation_url_empty: false,
            request_container_navigation_url_empty: false,
            container_navigation_url_empty: false,
            displayed_url_empty: false,
        }
    }

    fn begin_source_load_event(
        has_image: bool,
        loading_container_navigation_url_empty: bool,
    ) -> ImageOpenWorkflowEvent {
        ImageOpenWorkflowEvent {
            kind: ImageOpenWorkflowEventKind::BeginSourceLoad,
            has_image,
            loading_container_navigation_url_empty,
            ..image_open_event(ImageOpenWorkflowEventKind::BeginSourceLoad)
        }
    }

    fn successful_image_load_event(
        request_container_navigation_url_empty: bool,
    ) -> ImageOpenWorkflowEvent {
        ImageOpenWorkflowEvent {
            kind: ImageOpenWorkflowEventKind::FinishSuccessfulImageLoad,
            request_container_navigation_url_empty,
            ..image_open_event(ImageOpenWorkflowEventKind::FinishSuccessfulImageLoad)
        }
    }

    fn source_load_error_event(
        container_navigation_url_empty: bool,
        has_image: bool,
        displayed_url_empty: bool,
    ) -> ImageOpenWorkflowEvent {
        ImageOpenWorkflowEvent {
            kind: ImageOpenWorkflowEventKind::FinishSourceLoadWithError,
            container_navigation_url_empty,
            has_image,
            displayed_url_empty,
            ..image_open_event(ImageOpenWorkflowEventKind::FinishSourceLoadWithError)
        }
    }

    #[test]
    fn first_source_load_clears_image_and_enters_loading() {
        let transition = rust_image_open_transition(begin_source_load_event(false, true));

        assert_eq!(
            transition.state_delta,
            state_delta(
                ImageOpenUrlTarget::Unchanged,
                ImageOpenDisplayedLocationTarget::Unchanged,
                ImageOpenUrlTarget::Empty,
                ImageOpenBoolTarget::True,
                ImageOpenStatusTarget::Loading,
                ImageOpenErrorStringTarget::Unchanged,
                false,
            )
        );
        assert_eq!(
            transition.effects,
            vec![ImageOpenEffect::ClearImage, ImageOpenEffect::ResetZoom]
        );
    }

    #[test]
    fn replacement_source_load_preserves_image_and_enters_ready() {
        let transition = rust_image_open_transition(begin_source_load_event(true, true));

        assert_eq!(
            transition.state_delta,
            state_delta(
                ImageOpenUrlTarget::Unchanged,
                ImageOpenDisplayedLocationTarget::Unchanged,
                ImageOpenUrlTarget::Unchanged,
                ImageOpenBoolTarget::True,
                ImageOpenStatusTarget::Ready,
                ImageOpenErrorStringTarget::Unchanged,
                false,
            )
        );
        assert!(!has_effect(&transition, ImageOpenEffect::ClearImage));
        assert!(!has_effect(&transition, ImageOpenEffect::ResetZoom));
    }

    #[test]
    fn successful_load_uses_session_targets_and_clears_error() {
        let transition = rust_image_open_transition(successful_image_load_event(false));

        assert_eq!(
            transition.state_delta,
            completed_load_delta(
                ImageOpenUrlTarget::SessionImage,
                ImageOpenDisplayedLocationTarget::Session,
                ImageOpenUrlTarget::SessionContainerNavigation,
                ImageOpenStatusTarget::Ready,
                ImageOpenErrorStringTarget::Clear,
            )
        );
        assert_eq!(
            transition.effects,
            vec![
                ImageOpenEffect::UpdatePageNavigation,
                ImageOpenEffect::ScheduleAdjacentImagePredecode,
            ]
        );
    }

    #[test]
    fn replacement_failure_restores_displayed_source_and_schedules_predecode() {
        let transition = replacement_load_error_transition(false);

        assert_eq!(
            transition.state_delta,
            completed_load_delta(
                ImageOpenUrlTarget::Displayed,
                ImageOpenDisplayedLocationTarget::Unchanged,
                ImageOpenUrlTarget::Unchanged,
                ImageOpenStatusTarget::Ready,
                ImageOpenErrorStringTarget::Provided,
            )
        );
        assert_eq!(
            transition.effects,
            vec![
                ImageOpenEffect::UpdatePageNavigation,
                ImageOpenEffect::ScheduleAdjacentImagePredecode,
            ]
        );
    }

    #[test]
    fn initial_and_animation_errors_share_clear_policy_but_only_animation_resets_zoom() {
        let initial = rust_image_open_transition(source_load_error_event(true, false, true));
        let animation = rust_image_open_transition(image_open_event(
            ImageOpenWorkflowEventKind::FinishAnimationLoadWithError,
        ));

        assert!(has_effect(&initial, ImageOpenEffect::ClearImage));
        assert!(!has_effect(&initial, ImageOpenEffect::ResetZoom));
        assert_eq!(
            initial.state_delta.container_navigation_url,
            ImageOpenUrlTarget::Empty
        );
        assert_eq!(initial.state_delta.status, ImageOpenStatusTarget::Error);

        assert_eq!(
            animation.state_delta,
            completed_load_delta(
                ImageOpenUrlTarget::Unchanged,
                ImageOpenDisplayedLocationTarget::Unchanged,
                ImageOpenUrlTarget::Empty,
                ImageOpenStatusTarget::Error,
                ImageOpenErrorStringTarget::Provided,
            )
        );
        assert_eq!(
            animation.effects,
            vec![ImageOpenEffect::ClearImage, ImageOpenEffect::ResetZoom]
        );
    }

    #[test]
    fn routed_load_failure_returns_the_selected_error_transition() {
        let container = rust_image_open_transition(source_load_error_event(false, true, false));
        assert_eq!(
            container.state_delta.source_url,
            ImageOpenUrlTarget::Container
        );
        assert!(has_effect(&container, ImageOpenEffect::ClearImage));
        assert!(has_effect(
            &container,
            ImageOpenEffect::PrepareFailedContainer
        ));
        assert_eq!(container.state_delta.status, ImageOpenStatusTarget::Error);

        let replacement = rust_image_open_transition(source_load_error_event(true, true, false));
        assert_eq!(
            replacement.state_delta.source_url,
            ImageOpenUrlTarget::Displayed
        );
        assert!(has_effect(
            &replacement,
            ImageOpenEffect::UpdatePageNavigation
        ));
        assert!(has_effect(
            &replacement,
            ImageOpenEffect::ScheduleAdjacentImagePredecode
        ));
        assert_eq!(replacement.state_delta.status, ImageOpenStatusTarget::Ready);

        let initial = rust_image_open_transition(source_load_error_event(true, false, true));
        assert_eq!(
            initial.state_delta.source_url,
            ImageOpenUrlTarget::Unchanged
        );
        assert!(has_effect(&initial, ImageOpenEffect::ClearImage));
        assert_eq!(
            initial.state_delta.container_navigation_url,
            ImageOpenUrlTarget::Empty
        );
        assert_eq!(initial.state_delta.status, ImageOpenStatusTarget::Error);

        let explicit_container = rust_image_open_transition(image_open_event(
            ImageOpenWorkflowEventKind::FinishContainerNavigationLoadWithError,
        ));
        assert_eq!(
            explicit_container.state_delta.source_url,
            ImageOpenUrlTarget::Container
        );
        assert!(has_effect(
            &explicit_container,
            ImageOpenEffect::PrepareFailedContainer
        ));
    }
}
