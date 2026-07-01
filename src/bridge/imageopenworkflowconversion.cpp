// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflowconversion.h"

namespace {
kiriview::RustImageDocumentSourceLoadKind rustSourceLoadKind(
    kiriview::Bridge::ImageDocumentSourceLoadKind loadKind)
{
    switch (loadKind) {
    case kiriview::Bridge::ImageDocumentSourceLoadKind::CurrentSource:
        return kiriview::RustImageDocumentSourceLoadKind::CurrentSource;
    case kiriview::Bridge::ImageDocumentSourceLoadKind::SameScopeImageNavigation:
        return kiriview::RustImageDocumentSourceLoadKind::SameScopeImageNavigation;
    case kiriview::Bridge::ImageDocumentSourceLoadKind::ReplacementSource:
        return kiriview::RustImageDocumentSourceLoadKind::ReplacementSource;
    }

    return kiriview::RustImageDocumentSourceLoadKind::CurrentSource;
}

kiriview::ImageDocumentSourceLoadEffect sourceLoadEffectFromBridge(
    kiriview::RustImageDocumentSourceLoadOperation operation)
{
    using RustOperation = kiriview::RustImageDocumentSourceLoadOperation;

    switch (operation) {
    case RustOperation::CancelFileDeletion:
        return kiriview::ImageDocumentSourceLoadEffect::CancelFileDeletion;
    case RustOperation::CancelAllNavigation:
        return kiriview::ImageDocumentSourceLoadEffect::CancelAllNavigation;
    case RustOperation::CancelPredecode:
        return kiriview::ImageDocumentSourceLoadEffect::CancelPredecode;
    case RustOperation::FinishSpreadTransition:
        return kiriview::ImageDocumentSourceLoadEffect::FinishSpreadTransition;
    case RustOperation::ResetRightToLeftReading:
        return kiriview::ImageDocumentSourceLoadEffect::ResetRightToLeftReading;
    case RustOperation::NotifyRightToLeftReadingChanged:
        return kiriview::ImageDocumentSourceLoadEffect::NotifyRightToLeftReadingChanged;
    case RustOperation::ClearSecondaryPage:
        return kiriview::ImageDocumentSourceLoadEffect::ClearSecondaryPage;
    case RustOperation::ClearLoadingContainerNavigationUrl:
        return kiriview::ImageDocumentSourceLoadEffect::ClearLoadingContainerNavigationUrl;
    case RustOperation::SetLoadingContainerNavigationUrlToRequested:
        return kiriview::ImageDocumentSourceLoadEffect::SetLoadingContainerNavigationUrlToRequested;
    case RustOperation::SetContainerNavigationUrlToRequested:
        return kiriview::ImageDocumentSourceLoadEffect::SetContainerNavigationUrlToRequested;
    case RustOperation::PrepareSourceLoad:
        return kiriview::ImageDocumentSourceLoadEffect::PrepareSourceLoad;
    case RustOperation::SetSourceUrlToRequested:
        return kiriview::ImageDocumentSourceLoadEffect::SetSourceUrlToRequested;
    case RustOperation::BeginOpen:
        return kiriview::ImageDocumentSourceLoadEffect::BeginOpen;
    }

    return kiriview::ImageDocumentSourceLoadEffect::CancelFileDeletion;
}

kiriview::ImageOpenBoolTarget imageOpenBoolTarget(kiriview::RustImageOpenBoolTarget target)
{
    switch (target) {
    case kiriview::RustImageOpenBoolTarget::False:
        return kiriview::ImageOpenBoolTarget::False;
    case kiriview::RustImageOpenBoolTarget::True:
        return kiriview::ImageOpenBoolTarget::True;
    case kiriview::RustImageOpenBoolTarget::Unchanged:
        return kiriview::ImageOpenBoolTarget::Unchanged;
    }

    return kiriview::ImageOpenBoolTarget::Unchanged;
}

kiriview::ImageOpenStatusTarget imageOpenStatusTarget(kiriview::RustImageOpenStatusTarget target)
{
    switch (target) {
    case kiriview::RustImageOpenStatusTarget::Null:
        return kiriview::ImageOpenStatusTarget::Null;
    case kiriview::RustImageOpenStatusTarget::Loading:
        return kiriview::ImageOpenStatusTarget::Loading;
    case kiriview::RustImageOpenStatusTarget::Ready:
        return kiriview::ImageOpenStatusTarget::Ready;
    case kiriview::RustImageOpenStatusTarget::Error:
        return kiriview::ImageOpenStatusTarget::Error;
    case kiriview::RustImageOpenStatusTarget::Unchanged:
        return kiriview::ImageOpenStatusTarget::Unchanged;
    }

    return kiriview::ImageOpenStatusTarget::Unchanged;
}

kiriview::ImageOpenErrorStringTarget imageOpenErrorStringTarget(
    kiriview::RustImageOpenErrorStringTarget target)
{
    switch (target) {
    case kiriview::RustImageOpenErrorStringTarget::Clear:
        return kiriview::ImageOpenErrorStringTarget::Clear;
    case kiriview::RustImageOpenErrorStringTarget::Provided:
        return kiriview::ImageOpenErrorStringTarget::Provided;
    case kiriview::RustImageOpenErrorStringTarget::Unchanged:
        return kiriview::ImageOpenErrorStringTarget::Unchanged;
    }

    return kiriview::ImageOpenErrorStringTarget::Unchanged;
}

kiriview::ImageOpenEmbeddedMetadataTarget imageOpenEmbeddedMetadataTarget(bool clear)
{
    return clear ? kiriview::ImageOpenEmbeddedMetadataTarget::Clear
                 : kiriview::ImageOpenEmbeddedMetadataTarget::Unchanged;
}

kiriview::ImageOpenUrlTarget imageOpenUrlTarget(kiriview::RustImageOpenUrlTarget target)
{
    switch (target) {
    case kiriview::RustImageOpenUrlTarget::Empty:
        return kiriview::ImageOpenUrlTarget::Empty;
    case kiriview::RustImageOpenUrlTarget::SessionImage:
        return kiriview::ImageOpenUrlTarget::SessionImage;
    case kiriview::RustImageOpenUrlTarget::SessionContainerNavigation:
        return kiriview::ImageOpenUrlTarget::SessionContainerNavigation;
    case kiriview::RustImageOpenUrlTarget::DerivedContainerNavigation:
        return kiriview::ImageOpenUrlTarget::DerivedContainerNavigation;
    case kiriview::RustImageOpenUrlTarget::Container:
        return kiriview::ImageOpenUrlTarget::Container;
    case kiriview::RustImageOpenUrlTarget::Displayed:
        return kiriview::ImageOpenUrlTarget::Displayed;
    case kiriview::RustImageOpenUrlTarget::Unchanged:
        return kiriview::ImageOpenUrlTarget::Unchanged;
    }

    return kiriview::ImageOpenUrlTarget::Unchanged;
}

kiriview::ImageOpenSourceKindTarget imageOpenSourceKindTarget(
    kiriview::RustImageOpenSourceKindTarget target)
{
    switch (target) {
    case kiriview::RustImageOpenSourceKindTarget::Session:
        return kiriview::ImageOpenSourceKindTarget::Session;
    case kiriview::RustImageOpenSourceKindTarget::Unchanged:
        return kiriview::ImageOpenSourceKindTarget::Unchanged;
    }

    return kiriview::ImageOpenSourceKindTarget::Unchanged;
}

kiriview::ImageOpenDisplayedLocationTarget imageOpenDisplayedLocationTarget(
    kiriview::RustImageOpenDisplayedLocationTarget target)
{
    switch (target) {
    case kiriview::RustImageOpenDisplayedLocationTarget::Session:
        return kiriview::ImageOpenDisplayedLocationTarget::Session;
    case kiriview::RustImageOpenDisplayedLocationTarget::Unchanged:
        return kiriview::ImageOpenDisplayedLocationTarget::Unchanged;
    }

    return kiriview::ImageOpenDisplayedLocationTarget::Unchanged;
}

kiriview::ImageOpenEffect imageOpenEffect(kiriview::RustImageOpenEffect effect)
{
    switch (effect) {
    case kiriview::RustImageOpenEffect::ClearImage:
        return kiriview::ImageOpenEffect::ClearImage;
    case kiriview::RustImageOpenEffect::ResetZoom:
        return kiriview::ImageOpenEffect::ResetZoom;
    case kiriview::RustImageOpenEffect::UpdatePageNavigation:
        return kiriview::ImageOpenEffect::UpdatePageNavigation;
    case kiriview::RustImageOpenEffect::ScheduleAdjacentImagePredecode:
        return kiriview::ImageOpenEffect::ScheduleAdjacentImagePredecode;
    case kiriview::RustImageOpenEffect::PrepareFailedContainer:
        return kiriview::ImageOpenEffect::PrepareFailedContainer;
    case kiriview::RustImageOpenEffect::ClearLoadingPresentation:
        return kiriview::ImageOpenEffect::ClearLoadingPresentation;
    case kiriview::RustImageOpenEffect::FinishSpreadTransition:
        return kiriview::ImageOpenEffect::FinishSpreadTransition;
    case kiriview::RustImageOpenEffect::ClearSecondaryPage:
        return kiriview::ImageOpenEffect::ClearSecondaryPage;
    }

    return kiriview::ImageOpenEffect::ClearImage;
}
}

