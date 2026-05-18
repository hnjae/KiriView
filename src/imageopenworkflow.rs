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
        Session = 1,
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
    enum RustImageOpenWorkflowEventKind {
        BeginSourceLoad = 0,
        FinishEmptySourceLoad = 1,
        FinishSuccessfulImageLoad = 2,
        FinishSourceLoadWithError = 3,
        FinishContainerNavigationLoadWithError = 4,
        FinishAnimationLoadWithError = 5,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenBeginSourceLoadInput {
        has_image: bool,
        has_loading_container_navigation_target: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenSuccessfulImageLoadInput {
        has_request_container_navigation_target: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenSourceLoadErrorInput {
        has_container_navigation_target: bool,
        has_image: bool,
        has_displayed_url: bool,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenWorkflowEvent {
        kind: RustImageOpenWorkflowEventKind,
        begin_source_load: RustImageOpenBeginSourceLoadInput,
        successful_image_load: RustImageOpenSuccessfulImageLoadInput,
        source_load_error: RustImageOpenSourceLoadErrorInput,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustImageOpenTransition {
        state_delta: RustImageOpenStateDelta,
        effects: Vec<RustImageOpenEffect>,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenStateDelta {
        source_url: RustImageOpenUrlTarget,
        displayed_location: RustImageOpenDisplayedLocationTarget,
        container_navigation_url: RustImageOpenUrlTarget,
        loading: RustImageOpenBoolTarget,
        status: RustImageOpenStatusTarget,
        error_string: RustImageOpenErrorStringTarget,
        clear_loading_container_navigation_url: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustImageOpenTransition"]
        fn rust_image_open_transition(event: RustImageOpenWorkflowEvent)
        -> RustImageOpenTransition;
    }
}

use ffi::{
    RustImageOpenBoolTarget, RustImageOpenDisplayedLocationTarget, RustImageOpenEffect,
    RustImageOpenErrorStringTarget, RustImageOpenStateDelta, RustImageOpenStatusTarget,
    RustImageOpenTransition, RustImageOpenUrlTarget, RustImageOpenWorkflowEvent,
    RustImageOpenWorkflowEventKind,
};

fn rust_image_open_transition(event: RustImageOpenWorkflowEvent) -> RustImageOpenTransition {
    match event.kind {
        RustImageOpenWorkflowEventKind::BeginSourceLoad => begin_source_load_transition(event),
        RustImageOpenWorkflowEventKind::FinishEmptySourceLoad => {
            finish_empty_source_load_transition()
        }
        RustImageOpenWorkflowEventKind::FinishSuccessfulImageLoad => {
            finish_successful_image_load_transition(event)
        }
        RustImageOpenWorkflowEventKind::FinishSourceLoadWithError => {
            source_load_error_transition(event)
        }
        RustImageOpenWorkflowEventKind::FinishContainerNavigationLoadWithError => {
            container_navigation_load_error_transition()
        }
        RustImageOpenWorkflowEventKind::FinishAnimationLoadWithError => {
            animation_load_error_transition()
        }
        _ => empty_transition(),
    }
}

fn begin_source_load_transition(event: RustImageOpenWorkflowEvent) -> RustImageOpenTransition {
    let mut transition = empty_transition();
    let input = event.begin_source_load;

    if !input.has_image && !input.has_loading_container_navigation_target {
        set_container_navigation_url(&mut transition, RustImageOpenUrlTarget::Empty);
    }

    set_loading(&mut transition, RustImageOpenBoolTarget::True);
    if input.has_image {
        set_status(&mut transition, RustImageOpenStatusTarget::Ready);
    } else {
        push_effect(&mut transition, RustImageOpenEffect::ClearImage);
        push_effect(&mut transition, RustImageOpenEffect::ResetZoom);
        set_status(&mut transition, RustImageOpenStatusTarget::Loading);
    }
    transition
}

fn finish_empty_source_load_transition() -> RustImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, RustImageOpenEffect::ClearImage);
    push_effect(&mut transition, RustImageOpenEffect::ResetZoom);
    set_tracked_load_completed(&mut transition);
    set_container_navigation_url(&mut transition, RustImageOpenUrlTarget::Empty);
    set_status(&mut transition, RustImageOpenStatusTarget::Null);
    transition
}

fn finish_successful_image_load_transition(
    event: RustImageOpenWorkflowEvent,
) -> RustImageOpenTransition {
    let mut transition = empty_transition();
    let input = event.successful_image_load;
    set_source_url(&mut transition, RustImageOpenUrlTarget::SessionImage);
    set_displayed_location(
        &mut transition,
        RustImageOpenDisplayedLocationTarget::Session,
    );
    set_container_navigation_url(
        &mut transition,
        if input.has_request_container_navigation_target {
            RustImageOpenUrlTarget::SessionContainerNavigation
        } else {
            RustImageOpenUrlTarget::DerivedContainerNavigation
        },
    );
    set_error_string(&mut transition, RustImageOpenErrorStringTarget::Clear);
    set_tracked_load_completed(&mut transition);
    set_status(&mut transition, RustImageOpenStatusTarget::Ready);
    push_effect(&mut transition, RustImageOpenEffect::UpdatePageNavigation);
    push_effect(
        &mut transition,
        RustImageOpenEffect::ScheduleAdjacentImagePredecode,
    );
    transition
}

fn container_navigation_load_error_transition() -> RustImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, RustImageOpenEffect::ClearImage);
    push_effect(&mut transition, RustImageOpenEffect::PrepareFailedContainer);
    set_tracked_load_completed(&mut transition);
    set_container_navigation_url(&mut transition, RustImageOpenUrlTarget::Container);
    set_source_url(&mut transition, RustImageOpenUrlTarget::Container);
    set_error_string(&mut transition, RustImageOpenErrorStringTarget::Provided);
    set_status(&mut transition, RustImageOpenStatusTarget::Error);
    transition
}

fn replacement_load_error_transition(has_displayed_url: bool) -> RustImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, RustImageOpenEffect::UpdatePageNavigation);
    push_effect(
        &mut transition,
        RustImageOpenEffect::ScheduleAdjacentImagePredecode,
    );
    set_tracked_load_completed(&mut transition);
    if has_displayed_url {
        set_source_url(&mut transition, RustImageOpenUrlTarget::Displayed);
    }
    set_error_string(&mut transition, RustImageOpenErrorStringTarget::Provided);
    set_status(&mut transition, RustImageOpenStatusTarget::Ready);
    transition
}

