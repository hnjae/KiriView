// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "KiriView")]
mod ffi {
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
    enum RustImageDocumentRightToLeftReadingTransition {
        Keep = 0,
        Reset = 1,
        ResetAndNotifyBeforeSourceState = 2,
        ResetAndNotifyAfterOpen = 3,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentSourceLoadUrlTarget {
        Unchanged = 0,
        Empty = 1,
        RequestedContainerNavigation = 2,
        RequestedSource = 3,
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
        cancel_navigation_and_predecode: bool,
        finish_spread_transition: bool,
        right_to_left_reading_transition: RustImageDocumentRightToLeftReadingTransition,
        clear_secondary_page: bool,
        loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget,
        container_navigation_url: RustImageDocumentSourceLoadUrlTarget,
        source_url: RustImageDocumentSourceLoadUrlTarget,
        begin_open: bool,
    }

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
    enum ImageOpenTransitionOperationKind {
        SourceUrl = 0,
        DisplayedLocation = 1,
        ContainerNavigationUrl = 2,
        Loading = 3,
        Status = 4,
        ErrorString = 5,
        ClearLoadingContainerNavigationUrl = 6,
        Effect = 7,
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
        operations: Vec<ImageOpenTransitionOperation>,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct ImageOpenTransitionOperation {
        kind: ImageOpenTransitionOperationKind,
        url_target: ImageOpenUrlTarget,
        displayed_location_target: ImageOpenDisplayedLocationTarget,
        bool_target: ImageOpenBoolTarget,
        status_target: ImageOpenStatusTarget,
        error_string_target: ImageOpenErrorStringTarget,
        effect: ImageOpenEffect,
    }

    extern "Rust" {
        #[cxx_name = "rustImageDocumentSourceLoadPlan"]
        fn rust_image_document_source_load_plan(
            input: RustImageDocumentSourceLoadPolicyInput,
        ) -> RustImageDocumentSourceLoadPlan;

        #[cxx_name = "rustImageOpenTransition"]
        fn rust_image_open_transition(event: ImageOpenWorkflowEvent) -> ImageOpenTransition;
    }
}

use ffi::{
    ImageOpenBoolTarget, ImageOpenDisplayedLocationTarget, ImageOpenEffect,
    ImageOpenErrorStringTarget, ImageOpenStatusTarget, ImageOpenTransition,
    ImageOpenTransitionOperation, ImageOpenTransitionOperationKind, ImageOpenUrlTarget,
    ImageOpenWorkflowEvent, ImageOpenWorkflowEventKind, RustImageDocumentRightToLeftReadingReset,
    RustImageDocumentRightToLeftReadingTransition, RustImageDocumentSourceLoadKind,
    RustImageDocumentSourceLoadPlan, RustImageDocumentSourceLoadPolicyInput,
    RustImageDocumentSourceLoadUrlTarget,
};

fn rust_image_document_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    if replaces_source(input) {
        return replacement_source_load_plan(input);
    }

    current_source_load_plan(input)
}

fn replaces_source(input: RustImageDocumentSourceLoadPolicyInput) -> bool {
    input.load_kind == RustImageDocumentSourceLoadKind::ReplacementSource
}

fn current_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    RustImageDocumentSourceLoadPlan {
        cancel_navigation_and_predecode: false,
        finish_spread_transition: !input.preserve_two_page_spread_transition,
        right_to_left_reading_transition: right_to_left_reading_transition(input, false),
        clear_secondary_page: false,
        loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Empty,
        container_navigation_url: if input.has_requested_container_navigation_url {
            RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation
        } else {
            RustImageDocumentSourceLoadUrlTarget::Unchanged
        },
        source_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
        begin_open: false,
    }
}

fn replacement_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    RustImageDocumentSourceLoadPlan {
        cancel_navigation_and_predecode: true,
        finish_spread_transition: !input.preserve_two_page_spread_transition,
        right_to_left_reading_transition: right_to_left_reading_transition(input, true),
        clear_secondary_page: true,
        loading_container_navigation_url:
            RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
        container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
        source_url: RustImageDocumentSourceLoadUrlTarget::RequestedSource,
        begin_open: true,
    }
}

