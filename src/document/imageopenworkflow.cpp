// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "kiriview/src/policy/imageopenworkflow.cxx.h"
#include "navigation/imagecontainer.h"

namespace {
KiriView::ImageDocumentSourceLoadKind sourceLoadKind(
    const KiriView::ImageDocumentSourceLoadSnapshot &snapshot,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    if (snapshot.currentSourceUrl == request.sourceUrl) {
        return KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    }

    return KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
}

bool sourceWithinDisplayedComicBookArchive(
    const KiriView::ImageDocumentSourceLoadSnapshot &snapshot,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    return snapshot.displayedArchiveDocument.isComicBook()
        && KiriView::archiveDocumentContainsUrl(
            snapshot.displayedArchiveDocument, request.sourceUrl);
}

KiriView::RustImageDocumentSourceLoadKind rustSourceLoadKind(
    KiriView::ImageDocumentSourceLoadKind loadKind)
{
    switch (loadKind) {
    case KiriView::ImageDocumentSourceLoadKind::CurrentSource:
        return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
    case KiriView::ImageDocumentSourceLoadKind::ReplacementSource:
        return KiriView::RustImageDocumentSourceLoadKind::ReplacementSource;
    }

    return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
}

void appendSourceLoadRuntimeOperation(KiriView::ImageDocumentRuntimePlan &runtimePlan,
    KiriView::RustImageDocumentSourceLoadOperation operation,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    using RustOperation = KiriView::RustImageDocumentSourceLoadOperation;

    switch (operation) {
    case RustOperation::CancelFileDeletion:
        runtimePlan.push_back(KiriView::CancelFileDeletionOperation {});
        return;
    case RustOperation::CancelAllNavigation:
        runtimePlan.push_back(KiriView::CancelAllNavigationOperation {});
        return;
    case RustOperation::CancelPredecode:
        runtimePlan.push_back(KiriView::CancelPredecodeOperation {});
        return;
    case RustOperation::FinishSpreadTransition:
        runtimePlan.push_back(KiriView::FinishSpreadTransitionOperation {});
        return;
    case RustOperation::ResetRightToLeftReading:
        runtimePlan.push_back(KiriView::ResetRightToLeftReadingOperation {});
        return;
    case RustOperation::NotifyRightToLeftReadingChanged:
        runtimePlan.push_back(KiriView::NotifyRightToLeftReadingChangedOperation {});
        return;
    case RustOperation::ClearSecondaryPage:
        runtimePlan.push_back(KiriView::ClearSecondaryPageOperation {});
        return;
    case RustOperation::ClearLoadingContainerNavigationUrl:
        runtimePlan.push_back(KiriView::ClearLoadingContainerNavigationUrlOperation {});
        return;
    case RustOperation::SetLoadingContainerNavigationUrlToRequested:
        runtimePlan.push_back(KiriView::SetLoadingContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case RustOperation::SetContainerNavigationUrlToRequested:
        runtimePlan.push_back(KiriView::SetContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case RustOperation::PrepareSourceLoad:
        runtimePlan.push_back(KiriView::PrepareSourceLoadOperation { request });
        return;
    case RustOperation::SetSourceUrlToRequested:
        runtimePlan.push_back(KiriView::SetSourceUrlOperation { request.sourceUrl });
        return;
    case RustOperation::BeginOpen:
        runtimePlan.push_back(KiriView::BeginOpenOperation {});
        return;
    }
}

KiriView::RustImageDocumentSourceLoadPolicyInput rustSourceLoadPolicyInput(
    const KiriView::ImageDocumentSourceLoadPolicyInput &input)
{
    return KiriView::RustImageDocumentSourceLoadPolicyInput {
        rustSourceLoadKind(input.loadKind),
        input.preserveTwoPageSpreadTransition,
        input.rightToLeftReadingEnabled,
        input.sourceWithinDisplayedComicBookArchive,
        input.hasRequestedContainerNavigationUrl,
    };
}

KiriView::ImageDocumentRuntimePlan runtimePlanFromRust(
    const KiriView::RustImageDocumentSourceLoadPlan &rustPlan,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    KiriView::ImageDocumentRuntimePlan plan;
    plan.reserve(rustPlan.operations.size());
    for (KiriView::RustImageDocumentSourceLoadOperation operation : rustPlan.operations) {
        appendSourceLoadRuntimeOperation(plan, operation, request);
    }
    return plan;
}

KiriView::RustImageOpenWorkflowEvent imageOpenWorkflowEvent(
    KiriView::RustImageOpenWorkflowEventKind kind)
{
    KiriView::RustImageOpenWorkflowEvent event {};
    event.kind = kind;
    return event;
}

KiriView::RustImageOpenWorkflowEvent beginSourceLoadEvent(
    KiriView::ImageOpenBeginSourceLoadSnapshot snapshot)
{
    KiriView::RustImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::RustImageOpenWorkflowEventKind::BeginSourceLoad);
    event.begin_source_load.has_image = snapshot.hasImage;
    event.begin_source_load.has_loading_container_navigation_target
        = snapshot.hasLoadingContainerNavigationTarget;
    return event;
}

KiriView::RustImageOpenWorkflowEvent successfulImageLoadEvent(
    KiriView::ImageOpenSuccessfulImageLoadSnapshot snapshot)
{
    KiriView::RustImageOpenWorkflowEvent event = imageOpenWorkflowEvent(
        KiriView::RustImageOpenWorkflowEventKind::FinishSuccessfulImageLoad);
    event.successful_image_load.has_request_container_navigation_target
        = snapshot.hasRequestContainerNavigationTarget;
    return event;
}

KiriView::RustImageOpenWorkflowEvent sourceLoadErrorEvent(
    KiriView::ImageOpenLoadErrorSnapshot snapshot)
{
    KiriView::RustImageOpenWorkflowEvent event = imageOpenWorkflowEvent(
        KiriView::RustImageOpenWorkflowEventKind::FinishSourceLoadWithError);
    event.source_load_error.has_container_navigation_target = snapshot.hasContainerNavigationTarget;
    event.source_load_error.has_image = snapshot.hasImage;
    event.source_load_error.has_displayed_url = snapshot.hasDisplayedUrl;
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
ImageDocumentSourceLoadPolicyInput sourceLoadPolicyInput(
    const ImageDocumentSourceLoadSnapshot &snapshot, const ImageDocumentSourceLoadRequest &request)
{
    return ImageDocumentSourceLoadPolicyInput {
        sourceLoadKind(snapshot, request),
        request.preserveTwoPageSpreadTransition,
        snapshot.rightToLeftReadingEnabled,
        sourceWithinDisplayedComicBookArchive(snapshot, request),
        !request.containerNavigationUrl.isEmpty(),
    };
}

ImageDocumentRuntimePlan sourceLoadPlan(
    const ImageDocumentSourceLoadPolicyInput &input, const ImageDocumentSourceLoadRequest &request)
{
    return runtimePlanFromRust(
        rustImageDocumentSourceLoadPlan(rustSourceLoadPolicyInput(input)), request);
}

ImageOpenTransition beginSourceLoadTransition(ImageOpenBeginSourceLoadSnapshot snapshot)
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(beginSourceLoadEvent(snapshot)));
}

ImageOpenTransition finishEmptySourceLoadTransition()
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(
        imageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::FinishEmptySourceLoad)));
}

ImageOpenTransition resolveSourceImageTransition()
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(
        imageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::ResolveSourceImage)));
}

ImageOpenTransition finishSuccessfulImageLoadTransition(
    ImageOpenSuccessfulImageLoadSnapshot snapshot)
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(successfulImageLoadEvent(snapshot)));
}

ImageOpenTransition finishLoadWithErrorTransition(ImageOpenLoadErrorSnapshot snapshot)
{
    return imageOpenTransitionFromRust(rustImageOpenTransition(sourceLoadErrorEvent(snapshot)));
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