fn initial_load_error_transition() -> RustImageOpenTransition {
    cleared_load_error_transition(false)
}

fn animation_load_error_transition() -> RustImageOpenTransition {
    cleared_load_error_transition(true)
}

fn source_load_error_transition(event: RustImageOpenWorkflowEvent) -> RustImageOpenTransition {
    let input = event.source_load_error;
    if input.has_container_navigation_target {
        return container_navigation_load_error_transition();
    }
    if input.has_image {
        return replacement_load_error_transition(input.has_displayed_url);
    }

    initial_load_error_transition()
}

fn cleared_load_error_transition(reset_zoom: bool) -> RustImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, RustImageOpenEffect::ClearImage);
    if reset_zoom {
        push_effect(&mut transition, RustImageOpenEffect::ResetZoom);
    }
    set_tracked_load_completed(&mut transition);
    set_container_navigation_url(&mut transition, RustImageOpenUrlTarget::Empty);
    set_error_string(&mut transition, RustImageOpenErrorStringTarget::Provided);
    set_status(&mut transition, RustImageOpenStatusTarget::Error);
    transition
}

fn set_tracked_load_completed(transition: &mut RustImageOpenTransition) {
    transition
        .state_delta
        .clear_loading_container_navigation_url = true;
    set_loading(transition, RustImageOpenBoolTarget::False);
}

fn set_source_url(transition: &mut RustImageOpenTransition, target: RustImageOpenUrlTarget) {
    debug_assert_eq!(
        transition.state_delta.source_url,
        RustImageOpenUrlTarget::Unchanged
    );
    transition.state_delta.source_url = target;
}