fn right_to_left_reading_transition(
    input: RustImageDocumentSourceLoadPolicyInput,
    replacement_source: bool,
) -> RustImageDocumentRightToLeftReadingTransition {
    match input.right_to_left_reading_reset {
        RustImageDocumentRightToLeftReadingReset::Keep => {
            RustImageDocumentRightToLeftReadingTransition::Keep
        }
        RustImageDocumentRightToLeftReadingReset::ResetInactive => {
            RustImageDocumentRightToLeftReadingTransition::Reset
        }
        RustImageDocumentRightToLeftReadingReset::ResetActive => {
            if replacement_source {
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen
            } else {
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState
            }
        }
        _ => RustImageDocumentRightToLeftReadingTransition::Keep,
    }
}

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
        push_container_navigation_url(&mut transition, ImageOpenUrlTarget::Empty);
    }

    push_loading(&mut transition, ImageOpenBoolTarget::True);
    if event.has_image {
        push_status(&mut transition, ImageOpenStatusTarget::Ready);
    } else {
        push_effect(&mut transition, ImageOpenEffect::ClearImage);
        push_effect(&mut transition, ImageOpenEffect::ResetZoom);
        push_status(&mut transition, ImageOpenStatusTarget::Loading);
    }
    transition
}

fn finish_empty_source_load_transition() -> ImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, ImageOpenEffect::ClearImage);
    push_effect(&mut transition, ImageOpenEffect::ResetZoom);
    push_tracked_load_completion(&mut transition);
    push_container_navigation_url(&mut transition, ImageOpenUrlTarget::Empty);
    push_status(&mut transition, ImageOpenStatusTarget::Null);
    transition
}

fn finish_successful_image_load_transition(event: ImageOpenWorkflowEvent) -> ImageOpenTransition {
    let mut transition = empty_transition();
    push_source_url(&mut transition, ImageOpenUrlTarget::SessionImage);
    push_displayed_location(&mut transition, ImageOpenDisplayedLocationTarget::Session);
    push_container_navigation_url(
        &mut transition,
        if event.request_container_navigation_url_empty {
            ImageOpenUrlTarget::DerivedContainerNavigation
        } else {
            ImageOpenUrlTarget::SessionContainerNavigation
        },
    );
    push_error_string(&mut transition, ImageOpenErrorStringTarget::Clear);
    push_tracked_load_completion(&mut transition);
    push_status(&mut transition, ImageOpenStatusTarget::Ready);
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
    push_tracked_load_completion(&mut transition);
    push_container_navigation_url(&mut transition, ImageOpenUrlTarget::Container);
    push_source_url(&mut transition, ImageOpenUrlTarget::Container);
    push_error_string(&mut transition, ImageOpenErrorStringTarget::Provided);
    push_status(&mut transition, ImageOpenStatusTarget::Error);
    transition
}

fn replacement_load_error_transition(displayed_url_empty: bool) -> ImageOpenTransition {
    let mut transition = empty_transition();
    push_effect(&mut transition, ImageOpenEffect::UpdatePageNavigation);
    push_effect(
        &mut transition,
        ImageOpenEffect::ScheduleAdjacentImagePredecode,
    );
    push_tracked_load_completion(&mut transition);
    if !displayed_url_empty {
        push_source_url(&mut transition, ImageOpenUrlTarget::Displayed);
    }
    push_error_string(&mut transition, ImageOpenErrorStringTarget::Provided);
    push_status(&mut transition, ImageOpenStatusTarget::Ready);
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
    push_tracked_load_completion(&mut transition);
    push_container_navigation_url(&mut transition, ImageOpenUrlTarget::Empty);
    push_error_string(&mut transition, ImageOpenErrorStringTarget::Provided);
    push_status(&mut transition, ImageOpenStatusTarget::Error);
    transition
}

fn push_tracked_load_completion(transition: &mut ImageOpenTransition) {
    transition.operations.push(operation(
        ImageOpenTransitionOperationKind::ClearLoadingContainerNavigationUrl,
    ));
    push_loading(transition, ImageOpenBoolTarget::False);
}

fn push_source_url(transition: &mut ImageOpenTransition, target: ImageOpenUrlTarget) {
    transition.operations.push(url_operation(
        ImageOpenTransitionOperationKind::SourceUrl,
        target,
    ));
}

