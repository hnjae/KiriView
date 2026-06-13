// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#[cxx::bridge(namespace = "kiriview")]
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
    enum RustImageOpenSourceKindTarget {
        Unchanged = 0,
        Session = 1,
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
        ClearLoadingPresentation = 5,
        FinishSpreadTransition = 6,
        ClearSecondaryPage = 7,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageOpenWorkflowEventKind {
        BeginSourceLoad = 0,
        FinishEmptySourceLoad = 1,
        FinishSuccessfulImageLoad = 2,
        FinishSourceLoadWithError = 3,
        FinishContainerNavigationLoadWithError = 4,
        FinishAnimationLoadWithError = 5,
        ResolveSourceImage = 6,
        FinishUnsupportedOpenedCollectionVideoLoad = 7,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentSourceLoadKind {
        CurrentSource = 0,
        ReplacementSource = 1,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    enum RustImageDocumentSourceLoadOperation {
        CancelFileDeletion = 0,
        CancelAllNavigation = 1,
        CancelPredecode = 2,
        FinishSpreadTransition = 3,
        ResetRightToLeftReading = 4,
        NotifyRightToLeftReadingChanged = 5,
        ClearSecondaryPage = 6,
        ClearLoadingContainerNavigationUrl = 7,
        SetLoadingContainerNavigationUrlToRequested = 8,
        SetContainerNavigationUrlToRequested = 9,
        PrepareSourceLoad = 10,
        SetSourceUrlToRequested = 11,
        BeginOpen = 12,
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

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageDocumentSourceLoadPolicyInput {
        load_kind: RustImageDocumentSourceLoadKind,
        preserve_two_page_spread_transition: bool,
        right_to_left_reading_enabled: bool,
        source_within_displayed_comic_book_archive: bool,
        has_requested_container_navigation_url: bool,
    }

    #[derive(Debug, PartialEq, Eq)]
    struct RustImageOpenTransition {
        state_delta: RustImageOpenStateDelta,
        effects: Vec<RustImageOpenEffect>,
    }

    #[derive(Clone, Debug, PartialEq, Eq)]
    struct RustImageDocumentSourceLoadPlan {
        operations: Vec<RustImageDocumentSourceLoadOperation>,
    }

    #[derive(Clone, Copy, Debug, PartialEq, Eq)]
    struct RustImageOpenStateDelta {
        source_url: RustImageOpenUrlTarget,
        source_kind: RustImageOpenSourceKindTarget,
        displayed_location: RustImageOpenDisplayedLocationTarget,
        container_navigation_url: RustImageOpenUrlTarget,
        loading: RustImageOpenBoolTarget,
        status: RustImageOpenStatusTarget,
        error_string: RustImageOpenErrorStringTarget,
        unsupported_opened_collection_video: RustImageOpenBoolTarget,
        clear_embedded_metadata: bool,
        clear_loading_container_navigation_url: bool,
    }

    extern "Rust" {
        #[cxx_name = "rustImageOpenTransition"]
        fn rust_image_open_transition(event: RustImageOpenWorkflowEvent)
        -> RustImageOpenTransition;

        #[cxx_name = "rustImageDocumentSourceLoadPlan"]
        fn rust_image_document_source_load_plan(
            input: RustImageDocumentSourceLoadPolicyInput,
        ) -> RustImageDocumentSourceLoadPlan;
    }
}

use ffi::{
    RustImageDocumentSourceLoadKind, RustImageDocumentSourceLoadOperation,
    RustImageDocumentSourceLoadPlan, RustImageDocumentSourceLoadPolicyInput,
    RustImageOpenBoolTarget, RustImageOpenDisplayedLocationTarget, RustImageOpenEffect,
    RustImageOpenErrorStringTarget, RustImageOpenSourceKindTarget, RustImageOpenStateDelta,
    RustImageOpenStatusTarget, RustImageOpenTransition, RustImageOpenUrlTarget,
    RustImageOpenWorkflowEvent, RustImageOpenWorkflowEventKind,
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
        RustImageOpenWorkflowEventKind::ResolveSourceImage => resolve_source_image_transition(),
        RustImageOpenWorkflowEventKind::FinishUnsupportedOpenedCollectionVideoLoad => {
            unsupported_opened_collection_video_transition()
        }
        _ => empty_transition(),
    }
}

fn rust_image_document_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    match input.load_kind {
        RustImageDocumentSourceLoadKind::CurrentSource => current_source_load_plan(input),
        RustImageDocumentSourceLoadKind::ReplacementSource => replacement_source_load_plan(input),
        _ => current_source_load_plan(input),
    }
}

fn current_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    let mut plan = source_load_plan();
    push_source_load_if(
        &mut plan,
        !input.preserve_two_page_spread_transition,
        RustImageDocumentSourceLoadOperation::FinishSpreadTransition,
    );
    apply_right_to_left_reading_transition(&mut plan, input, false);
    push_source_load_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl,
    );
    if input.has_requested_container_navigation_url {
        push_source_load_operation(
            &mut plan,
            RustImageDocumentSourceLoadOperation::SetContainerNavigationUrlToRequested,
        );
    }
    plan
}