fn set_displayed_location(
    transition: &mut RustImageOpenTransition,
    target: RustImageOpenDisplayedLocationTarget,
) {
    debug_assert_eq!(
        transition.state_delta.displayed_location,
        RustImageOpenDisplayedLocationTarget::Unchanged
    );
    transition.state_delta.displayed_location = target;
}

fn set_container_navigation_url(
    transition: &mut RustImageOpenTransition,
    target: RustImageOpenUrlTarget,
) {
    debug_assert_eq!(
        transition.state_delta.container_navigation_url,
        RustImageOpenUrlTarget::Unchanged
    );
    transition.state_delta.container_navigation_url = target;
}

fn set_loading(transition: &mut RustImageOpenTransition, target: RustImageOpenBoolTarget) {
    debug_assert_eq!(
        transition.state_delta.loading,
        RustImageOpenBoolTarget::Unchanged
    );
    transition.state_delta.loading = target;
}

fn set_status(transition: &mut RustImageOpenTransition, target: RustImageOpenStatusTarget) {
    debug_assert_eq!(
        transition.state_delta.status,
        RustImageOpenStatusTarget::Unchanged
    );
    transition.state_delta.status = target;
}

fn set_error_string(
    transition: &mut RustImageOpenTransition,
    target: RustImageOpenErrorStringTarget,
) {
    debug_assert_eq!(
        transition.state_delta.error_string,
        RustImageOpenErrorStringTarget::Unchanged
    );
    transition.state_delta.error_string = target;
}

fn push_effect(transition: &mut RustImageOpenTransition, effect: RustImageOpenEffect) {
    transition.effects.push(effect);
}

fn empty_transition() -> RustImageOpenTransition {
    RustImageOpenTransition {
        state_delta: empty_state_delta(),
        effects: Vec::new(),
    }
}