fn push_displayed_location(
    transition: &mut ImageOpenTransition,
    target: ImageOpenDisplayedLocationTarget,
) {
    let mut operation = operation(ImageOpenTransitionOperationKind::DisplayedLocation);
    operation.displayed_location_target = target;
    transition.operations.push(operation);
}

fn push_container_navigation_url(transition: &mut ImageOpenTransition, target: ImageOpenUrlTarget) {
    transition.operations.push(url_operation(
        ImageOpenTransitionOperationKind::ContainerNavigationUrl,
        target,
    ));
}

fn push_loading(transition: &mut ImageOpenTransition, target: ImageOpenBoolTarget) {
    let mut operation = operation(ImageOpenTransitionOperationKind::Loading);
    operation.bool_target = target;
    transition.operations.push(operation);
}

fn push_status(transition: &mut ImageOpenTransition, target: ImageOpenStatusTarget) {
    let mut operation = operation(ImageOpenTransitionOperationKind::Status);
    operation.status_target = target;
    transition.operations.push(operation);
}

fn push_error_string(transition: &mut ImageOpenTransition, target: ImageOpenErrorStringTarget) {
    let mut operation = operation(ImageOpenTransitionOperationKind::ErrorString);
    operation.error_string_target = target;
    transition.operations.push(operation);
}

fn push_effect(transition: &mut ImageOpenTransition, effect: ImageOpenEffect) {
    let mut operation = operation(ImageOpenTransitionOperationKind::Effect);
    operation.effect = effect;
    transition.operations.push(operation);
}

fn url_operation(
    kind: ImageOpenTransitionOperationKind,
    target: ImageOpenUrlTarget,
) -> ImageOpenTransitionOperation {
    let mut operation = operation(kind);
    operation.url_target = target;
    operation
}

fn operation(kind: ImageOpenTransitionOperationKind) -> ImageOpenTransitionOperation {
    ImageOpenTransitionOperation {
        kind,
        url_target: ImageOpenUrlTarget::Unchanged,
        displayed_location_target: ImageOpenDisplayedLocationTarget::Unchanged,
        bool_target: ImageOpenBoolTarget::Unchanged,
        status_target: ImageOpenStatusTarget::Unchanged,
        error_string_target: ImageOpenErrorStringTarget::Unchanged,
        effect: ImageOpenEffect::ClearImage,
    }
}

