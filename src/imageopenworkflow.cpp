// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imageopentransitionapplier.h"
#include "kiriview/src/imageopenworkflow.cxx.h"

namespace {
KiriView::RustImageOpenWorkflowEvent imageOpenWorkflowEvent(
    KiriView::RustImageOpenWorkflowEventKind kind)
{
    KiriView::RustImageOpenWorkflowEvent event {};
    event.kind = kind;
    return event;
}

KiriView::RustImageOpenWorkflowEvent beginSourceLoadEvent(
    bool hasImage, bool hasLoadingContainerNavigationTarget)
{
    KiriView::RustImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::RustImageOpenWorkflowEventKind::BeginSourceLoad);
    event.begin_source_load.has_image = hasImage;
    event.begin_source_load.has_loading_container_navigation_target
        = hasLoadingContainerNavigationTarget;
    return event;
}

KiriView::RustImageOpenWorkflowEvent successfulImageLoadEvent(
    const KiriView::ImageLoadSession &session)
{
    KiriView::RustImageOpenWorkflowEvent event = imageOpenWorkflowEvent(
        KiriView::RustImageOpenWorkflowEventKind::FinishSuccessfulImageLoad);
    event.successful_image_load.has_request_container_navigation_target
        = session.hasContainerNavigationTarget();
    return event;
}

KiriView::RustImageOpenWorkflowEvent sourceLoadErrorEvent(
    bool hasContainerNavigationTarget, bool hasImage, bool hasDisplayedUrl)
{
    KiriView::RustImageOpenWorkflowEvent event = imageOpenWorkflowEvent(
        KiriView::RustImageOpenWorkflowEventKind::FinishSourceLoadWithError);
    event.source_load_error.has_container_navigation_target = hasContainerNavigationTarget;
    event.source_load_error.has_image = hasImage;
    event.source_load_error.has_displayed_url = hasDisplayedUrl;
    return event;
}

KiriView::ImageOpenBoolTarget imageOpenBoolTarget(KiriView::RustImageOpenBoolTarget target)
{
    switch (target) {
    case KiriView::RustImageOpenBoolTarget::False:
        return KiriView::ImageOpenBoolTarget::False;
    case KiriView::RustImageOpenBoolTarget::True:
        return KiriView::ImageOpenBoolTarget::True;
    case KiriView::RustImageOpenBoolTarget::Unchanged:
        return KiriView::ImageOpenBoolTarget::Unchanged;
    }

    return KiriView::ImageOpenBoolTarget::Unchanged;
}

KiriView::ImageOpenStatusTarget imageOpenStatusTarget(KiriView::RustImageOpenStatusTarget target)
{
    switch (target) {
    case KiriView::RustImageOpenStatusTarget::Null:
        return KiriView::ImageOpenStatusTarget::Null;
    case KiriView::RustImageOpenStatusTarget::Loading:
        return KiriView::ImageOpenStatusTarget::Loading;
    case KiriView::RustImageOpenStatusTarget::Ready:
        return KiriView::ImageOpenStatusTarget::Ready;
    case KiriView::RustImageOpenStatusTarget::Error:
        return KiriView::ImageOpenStatusTarget::Error;
    case KiriView::RustImageOpenStatusTarget::Unchanged:
        return KiriView::ImageOpenStatusTarget::Unchanged;
    }

    return KiriView::ImageOpenStatusTarget::Unchanged;
}

KiriView::ImageOpenErrorStringTarget imageOpenErrorStringTarget(
    KiriView::RustImageOpenErrorStringTarget target)
{
    switch (target) {
    case KiriView::RustImageOpenErrorStringTarget::Clear:
        return KiriView::ImageOpenErrorStringTarget::Clear;
    case KiriView::RustImageOpenErrorStringTarget::Provided:
        return KiriView::ImageOpenErrorStringTarget::Provided;
    case KiriView::RustImageOpenErrorStringTarget::Unchanged:
        return KiriView::ImageOpenErrorStringTarget::Unchanged;
    }

    return KiriView::ImageOpenErrorStringTarget::Unchanged;
}