fn replacement_source_load_plan(
    input: RustImageDocumentSourceLoadPolicyInput,
) -> RustImageDocumentSourceLoadPlan {
    let mut plan = source_load_plan();
    push_source_load_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::CancelAllNavigation,
    );
    push_source_load_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::CancelPredecode,
    );
    push_source_load_if(
        &mut plan,
        !input.preserve_two_page_spread_transition,
        RustImageDocumentSourceLoadOperation::FinishSpreadTransition,
    );
    apply_right_to_left_reading_transition(&mut plan, input, true);
    push_source_load_if(
        &mut plan,
        !input.preserve_two_page_spread_transition,
        RustImageDocumentSourceLoadOperation::ClearSecondaryPage,
    );
    push_source_load_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
    );
    push_source_load_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
    );
    push_source_load_operation(
        &mut plan,
        RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
    );
    push_source_load_operation(&mut plan, RustImageDocumentSourceLoadOperation::BeginOpen);
    if right_to_left_reading_reset_applies(input) {
        push_source_load_operation(
            &mut plan,
            RustImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged,
        );
    }
    plan
}

fn apply_right_to_left_reading_transition(
    plan: &mut RustImageDocumentSourceLoadPlan,
    input: RustImageDocumentSourceLoadPolicyInput,
    replacement_source: bool,
) {
    if !right_to_left_reading_reset_applies(input) {
        return;
    }

    push_source_load_operation(
        plan,
        RustImageDocumentSourceLoadOperation::ResetRightToLeftReading,
    );
    if !replacement_source {
        push_source_load_operation(
            plan,
            RustImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged,
        );
    }
}

fn right_to_left_reading_reset_applies(input: RustImageDocumentSourceLoadPolicyInput) -> bool {
    input.right_to_left_reading_enabled
        && !input.has_requested_container_navigation_url
        && !input.source_within_displayed_comic_book_archive
}

fn push_source_load_if(
    plan: &mut RustImageDocumentSourceLoadPlan,
    condition: bool,
    operation: RustImageDocumentSourceLoadOperation,
) {
    if condition {
        push_source_load_operation(plan, operation);
    }
}

fn push_source_load_operation(
    plan: &mut RustImageDocumentSourceLoadPlan,
    operation: RustImageDocumentSourceLoadOperation,
) {
    plan.operations.push(operation);
}

fn source_load_plan() -> RustImageDocumentSourceLoadPlan {
    RustImageDocumentSourceLoadPlan {
        operations: vec![RustImageDocumentSourceLoadOperation::CancelFileDeletion],
    }
}

