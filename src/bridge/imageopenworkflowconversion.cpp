// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflowconversion.h"

namespace {
KiriView::RustImageDocumentSourceLoadKind rustSourceLoadKind(
    KiriView::Bridge::ImageDocumentSourceLoadKind loadKind)
{
    switch (loadKind) {
    case KiriView::Bridge::ImageDocumentSourceLoadKind::CurrentSource:
        return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
    case KiriView::Bridge::ImageDocumentSourceLoadKind::ReplacementSource:
        return KiriView::RustImageDocumentSourceLoadKind::ReplacementSource;
    }

    return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
}

KiriView::ImageDocumentSourceLoadEffect sourceLoadEffectFromBridge(
    KiriView::RustImageDocumentSourceLoadOperation operation)
{
    using RustOperation = KiriView::RustImageDocumentSourceLoadOperation;

    switch (operation) {
    case RustOperation::CancelFileDeletion:
        return KiriView::ImageDocumentSourceLoadEffect::CancelFileDeletion;
    case RustOperation::CancelAllNavigation:
        return KiriView::ImageDocumentSourceLoadEffect::CancelAllNavigation;
    case RustOperation::CancelPredecode:
        return KiriView::ImageDocumentSourceLoadEffect::CancelPredecode;
    case RustOperation::FinishSpreadTransition:
        return KiriView::ImageDocumentSourceLoadEffect::FinishSpreadTransition;
    case RustOperation::ResetRightToLeftReading:
        return KiriView::ImageDocumentSourceLoadEffect::ResetRightToLeftReading;
    case RustOperation::NotifyRightToLeftReadingChanged:
        return KiriView::ImageDocumentSourceLoadEffect::NotifyRightToLeftReadingChanged;
    case RustOperation::ClearSecondaryPage:
        return KiriView::ImageDocumentSourceLoadEffect::ClearSecondaryPage;
    case RustOperation::ClearLoadingContainerNavigationUrl:
        return KiriView::ImageDocumentSourceLoadEffect::ClearLoadingContainerNavigationUrl;
    case RustOperation::SetLoadingContainerNavigationUrlToRequested:
        return KiriView::ImageDocumentSourceLoadEffect::SetLoadingContainerNavigationUrlToRequested;
    case RustOperation::SetContainerNavigationUrlToRequested:
        return KiriView::ImageDocumentSourceLoadEffect::SetContainerNavigationUrlToRequested;
    case RustOperation::PrepareSourceLoad:
        return KiriView::ImageDocumentSourceLoadEffect::PrepareSourceLoad;
    case RustOperation::SetSourceUrlToRequested:
        return KiriView::ImageDocumentSourceLoadEffect::SetSourceUrlToRequested;
    case RustOperation::BeginOpen:
        return KiriView::ImageDocumentSourceLoadEffect::BeginOpen;
    }

    return KiriView::ImageDocumentSourceLoadEffect::CancelFileDeletion;
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

KiriView::ImageOpenSourceKindTarget imageOpenSourceKindTarget(
    KiriView::RustImageOpenSourceKindTarget target)
{
    switch (target) {
    case KiriView::RustImageOpenSourceKindTarget::Session:
        return KiriView::ImageOpenSourceKindTarget::Session;
    case KiriView::RustImageOpenSourceKindTarget::Unchanged:
        return KiriView::ImageOpenSourceKindTarget::Unchanged;
    }

    return KiriView::ImageOpenSourceKindTarget::Unchanged;
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
    case KiriView::RustImageOpenEffect::ClearLoadingPresentation:
        return KiriView::ImageOpenEffect::ClearLoadingPresentation;
    case KiriView::RustImageOpenEffect::FinishSpreadTransition:
        return KiriView::ImageOpenEffect::FinishSpreadTransition;
    case KiriView::RustImageOpenEffect::ClearSecondaryPage:
        return KiriView::ImageOpenEffect::ClearSecondaryPage;
    }

    return KiriView::ImageOpenEffect::ClearImage;
}
}

namespace KiriView {
RustImageDocumentSourceLoadPolicyInput rustImageDocumentSourceLoadPolicyInput(
    const Bridge::ImageDocumentSourceLoadPolicyInput &input)
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
    const RustImageDocumentSourceLoadPlan &rustPlan)
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

ImageOpenTransition imageOpenTransitionFromBridge(const RustImageOpenTransition &rustTransition)
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
        rustTransition.state_delta.clear_embedded_metadata,
        rustTransition.state_delta.clear_loading_container_navigation_url,
    };

    for (RustImageOpenEffect effect : rustTransition.effects) {
        transition.effects.push_back(imageOpenEffect(effect));
    }
    return transition;
}
}