fn empty_state_delta() -> RustImageOpenStateDelta {
    RustImageOpenStateDelta {
        source_url: RustImageOpenUrlTarget::Unchanged,
        displayed_location: RustImageOpenDisplayedLocationTarget::Unchanged,
        container_navigation_url: RustImageOpenUrlTarget::Unchanged,
        loading: RustImageOpenBoolTarget::Unchanged,
        status: RustImageOpenStatusTarget::Unchanged,
        error_string: RustImageOpenErrorStringTarget::Unchanged,
        clear_loading_container_navigation_url: false,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn has_effect(transition: &RustImageOpenTransition, effect: RustImageOpenEffect) -> bool {
        transition.effects.contains(&effect)
    }

    fn state_delta(
        source_url: RustImageOpenUrlTarget,
        displayed_location: RustImageOpenDisplayedLocationTarget,
        container_navigation_url: RustImageOpenUrlTarget,
        loading: RustImageOpenBoolTarget,
        status: RustImageOpenStatusTarget,
        error_string: RustImageOpenErrorStringTarget,
        clear_loading_container_navigation_url: bool,
    ) -> RustImageOpenStateDelta {
        RustImageOpenStateDelta {
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
        source_url: RustImageOpenUrlTarget,
        displayed_location: RustImageOpenDisplayedLocationTarget,
        container_navigation_url: RustImageOpenUrlTarget,
        status: RustImageOpenStatusTarget,
        error_string: RustImageOpenErrorStringTarget,
    ) -> RustImageOpenStateDelta {
        state_delta(
            source_url,
            displayed_location,
            container_navigation_url,
            RustImageOpenBoolTarget::False,
            status,
            error_string,
            true,
        )
    }

    fn image_open_event(kind: RustImageOpenWorkflowEventKind) -> RustImageOpenWorkflowEvent {
        RustImageOpenWorkflowEvent {
            kind,
            begin_source_load: ffi::RustImageOpenBeginSourceLoadInput {
                has_image: false,
                has_loading_container_navigation_target: false,
            },
            successful_image_load: ffi::RustImageOpenSuccessfulImageLoadInput {
                has_request_container_navigation_target: false,
            },
            source_load_error: ffi::RustImageOpenSourceLoadErrorInput {
                has_container_navigation_target: false,
                has_image: false,
                has_displayed_url: false,
            },
        }
    }

    fn begin_source_load_event(
        has_image: bool,
        has_loading_container_navigation_target: bool,
    ) -> RustImageOpenWorkflowEvent {
        RustImageOpenWorkflowEvent {
            kind: RustImageOpenWorkflowEventKind::BeginSourceLoad,
            begin_source_load: ffi::RustImageOpenBeginSourceLoadInput {
                has_image,
                has_loading_container_navigation_target,
            },
            ..image_open_event(RustImageOpenWorkflowEventKind::BeginSourceLoad)
        }
    }

    fn successful_image_load_event(
        has_request_container_navigation_target: bool,
    ) -> RustImageOpenWorkflowEvent {
        RustImageOpenWorkflowEvent {
            kind: RustImageOpenWorkflowEventKind::FinishSuccessfulImageLoad,
            successful_image_load: ffi::RustImageOpenSuccessfulImageLoadInput {
                has_request_container_navigation_target,
            },
            ..image_open_event(RustImageOpenWorkflowEventKind::FinishSuccessfulImageLoad)
        }
    }

    fn source_load_error_event(
        has_container_navigation_target: bool,
        has_image: bool,
        has_displayed_url: bool,
    ) -> RustImageOpenWorkflowEvent {
        RustImageOpenWorkflowEvent {
            kind: RustImageOpenWorkflowEventKind::FinishSourceLoadWithError,
            source_load_error: ffi::RustImageOpenSourceLoadErrorInput {
                has_container_navigation_target,
                has_image,
                has_displayed_url,
            },
            ..image_open_event(RustImageOpenWorkflowEventKind::FinishSourceLoadWithError)
        }
    }

    #[test]
    fn first_source_load_clears_image_and_enters_loading() {
        let transition = rust_image_open_transition(begin_source_load_event(false, false));

        assert_eq!(
            transition.state_delta,
            state_delta(
                RustImageOpenUrlTarget::Unchanged,
                RustImageOpenDisplayedLocationTarget::Unchanged,
                RustImageOpenUrlTarget::Empty,
                RustImageOpenBoolTarget::True,
                RustImageOpenStatusTarget::Loading,
                RustImageOpenErrorStringTarget::Unchanged,
                false,
            )
        );
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
        let transition = rust_image_open_transition(begin_source_load_event(true, true));

        assert_eq!(
            transition.state_delta,
            state_delta(
                RustImageOpenUrlTarget::Unchanged,
                RustImageOpenDisplayedLocationTarget::Unchanged,
                RustImageOpenUrlTarget::Unchanged,
                RustImageOpenBoolTarget::True,
                RustImageOpenStatusTarget::Ready,
                RustImageOpenErrorStringTarget::Unchanged,
                false,
            )
        );
        assert!(!has_effect(&transition, RustImageOpenEffect::ClearImage));
        assert!(!has_effect(&transition, RustImageOpenEffect::ResetZoom));
    }

    #[test]
    fn first_source_load_preserves_pending_container_navigation_target() {
        let transition = rust_image_open_transition(begin_source_load_event(false, true));

        assert_eq!(
            transition.state_delta.container_navigation_url,
            RustImageOpenUrlTarget::Unchanged
        );
        assert_eq!(
            transition.state_delta.status,
            RustImageOpenStatusTarget::Loading
        );
    }

    #[test]
    fn successful_load_uses_session_targets_and_clears_error() {
        let transition = rust_image_open_transition(successful_image_load_event(true));

        assert_eq!(
            transition.state_delta,
            completed_load_delta(
                RustImageOpenUrlTarget::SessionImage,
                RustImageOpenDisplayedLocationTarget::Session,
                RustImageOpenUrlTarget::SessionContainerNavigation,
                RustImageOpenStatusTarget::Ready,
                RustImageOpenErrorStringTarget::Clear,
            )
        );
        assert_eq!(
            transition.effects,
            vec![
                RustImageOpenEffect::UpdatePageNavigation,
                RustImageOpenEffect::ScheduleAdjacentImagePredecode,
            ]
        );
    }

    #[test]
    fn successful_load_derives_missing_container_navigation_target() {
        let transition = rust_image_open_transition(successful_image_load_event(false));

        assert_eq!(
            transition.state_delta.container_navigation_url,
            RustImageOpenUrlTarget::DerivedContainerNavigation
        );
    }

    #[test]
    fn replacement_failure_restores_displayed_source_and_schedules_predecode() {
        let transition = replacement_load_error_transition(true);

        assert_eq!(
            transition.state_delta,
            completed_load_delta(
                RustImageOpenUrlTarget::Displayed,
                RustImageOpenDisplayedLocationTarget::Unchanged,
                RustImageOpenUrlTarget::Unchanged,
                RustImageOpenStatusTarget::Ready,
                RustImageOpenErrorStringTarget::Provided,
            )
        );
        assert_eq!(
            transition.effects,
            vec![
                RustImageOpenEffect::UpdatePageNavigation,
                RustImageOpenEffect::ScheduleAdjacentImagePredecode,
            ]
        );
    }

    #[test]
    fn initial_and_animation_errors_share_clear_policy_but_only_animation_resets_zoom() {
        let initial = rust_image_open_transition(source_load_error_event(false, false, false));
        let animation = rust_image_open_transition(image_open_event(
            RustImageOpenWorkflowEventKind::FinishAnimationLoadWithError,
        ));

        assert!(has_effect(&initial, RustImageOpenEffect::ClearImage));
        assert!(!has_effect(&initial, RustImageOpenEffect::ResetZoom));
        assert_eq!(
            initial.state_delta.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(initial.state_delta.status, RustImageOpenStatusTarget::Error);

        assert_eq!(
            animation.state_delta,
            completed_load_delta(
                RustImageOpenUrlTarget::Unchanged,
                RustImageOpenDisplayedLocationTarget::Unchanged,
                RustImageOpenUrlTarget::Empty,
                RustImageOpenStatusTarget::Error,
                RustImageOpenErrorStringTarget::Provided,
            )
        );
        assert_eq!(
            animation.effects,
            vec![
                RustImageOpenEffect::ClearImage,
                RustImageOpenEffect::ResetZoom
            ]
        );
    }

    #[test]
    fn routed_load_failure_returns_the_selected_error_transition() {
        let container = rust_image_open_transition(source_load_error_event(true, true, true));
        assert_eq!(
            container.state_delta.source_url,
            RustImageOpenUrlTarget::Container
        );
        assert!(has_effect(&container, RustImageOpenEffect::ClearImage));
        assert!(has_effect(
            &container,
            RustImageOpenEffect::PrepareFailedContainer
        ));
        assert_eq!(
            container.state_delta.status,
            RustImageOpenStatusTarget::Error
        );

        let replacement = rust_image_open_transition(source_load_error_event(false, true, true));
        assert_eq!(
            replacement.state_delta.source_url,
            RustImageOpenUrlTarget::Displayed
        );
        assert!(has_effect(
            &replacement,
            RustImageOpenEffect::UpdatePageNavigation
        ));
        assert!(has_effect(
            &replacement,
            RustImageOpenEffect::ScheduleAdjacentImagePredecode
        ));
        assert_eq!(
            replacement.state_delta.status,
            RustImageOpenStatusTarget::Ready
        );

        let initial = rust_image_open_transition(source_load_error_event(false, false, false));
        assert_eq!(
            initial.state_delta.source_url,
            RustImageOpenUrlTarget::Unchanged
        );
        assert!(has_effect(&initial, RustImageOpenEffect::ClearImage));
        assert_eq!(
            initial.state_delta.container_navigation_url,
            RustImageOpenUrlTarget::Empty
        );
        assert_eq!(initial.state_delta.status, RustImageOpenStatusTarget::Error);

        let explicit_container = rust_image_open_transition(image_open_event(
            RustImageOpenWorkflowEventKind::FinishContainerNavigationLoadWithError,
        ));
        assert_eq!(
            explicit_container.state_delta.source_url,
            RustImageOpenUrlTarget::Container
        );
        assert!(has_effect(
            &explicit_container,
            RustImageOpenEffect::PrepareFailedContainer
        ));
    }
}