namespace kiriview {
RustImageDocumentSourceLoadPolicyInput rustImageDocumentSourceLoadPolicyInput(
    Bridge::ImageDocumentSourceLoadPolicyInput input)
{
    return RustImageDocumentSourceLoadPolicyInput {
        rustSourceLoadKind(input.loadKind),
        input.preserveTwoPageSpreadTransition,
        input.rightToLeftReadingEnabled,
        input.sourceWithinDisplayedComicBookArchive,
        input.hasRequestedContainerNavigationUrl,
    };
}

ImageDocumentSourceLoadPlan imageDocumentSourceLoadPlanFromBridge(
    const RustImageDocumentSourceLoadPlan& rustPlan)
{
    ImageDocumentSourceLoadPlan plan;
    plan.reserve(rustPlan.operations.size());
    for (RustImageDocumentSourceLoadOperation operation : rustPlan.operations) {
        plan.push_back(sourceLoadEffectFromBridge(operation));
    }
    return plan;
}

RustImageOpenWorkflowEvent rustImageOpenWorkflowEvent(RustImageOpenWorkflowEventKind kind)
{
    RustImageOpenWorkflowEvent event {};
    event.kind = kind;
    return event;
}

RustImageOpenWorkflowEvent rustBeginSourceLoadEvent(ImageOpenBeginSourceLoadSnapshot snapshot)
{
    RustImageOpenWorkflowEvent event
        = rustImageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::BeginSourceLoad);
    event.begin_source_load.has_image = snapshot.hasImage;
    event.begin_source_load.has_loading_container_navigation_target
        = snapshot.hasLoadingContainerNavigationTarget;
    return event;
}