KiriView::ImageOpenUrlTarget imageOpenUrlTarget(KiriView::RustImageOpenUrlTarget target)
{
    switch (target) {
    case KiriView::RustImageOpenUrlTarget::Empty:
        return KiriView::ImageOpenUrlTarget::Empty;
    case KiriView::RustImageOpenUrlTarget::SessionImage:
        return KiriView::ImageOpenUrlTarget::SessionImage;
    case KiriView::RustImageOpenUrlTarget::SessionContainerNavigation:
        return KiriView::ImageOpenUrlTarget::SessionContainerNavigation;
    case KiriView::RustImageOpenUrlTarget::DerivedContainerNavigation:
        return KiriView::ImageOpenUrlTarget::DerivedContainerNavigation;
    case KiriView::RustImageOpenUrlTarget::Container:
        return KiriView::ImageOpenUrlTarget::Container;
    case KiriView::RustImageOpenUrlTarget::Displayed:
        return KiriView::ImageOpenUrlTarget::Displayed;
    case KiriView::RustImageOpenUrlTarget::Unchanged:
        return KiriView::ImageOpenUrlTarget::Unchanged;
    }

    return KiriView::ImageOpenUrlTarget::Unchanged;
}

KiriView::ImageOpenDisplayedLocationTarget imageOpenDisplayedLocationTarget(
    KiriView::RustImageOpenDisplayedLocationTarget target)
{
    switch (target) {
    case KiriView::RustImageOpenDisplayedLocationTarget::Session:
        return KiriView::ImageOpenDisplayedLocationTarget::Session;
    case KiriView::RustImageOpenDisplayedLocationTarget::Unchanged:
        return KiriView::ImageOpenDisplayedLocationTarget::Unchanged;
    }

    return KiriView::ImageOpenDisplayedLocationTarget::Unchanged;
}

KiriView::ImageOpenEffect imageOpenEffect(KiriView::RustImageOpenEffect effect)
{
    switch (effect) {
    case KiriView::RustImageOpenEffect::ClearImage:
        return KiriView::ImageOpenEffect::ClearImage;
    case KiriView::RustImageOpenEffect::ResetZoom:
        return KiriView::ImageOpenEffect::ResetZoom;
    case KiriView::RustImageOpenEffect::UpdatePageNavigation:
        return KiriView::ImageOpenEffect::UpdatePageNavigation;
    case KiriView::RustImageOpenEffect::ScheduleAdjacentImagePredecode:
        return KiriView::ImageOpenEffect::ScheduleAdjacentImagePredecode;
    case KiriView::RustImageOpenEffect::PrepareFailedContainer:
        return KiriView::ImageOpenEffect::PrepareFailedContainer;
    }

    return KiriView::ImageOpenEffect::ClearImage;
}

KiriView::ImageOpenTransition imageOpenTransitionFromRust(
    const KiriView::RustImageOpenTransition &rustTransition)
{
    KiriView::ImageOpenTransition transition;
    transition.stateDelta = KiriView::ImageOpenStateDelta {
        imageOpenUrlTarget(rustTransition.state_delta.source_url),
        imageOpenDisplayedLocationTarget(rustTransition.state_delta.displayed_location),
        imageOpenUrlTarget(rustTransition.state_delta.container_navigation_url),
        imageOpenBoolTarget(rustTransition.state_delta.loading),
        imageOpenStatusTarget(rustTransition.state_delta.status),
        imageOpenErrorStringTarget(rustTransition.state_delta.error_string),
        rustTransition.state_delta.clear_loading_container_navigation_url,
    };

    for (KiriView::RustImageOpenEffect effect : rustTransition.effects) {
        transition.effects.push_back(imageOpenEffect(effect));
    }
    return transition;
}
}

namespace KiriView::ImageOpenWorkflow {
ImageOpenTransition beginSourceLoadTransition(
    bool hasImage, bool hasLoadingContainerNavigationTarget)
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(
        beginSourceLoadEvent(hasImage, hasLoadingContainerNavigationTarget)));
}

ImageOpenTransition finishEmptySourceLoadTransition()
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(
        imageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::FinishEmptySourceLoad)));
}

ImageOpenTransition finishSuccessfulImageLoadTransition(const ImageLoadSession &session)
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(successfulImageLoadEvent(session)));
}

ImageOpenTransition finishLoadWithErrorTransition(
    const ImageLoadSession &session, bool hasImage, bool hasDisplayedUrl)
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(
        sourceLoadErrorEvent(session.hasContainerNavigationTarget(), hasImage, hasDisplayedUrl)));
}

ImageOpenTransition finishContainerNavigationLoadWithErrorTransition()
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(imageOpenWorkflowEvent(
        RustImageOpenWorkflowEventKind::FinishContainerNavigationLoadWithError)));
}

ImageOpenTransition finishAnimationLoadWithErrorTransition()
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(
        imageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::FinishAnimationLoadWithError)));
}
}