fn begin_source_load_transition(event: RustImageOpenWorkflowEvent) -> RustImageOpenTransition {
    let mut transition = empty_transition();
    let input = event.begin_source_load;
    set_unsupported_opened_collection_video(&mut transition, RustImageOpenBoolTarget::False);
    set_embedded_metadata_cleared(&mut transition);
    set_error_string(&mut transition, RustImageOpenErrorStringTarget::Clear);

    if !input.has_image && !input.has_loading_container_navigation_target {
        set_container_navigation_url(&mut transition, RustImageOpenUrlTarget::Empty);
    }

    set_loading(&mut transition, RustImageOpenBoolTarget::True);
    if input.has_image {
        push_effect(
            &mut transition,
            RustImageOpenEffect::ClearLoadingPresentation,
        );
        set_status(&mut transition, RustImageOpenStatusTarget::Loading);
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
    set_unsupported_opened_collection_video(&mut transition, RustImageOpenBoolTarget::False);
    set_embedded_metadata_cleared(&mut transition);
    set_error_string(&mut transition, RustImageOpenErrorStringTarget::Clear);
    set_tracked_load_completed(&mut transition);
    set_container_navigation_url(&mut transition, RustImageOpenUrlTarget::Empty);
    set_status(&mut transition, RustImageOpenStatusTarget::Null);
    transition
}

fn resolve_source_image_transition() -> RustImageOpenTransition {
    let mut transition = empty_transition();
    set_source_url(&mut transition, RustImageOpenUrlTarget::SessionImage);
    set_source_kind(&mut transition, RustImageOpenSourceKindTarget::Session);
    transition
}

fn unsupported_opened_collection_video_transition() -> RustImageOpenTransition {
    let mut transition = empty_transition();
    set_source_url(&mut transition, RustImageOpenUrlTarget::SessionImage);
    set_source_kind(&mut transition, RustImageOpenSourceKindTarget::Session);
    set_displayed_location(
        &mut transition,
        RustImageOpenDisplayedLocationTarget::Session,
    );
    set_container_navigation_url(
        &mut transition,
        RustImageOpenUrlTarget::DerivedContainerNavigation,
    );
    set_error_string(&mut transition, RustImageOpenErrorStringTarget::Clear);
    set_embedded_metadata_cleared(&mut transition);
    set_tracked_load_completed(&mut transition);
    set_status(&mut transition, RustImageOpenStatusTarget::Ready);
    set_unsupported_opened_collection_video(&mut transition, RustImageOpenBoolTarget::True);
    push_effect(&mut transition, RustImageOpenEffect::FinishSpreadTransition);
    push_effect(&mut transition, RustImageOpenEffect::ClearSecondaryPage);
    push_effect(&mut transition, RustImageOpenEffect::UpdatePageNavigation);
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
    set_unsupported_opened_collection_video(&mut transition, RustImageOpenBoolTarget::False);
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
    set_unsupported_opened_collection_video(&mut transition, RustImageOpenBoolTarget::False);
    set_status(&mut transition, RustImageOpenStatusTarget::Error);
    transition
}

fn source_target_load_error_transition() -> RustImageOpenTransition {
    let mut transition = empty_transition();
    set_tracked_load_completed(&mut transition);
    set_container_navigation_url(&mut transition, RustImageOpenUrlTarget::Empty);
    set_error_string(&mut transition, RustImageOpenErrorStringTarget::Provided);
    set_unsupported_opened_collection_video(&mut transition, RustImageOpenBoolTarget::False);
    set_embedded_metadata_cleared(&mut transition);
    set_status(&mut transition, RustImageOpenStatusTarget::Error);
    transition
}

fn initial_load_error_transition() -> RustImageOpenTransition {
    source_target_load_error_transition()
}

fn animation_load_error_transition() -> RustImageOpenTransition {
    cleared_load_error_transition(true)
}

fn source_load_error_transition(event: RustImageOpenWorkflowEvent) -> RustImageOpenTransition {
    let input = event.source_load_error;
    if input.has_container_navigation_target {
        return container_navigation_load_error_transition();
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

fn set_source_kind(
    transition: &mut RustImageOpenTransition,
    target: RustImageOpenSourceKindTarget,
) {
    debug_assert_eq!(
        transition.state_delta.source_kind,
        RustImageOpenSourceKindTarget::Unchanged
    );
    transition.state_delta.source_kind = target;
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

fn set_unsupported_opened_collection_video(
    transition: &mut RustImageOpenTransition,
    target: RustImageOpenBoolTarget,
) {
    debug_assert_eq!(
        transition.state_delta.unsupported_opened_collection_video,
        RustImageOpenBoolTarget::Unchanged
    );
    transition.state_delta.unsupported_opened_collection_video = target;
}

fn set_embedded_metadata_cleared(transition: &mut RustImageOpenTransition) {
    debug_assert!(!transition.state_delta.clear_embedded_metadata);
    transition.state_delta.clear_embedded_metadata = true;
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
        source_kind: RustImageOpenSourceKindTarget::Unchanged,
        displayed_location: RustImageOpenDisplayedLocationTarget::Unchanged,
        container_navigation_url: RustImageOpenUrlTarget::Unchanged,
        loading: RustImageOpenBoolTarget::Unchanged,
        status: RustImageOpenStatusTarget::Unchanged,
        error_string: RustImageOpenErrorStringTarget::Unchanged,
        unsupported_opened_collection_video: RustImageOpenBoolTarget::Unchanged,
        clear_embedded_metadata: false,
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
            source_kind: RustImageOpenSourceKindTarget::Unchanged,
            displayed_location,
            container_navigation_url,
            loading,
            status,
            error_string,
            unsupported_opened_collection_video: RustImageOpenBoolTarget::Unchanged,
            clear_embedded_metadata: false,
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

    fn with_unsupported_opened_collection_video_cleared(
        mut delta: RustImageOpenStateDelta,
    ) -> RustImageOpenStateDelta {
        delta.unsupported_opened_collection_video = RustImageOpenBoolTarget::False;
        delta
    }

    fn with_embedded_metadata_cleared(
        mut delta: RustImageOpenStateDelta,
    ) -> RustImageOpenStateDelta {
        delta.clear_embedded_metadata = true;
        delta
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
            with_embedded_metadata_cleared(with_unsupported_opened_collection_video_cleared(
                state_delta(
                    RustImageOpenUrlTarget::Unchanged,
                    RustImageOpenDisplayedLocationTarget::Unchanged,
                    RustImageOpenUrlTarget::Empty,
                    RustImageOpenBoolTarget::True,
                    RustImageOpenStatusTarget::Loading,
                    RustImageOpenErrorStringTarget::Clear,
                    false,
                ),
            ))
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
    fn replacement_source_load_clears_presentation_and_enters_loading() {
        let transition = rust_image_open_transition(begin_source_load_event(true, true));

        assert_eq!(
            transition.state_delta,
            with_embedded_metadata_cleared(with_unsupported_opened_collection_video_cleared(
                state_delta(
                    RustImageOpenUrlTarget::Unchanged,
                    RustImageOpenDisplayedLocationTarget::Unchanged,
                    RustImageOpenUrlTarget::Unchanged,
                    RustImageOpenBoolTarget::True,
                    RustImageOpenStatusTarget::Loading,
                    RustImageOpenErrorStringTarget::Clear,
                    false,
                ),
            ))
        );
        assert!(has_effect(
            &transition,
            RustImageOpenEffect::ClearLoadingPresentation
        ));
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
            with_unsupported_opened_collection_video_cleared(completed_load_delta(
                RustImageOpenUrlTarget::SessionImage,
                RustImageOpenDisplayedLocationTarget::Session,
                RustImageOpenUrlTarget::SessionContainerNavigation,
                RustImageOpenStatusTarget::Ready,
                RustImageOpenErrorStringTarget::Clear,
            ))
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
    fn source_resolution_uses_session_image_without_completing_load() {
        let transition = rust_image_open_transition(image_open_event(
            RustImageOpenWorkflowEventKind::ResolveSourceImage,
        ));
        let mut expected = state_delta(
            RustImageOpenUrlTarget::SessionImage,
            RustImageOpenDisplayedLocationTarget::Unchanged,
            RustImageOpenUrlTarget::Unchanged,
            RustImageOpenBoolTarget::Unchanged,
            RustImageOpenStatusTarget::Unchanged,
            RustImageOpenErrorStringTarget::Unchanged,
            false,
        );
        expected.source_kind = RustImageOpenSourceKindTarget::Session;

        assert_eq!(transition.state_delta, expected);
        assert!(transition.effects.is_empty());
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
    fn source_load_failure_keeps_target_error_without_recovery_effects() {
        let transition = source_target_load_error_transition();

        assert_eq!(
            transition.state_delta,
            with_embedded_metadata_cleared(with_unsupported_opened_collection_video_cleared(
                completed_load_delta(
                    RustImageOpenUrlTarget::Unchanged,
                    RustImageOpenDisplayedLocationTarget::Unchanged,
                    RustImageOpenUrlTarget::Empty,
                    RustImageOpenStatusTarget::Error,
                    RustImageOpenErrorStringTarget::Provided,
                ),
            ))
        );
        assert!(transition.effects.is_empty());
    }

    #[test]
    fn source_and_animation_errors_are_distinct() {
        let initial = rust_image_open_transition(source_load_error_event(false, false, false));
        let animation = rust_image_open_transition(image_open_event(
            RustImageOpenWorkflowEventKind::FinishAnimationLoadWithError,
        ));

        assert!(!has_effect(&initial, RustImageOpenEffect::ClearImage));
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
            RustImageOpenUrlTarget::Unchanged
        );
        assert!(replacement.effects.is_empty());
        assert_eq!(
            replacement.state_delta.status,
            RustImageOpenStatusTarget::Error
        );

        let initial = rust_image_open_transition(source_load_error_event(false, false, false));
        assert_eq!(
            initial.state_delta.source_url,
            RustImageOpenUrlTarget::Unchanged
        );
        assert!(!has_effect(&initial, RustImageOpenEffect::ClearImage));
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

    #[test]
    fn current_source_load_plan_uses_reading_and_container_snapshots() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                true,
                false,
                false,
                false,
            )),
            RustImageDocumentSourceLoadPlan {
                operations: vec![
                    RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                    RustImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl,
                ],
            }
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                false,
                true,
                false,
                true,
            )),
            RustImageDocumentSourceLoadPlan {
                operations: vec![
                    RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                    RustImageDocumentSourceLoadOperation::FinishSpreadTransition,
                    RustImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl,
                    RustImageDocumentSourceLoadOperation::SetContainerNavigationUrlToRequested,
                ],
            }
        );
    }

    #[test]
    fn replacement_source_load_plan_begins_open_and_clears_competing_state_in_order() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                false,
                false,
                false,
            )),
            RustImageDocumentSourceLoadPlan {
                operations: vec![
                    RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                    RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                    RustImageDocumentSourceLoadOperation::CancelPredecode,
                    RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                    RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                    RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                    RustImageDocumentSourceLoadOperation::BeginOpen,
                ],
            }
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                false,
                true,
                false,
                true,
            )),
            RustImageDocumentSourceLoadPlan {
                operations: vec![
                    RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                    RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                    RustImageDocumentSourceLoadOperation::CancelPredecode,
                    RustImageDocumentSourceLoadOperation::FinishSpreadTransition,
                    RustImageDocumentSourceLoadOperation::ClearSecondaryPage,
                    RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                    RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                    RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                    RustImageDocumentSourceLoadOperation::BeginOpen,
                ],
            }
        );
    }

    #[test]
    fn right_to_left_reading_reset_is_decided_from_source_load_context() {
        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::CurrentSource,
                true,
                true,
                false,
                false,
            ))
            .operations,
            vec![
                RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                RustImageDocumentSourceLoadOperation::ResetRightToLeftReading,
                RustImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged,
                RustImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl,
            ]
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                true,
                false,
                false,
            ))
            .operations,
            vec![
                RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                RustImageDocumentSourceLoadOperation::CancelPredecode,
                RustImageDocumentSourceLoadOperation::ResetRightToLeftReading,
                RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                RustImageDocumentSourceLoadOperation::BeginOpen,
                RustImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged,
            ]
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                true,
                true,
                false,
            ))
            .operations,
            vec![
                RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                RustImageDocumentSourceLoadOperation::CancelPredecode,
                RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                RustImageDocumentSourceLoadOperation::BeginOpen,
            ]
        );

        assert_eq!(
            rust_image_document_source_load_plan(source_load_policy_input(
                RustImageDocumentSourceLoadKind::ReplacementSource,
                true,
                true,
                false,
                true,
            ))
            .operations,
            vec![
                RustImageDocumentSourceLoadOperation::CancelFileDeletion,
                RustImageDocumentSourceLoadOperation::CancelAllNavigation,
                RustImageDocumentSourceLoadOperation::CancelPredecode,
                RustImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested,
                RustImageDocumentSourceLoadOperation::PrepareSourceLoad,
                RustImageDocumentSourceLoadOperation::SetSourceUrlToRequested,
                RustImageDocumentSourceLoadOperation::BeginOpen,
            ]
        );
    }

    fn source_load_policy_input(
        load_kind: RustImageDocumentSourceLoadKind,
        preserve_two_page_spread_transition: bool,
        right_to_left_reading_enabled: bool,
        source_within_displayed_comic_book_archive: bool,
        has_requested_container_navigation_url: bool,
    ) -> RustImageDocumentSourceLoadPolicyInput {
        RustImageDocumentSourceLoadPolicyInput {
            load_kind,
            preserve_two_page_spread_transition,
            right_to_left_reading_enabled,
            source_within_displayed_comic_book_archive,
            has_requested_container_navigation_url,
        }
    }
}
