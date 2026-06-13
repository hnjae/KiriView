// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "bridge/imageopenworkflowconversion.h"
#include "imageopentransitionapplier.h"
#include "location/imagedocumentlocation.h"

#include <utility>

namespace {
kiriview::Bridge::ImageDocumentSourceLoadKind sourceLoadKind(
    const kiriview::ImageDocumentSourceLoadSnapshot &snapshot,
    const kiriview::ImageDocumentSourceLoadRequest &request)
{
    if (snapshot.currentSourceUrl == request.sourceUrl) {
        return kiriview::Bridge::ImageDocumentSourceLoadKind::CurrentSource;
    }

    return kiriview::Bridge::ImageDocumentSourceLoadKind::ReplacementSource;
}

bool sourceWithinDisplayedComicBookArchive(
    const kiriview::ImageDocumentSourceLoadSnapshot &snapshot,
    const kiriview::ImageDocumentSourceLoadRequest &request)
{
    return snapshot.displayedOpenedCollectionScope.isComicBook()
        && kiriview::openedCollectionScopeContainsUrl(
            snapshot.displayedOpenedCollectionScope, request.sourceUrl);
}

kiriview::Bridge::ImageDocumentSourceLoadPolicyInput sourceLoadPolicyInput(
    const kiriview::ImageDocumentSourceLoadSnapshot &snapshot,
    const kiriview::ImageDocumentSourceLoadRequest &request)
{
    return kiriview::Bridge::ImageDocumentSourceLoadPolicyInput {
        sourceLoadKind(snapshot, request),
        request.preserveTwoPageSpreadTransition,
        snapshot.rightToLeftReadingEnabled,
        sourceWithinDisplayedComicBookArchive(snapshot, request),
        !request.containerNavigationUrl.isEmpty(),
    };
}

void appendSourceLoadRuntimeOperation(kiriview::ImageDocumentRuntimePlan &runtimePlan,
    kiriview::ImageDocumentSourceLoadEffect effect,
    const kiriview::ImageDocumentSourceLoadRequest &request)
{
    using Effect = kiriview::ImageDocumentSourceLoadEffect;

    switch (effect) {
    case Effect::CancelFileDeletion:
        runtimePlan.push_back(kiriview::CancelFileDeletionOperation {});
        return;
    case Effect::CancelAllNavigation:
        runtimePlan.push_back(kiriview::CancelAllNavigationOperation {});
        return;
    case Effect::CancelPredecode:
        runtimePlan.push_back(kiriview::CancelPredecodeOperation {});
        return;
    case Effect::FinishSpreadTransition:
        runtimePlan.push_back(kiriview::FinishSpreadTransitionOperation {});
        return;
    case Effect::ResetRightToLeftReading:
        runtimePlan.push_back(kiriview::ResetRightToLeftReadingOperation {});
        return;
    case Effect::NotifyRightToLeftReadingChanged:
        runtimePlan.push_back(kiriview::NotifyRightToLeftReadingChangedOperation {});
        return;
    case Effect::ClearSecondaryPage:
        runtimePlan.push_back(kiriview::ClearSecondaryPageOperation {});
        return;
    case Effect::ClearLoadingContainerNavigationUrl:
        runtimePlan.push_back(kiriview::ClearLoadingContainerNavigationUrlOperation {});
        return;
    case Effect::SetLoadingContainerNavigationUrlToRequested:
        runtimePlan.push_back(kiriview::SetLoadingContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case Effect::SetContainerNavigationUrlToRequested:
        runtimePlan.push_back(kiriview::SetContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case Effect::PrepareSourceLoad:
        runtimePlan.push_back(kiriview::PrepareSourceLoadOperation { request });
        return;
    case Effect::SetSourceUrlToRequested:
        runtimePlan.push_back(kiriview::SetSourceUrlOperation {
            kiriview::ImageDocumentPageTarget { request.sourceUrl, request.sourceKind },
        });
        return;
    case Effect::BeginOpen:
        runtimePlan.push_back(kiriview::BeginOpenOperation {});
        return;
    }
}

kiriview::ImageDocumentRuntimePlan sourceLoadRuntimePlan(
    const kiriview::ImageDocumentSourceLoadPlan &sourceLoadPlan,
    const kiriview::ImageDocumentSourceLoadRequest &request)
{
    kiriview::ImageDocumentRuntimePlan runtimePlan;
    runtimePlan.reserve(sourceLoadPlan.size());
    for (kiriview::ImageDocumentSourceLoadEffect effect : sourceLoadPlan) {
        appendSourceLoadRuntimeOperation(runtimePlan, effect, request);
    }
    return runtimePlan;
}
}

namespace kiriview::ImageOpenWorkflow {
ImageDocumentRuntimePlan sourceLoadPlan(
    const ImageDocumentSourceLoadSnapshot &snapshot, const ImageDocumentSourceLoadRequest &request)
{
    const Bridge::ImageDocumentSourceLoadPolicyInput input
        = sourceLoadPolicyInput(snapshot, request);
    const ImageDocumentSourceLoadPlan plan = imageDocumentSourceLoadPlanFromBridge(
        rustImageDocumentSourceLoadPlan(rustImageDocumentSourceLoadPolicyInput(input)));
    return sourceLoadRuntimePlan(plan, request);
}

ImageOpenApplicationPlan beginSourceLoadPlan(ImageOpenBeginSourceLoadSnapshot snapshot)
{
    return imageOpenApplicationPlan(beginSourceLoadTransition(snapshot));
}

ImageOpenApplicationPlan finishEmptySourceLoadPlan()
{
    return imageOpenApplicationPlan(finishEmptySourceLoadTransition());
}

ImageOpenApplicationPlan resolveSourceImagePlan(const ImageLoadSession &session)
{
    return imageOpenApplicationPlan(
        resolveSourceImageTransition(), ImageOpenTransitionContext::sourceResolved(session));
}

ImageOpenApplicationPlan finishUnsupportedOpenedCollectionVideoLoadPlan(
    const ImageLoadSession &session)
{
    return imageOpenApplicationPlan(finishUnsupportedOpenedCollectionVideoLoadTransition(),
        ImageOpenTransitionContext::successfulImageLoad(session));
}

ImageOpenApplicationPlan finishSuccessfulImageLoadPlan(
    ImageOpenSuccessfulImageLoadSnapshot snapshot, const ImageLoadSession &session)
{
    return imageOpenApplicationPlan(finishSuccessfulImageLoadTransition(snapshot),
        ImageOpenTransitionContext::successfulImageLoad(session));
}

ImageOpenApplicationPlan finishSuccessfulImageLoadPlan(
    ImageOpenSuccessfulImageLoadSnapshot snapshot, const ImageLoadSession &session,
    EmbeddedMetadata metadata)
{
    ImageOpenTransition transition = finishSuccessfulImageLoadTransition(snapshot);
    transition.stateDelta.embeddedMetadata = ImageOpenEmbeddedMetadataTarget::Provided;
    return imageOpenApplicationPlan(
        transition, ImageOpenTransitionContext::successfulImageLoad(session, std::move(metadata)));
}

ImageOpenApplicationPlan finishLoadWithErrorPlan(ImageOpenLoadErrorSnapshot snapshot,
    const ImageLoadSession &session, const QUrl &displayedUrl, const QString &errorString)
{
    return imageOpenApplicationPlan(finishLoadWithErrorTransition(snapshot),
        ImageOpenTransitionContext::sourceLoadError(session, displayedUrl, errorString));
}

ImageOpenApplicationPlan finishContainerNavigationLoadWithErrorPlan(
    const QUrl &containerUrl, const QString &errorString)
{
    return imageOpenApplicationPlan(finishContainerNavigationLoadWithErrorTransition(),
        ImageOpenTransitionContext::containerNavigationError(containerUrl, errorString));
}

ImageOpenApplicationPlan finishAnimationLoadWithErrorPlan(const QString &errorString)
{
    return imageOpenApplicationPlan(finishAnimationLoadWithErrorTransition(),
        ImageOpenTransitionContext::animationError(errorString));
}

ImageOpenTransition beginSourceLoadTransition(ImageOpenBeginSourceLoadSnapshot snapshot)
{
    return imageOpenTransitionFromBridge(
        rustImageOpenTransition(rustBeginSourceLoadEvent(snapshot)));
}

ImageOpenTransition finishEmptySourceLoadTransition()
{
    return imageOpenTransitionFromBridge(rustImageOpenTransition(
        rustImageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::FinishEmptySourceLoad)));
}

ImageOpenTransition resolveSourceImageTransition()
{
    return imageOpenTransitionFromBridge(rustImageOpenTransition(
        rustImageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::ResolveSourceImage)));
}

ImageOpenTransition finishUnsupportedOpenedCollectionVideoLoadTransition()
{
    return imageOpenTransitionFromBridge(rustImageOpenTransition(rustImageOpenWorkflowEvent(
        RustImageOpenWorkflowEventKind::FinishUnsupportedOpenedCollectionVideoLoad)));
}

ImageOpenTransition finishSuccessfulImageLoadTransition(
    ImageOpenSuccessfulImageLoadSnapshot snapshot)
{
    return imageOpenTransitionFromBridge(
        rustImageOpenTransition(rustSuccessfulImageLoadEvent(snapshot)));
}

ImageOpenTransition finishLoadWithErrorTransition(ImageOpenLoadErrorSnapshot snapshot)
{
    return imageOpenTransitionFromBridge(
        rustImageOpenTransition(rustSourceLoadErrorEvent(snapshot)));
}

ImageOpenTransition finishContainerNavigationLoadWithErrorTransition()
{
    return imageOpenTransitionFromBridge(rustImageOpenTransition(rustImageOpenWorkflowEvent(
        RustImageOpenWorkflowEventKind::FinishContainerNavigationLoadWithError)));
}

ImageOpenTransition finishAnimationLoadWithErrorTransition()
{
    return imageOpenTransitionFromBridge(rustImageOpenTransition(
        rustImageOpenWorkflowEvent(RustImageOpenWorkflowEventKind::FinishAnimationLoadWithError)));
}
}