fn empty_transition() -> ImageOpenTransition {
    ImageOpenTransition {
        operations: Vec::new(),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn has_effect(transition: &ImageOpenTransition, effect: ImageOpenEffect) -> bool {
        transition.operations.iter().any(|operation| {
            operation.kind == ImageOpenTransitionOperationKind::Effect && operation.effect == effect
        })
    }

    fn effect_operation(effect: ImageOpenEffect) -> ImageOpenTransitionOperation {
        let mut operation = operation(ImageOpenTransitionOperationKind::Effect);
        operation.effect = effect;
        operation
    }

    fn source_url_operation(target: ImageOpenUrlTarget) -> ImageOpenTransitionOperation {
        url_operation(ImageOpenTransitionOperationKind::SourceUrl, target)
    }

    fn container_navigation_url_operation(
        target: ImageOpenUrlTarget,
    ) -> ImageOpenTransitionOperation {
        url_operation(
            ImageOpenTransitionOperationKind::ContainerNavigationUrl,
            target,
        )
    }

    fn displayed_location_operation(
        target: ImageOpenDisplayedLocationTarget,
    ) -> ImageOpenTransitionOperation {
        let mut operation = operation(ImageOpenTransitionOperationKind::DisplayedLocation);
        operation.displayed_location_target = target;
        operation
    }

    fn loading_operation(target: ImageOpenBoolTarget) -> ImageOpenTransitionOperation {
        let mut operation = operation(ImageOpenTransitionOperationKind::Loading);
        operation.bool_target = target;
        operation
    }

    fn status_operation(target: ImageOpenStatusTarget) -> ImageOpenTransitionOperation {
        let mut operation = operation(ImageOpenTransitionOperationKind::Status);
        operation.status_target = target;
        operation
    }

    fn error_string_operation(target: ImageOpenErrorStringTarget) -> ImageOpenTransitionOperation {
        let mut operation = operation(ImageOpenTransitionOperationKind::ErrorString);
        operation.error_string_target = target;
        operation
    }

    fn clear_loading_container_navigation_url_operation() -> ImageOpenTransitionOperation {
        operation(ImageOpenTransitionOperationKind::ClearLoadingContainerNavigationUrl)
    }

    fn document_source_load_input(
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

    fn current_source_plan(
        finish_spread_transition: bool,
        right_to_left_reading_transition: RustImageDocumentRightToLeftReadingTransition,
        container_navigation_url: RustImageDocumentSourceLoadUrlTarget,
    ) -> RustImageDocumentSourceLoadPlan {
        RustImageDocumentSourceLoadPlan {
            cancel_navigation_and_predecode: false,
            finish_spread_transition,
            right_to_left_reading_transition,
            clear_secondary_page: false,
            loading_container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Empty,
            container_navigation_url,
            source_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
            begin_open: false,
        }
    }

    fn replacement_source_plan(
        finish_spread_transition: bool,
        right_to_left_reading_transition: RustImageDocumentRightToLeftReadingTransition,
    ) -> RustImageDocumentSourceLoadPlan {
        RustImageDocumentSourceLoadPlan {
            cancel_navigation_and_predecode: true,
            finish_spread_transition,
            right_to_left_reading_transition,
            clear_secondary_page: true,
            loading_container_navigation_url:
                RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
            container_navigation_url: RustImageDocumentSourceLoadUrlTarget::Unchanged,
            source_url: RustImageDocumentSourceLoadUrlTarget::RequestedSource,
            begin_open: true,
        }
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
    fn source_load_plan_routes_unchanged_and_replacement_loads() {
        let unchanged = rust_image_document_source_load_plan(document_source_load_input(
            RustImageDocumentSourceLoadKind::CurrentSource,
            false,
            RustImageDocumentRightToLeftReadingReset::ResetActive,
            true,
        ));
        assert_eq!(
            unchanged,
            current_source_plan(
                true,
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState,
                RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation,
            )
        );

        let replacement = rust_image_document_source_load_plan(document_source_load_input(
            RustImageDocumentSourceLoadKind::ReplacementSource,
            true,
            RustImageDocumentRightToLeftReadingReset::Keep,
            false,
        ));
        assert_eq!(
            replacement,
            replacement_source_plan(false, RustImageDocumentRightToLeftReadingTransition::Keep)
        );

        let inactive_reset_replacement =
            rust_image_document_source_load_plan(document_source_load_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                RustImageDocumentRightToLeftReadingReset::ResetInactive,
                false,
            ));
        assert_eq!(
            inactive_reset_replacement,
            replacement_source_plan(false, RustImageDocumentRightToLeftReadingTransition::Reset)
        );

        let resetting_replacement =
            rust_image_document_source_load_plan(document_source_load_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                RustImageDocumentRightToLeftReadingReset::ResetActive,
                false,
            ));
        assert_eq!(
            resetting_replacement,
            replacement_source_plan(
                false,
                RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen
            )
        );
    }

    #[test]
    fn first_source_load_clears_image_and_enters_loading() {
        let transition = rust_image_open_transition(begin_source_load_event(false, true));

        assert_eq!(
            transition.operations,
            vec![
                container_navigation_url_operation(ImageOpenUrlTarget::Empty),
                loading_operation(ImageOpenBoolTarget::True),
                effect_operation(ImageOpenEffect::ClearImage),
                effect_operation(ImageOpenEffect::ResetZoom),
                status_operation(ImageOpenStatusTarget::Loading),
            ]
        );
    }

    #[test]
    fn replacement_source_load_preserves_image_and_enters_ready() {
        let transition = rust_image_open_transition(begin_source_load_event(true, true));

        assert_eq!(
            transition.operations,
            vec![
                loading_operation(ImageOpenBoolTarget::True),
                status_operation(ImageOpenStatusTarget::Ready),
            ]
        );
        assert!(!has_effect(&transition, ImageOpenEffect::ClearImage));
        assert!(!has_effect(&transition, ImageOpenEffect::ResetZoom));
    }

    #[test]
    fn successful_load_uses_session_targets_and_clears_error() {
        let transition = rust_image_open_transition(successful_image_load_event(false));

        assert_eq!(
            transition.operations,
            vec![
                source_url_operation(ImageOpenUrlTarget::SessionImage),
                displayed_location_operation(ImageOpenDisplayedLocationTarget::Session),
                container_navigation_url_operation(ImageOpenUrlTarget::SessionContainerNavigation),
                error_string_operation(ImageOpenErrorStringTarget::Clear),
                clear_loading_container_navigation_url_operation(),
                loading_operation(ImageOpenBoolTarget::False),
                status_operation(ImageOpenStatusTarget::Ready),
                effect_operation(ImageOpenEffect::UpdatePageNavigation),
                effect_operation(ImageOpenEffect::ScheduleAdjacentImagePredecode),
            ]
        );
    }

    #[test]
    fn replacement_failure_restores_displayed_source_and_schedules_predecode() {
        let transition = replacement_load_error_transition(false);

        assert_eq!(
            transition.operations,
            vec![
                effect_operation(ImageOpenEffect::UpdatePageNavigation),
                effect_operation(ImageOpenEffect::ScheduleAdjacentImagePredecode),
                clear_loading_container_navigation_url_operation(),
                loading_operation(ImageOpenBoolTarget::False),
                source_url_operation(ImageOpenUrlTarget::Displayed),
                error_string_operation(ImageOpenErrorStringTarget::Provided),
                status_operation(ImageOpenStatusTarget::Ready),
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
        assert!(
            initial
                .operations
                .contains(&container_navigation_url_operation(
                    ImageOpenUrlTarget::Empty
                ))
        );
        assert!(
            initial
                .operations
                .contains(&status_operation(ImageOpenStatusTarget::Error))
        );

        assert_eq!(
            animation.operations,
            vec![
                effect_operation(ImageOpenEffect::ClearImage),
                effect_operation(ImageOpenEffect::ResetZoom),
                clear_loading_container_navigation_url_operation(),
                loading_operation(ImageOpenBoolTarget::False),
                container_navigation_url_operation(ImageOpenUrlTarget::Empty),
                error_string_operation(ImageOpenErrorStringTarget::Provided),
                status_operation(ImageOpenStatusTarget::Error),
            ]
        );
    }

    #[test]
    fn routed_load_failure_returns_the_selected_error_transition() {
        let container = rust_image_open_transition(source_load_error_event(false, true, false));
        assert!(
            container
                .operations
                .contains(&source_url_operation(ImageOpenUrlTarget::Container))
        );
        assert!(has_effect(&container, ImageOpenEffect::ClearImage));
        assert!(has_effect(
            &container,
            ImageOpenEffect::PrepareFailedContainer
        ));
        assert!(
            container
                .operations
                .contains(&status_operation(ImageOpenStatusTarget::Error))
        );

        let replacement = rust_image_open_transition(source_load_error_event(true, true, false));
        assert!(
            replacement
                .operations
                .contains(&source_url_operation(ImageOpenUrlTarget::Displayed))
        );
        assert!(has_effect(
            &replacement,
            ImageOpenEffect::UpdatePageNavigation
        ));
        assert!(has_effect(
            &replacement,
            ImageOpenEffect::ScheduleAdjacentImagePredecode
        ));
        assert!(
            replacement
                .operations
                .contains(&status_operation(ImageOpenStatusTarget::Ready))
        );

        let initial = rust_image_open_transition(source_load_error_event(true, false, true));
        assert!(
            !initial
                .operations
                .iter()
                .any(|operation| { operation.kind == ImageOpenTransitionOperationKind::SourceUrl })
        );
        assert!(has_effect(&initial, ImageOpenEffect::ClearImage));
        assert!(
            initial
                .operations
                .contains(&container_navigation_url_operation(
                    ImageOpenUrlTarget::Empty
                ))
        );
        assert!(
            initial
                .operations
                .contains(&status_operation(ImageOpenStatusTarget::Error))
        );

        let explicit_container = rust_image_open_transition(image_open_event(
            ImageOpenWorkflowEventKind::FinishContainerNavigationLoadWithError,
        ));
        assert!(
            explicit_container
                .operations
                .contains(&source_url_operation(ImageOpenUrlTarget::Container))
        );
        assert!(has_effect(
            &explicit_container,
            ImageOpenEffect::PrepareFailedContainer
        ));
    }
}