RustImageOpenWorkflowEvent rustSuccessfulImageLoadEvent(
    ImageOpenSuccessfulImageLoadSnapshot snapshot)
{
    RustImageOpenWorkflowEvent event
        = rustImageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::FinishSuccessfulImageLoad);
    event.successful_image_load.has_request_container_navigation_target
        = snapshot.hasRequestContainerNavigationTarget;
    return event;
}

RustImageOpenWorkflowEvent rustSourceLoadErrorEvent(ImageOpenLoadErrorSnapshot snapshot)
{
    RustImageOpenWorkflowEvent event
        = rustImageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::FinishSourceLoadWithError);
    event.source_load_error.has_container_navigation_target = snapshot.hasContainerNavigationTarget;
    event.source_load_error.has_image = snapshot.hasImage;
    event.source_load_error.has_displayed_url = snapshot.hasDisplayedUrl;
    return event;
}

ImageOpenTransition imageOpenTransitionFromBridge(const RustImageOpenTransition& rustTransition)
{
    ImageOpenTransition transition;
    transition.stateDelta = ImageOpenStateDelta {
        imageOpenUrlTarget(rustTransition.state_delta.source_url),
        imageOpenSourceKindTarget(rustTransition.state_delta.source_kind),
        imageOpenDisplayedLocationTarget(rustTransition.state_delta.displayed_location),
        imageOpenUrlTarget(rustTransition.state_delta.container_navigation_url),
        imageOpenBoolTarget(rustTransition.state_delta.loading),
        imageOpenStatusTarget(rustTransition.state_delta.status),
        imageOpenErrorStringTarget(rustTransition.state_delta.error_string),
        imageOpenBoolTarget(rustTransition.state_delta.unsupported_opened_collection_video),
        imageOpenEmbeddedMetadataTarget(rustTransition.state_delta.clear_embedded_metadata),
        rustTransition.state_delta.clear_loading_container_navigation_url,
    };

    for (RustImageOpenEffect effect : rustTransition.effects) {
        transition.effects.push_back(imageOpenEffect(effect));
    }
    return transition;
}
}
