// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "bridge/imageopenworkflowconversion.h"
#include "imageopentransitionapplier.h"
#include "location/imagedocumentlocation.h"

namespace {
KiriView::Bridge::ImageDocumentSourceLoadKind sourceLoadKind(
    const KiriView::ImageDocumentSourceLoadSnapshot &snapshot,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    if (snapshot.currentSourceUrl == request.sourceUrl) {
        return KiriView::Bridge::ImageDocumentSourceLoadKind::CurrentSource;
    }

    return KiriView::Bridge::ImageDocumentSourceLoadKind::ReplacementSource;
}

bool sourceWithinDisplayedComicBookArchive(
    const KiriView::ImageDocumentSourceLoadSnapshot &snapshot,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    return snapshot.displayedOpenedCollectionScope.isComicBook()
        && KiriView::openedCollectionScopeContainsUrl(
            snapshot.displayedOpenedCollectionScope, request.sourceUrl);
}

KiriView::Bridge::ImageDocumentSourceLoadPolicyInput sourceLoadPolicyInput(
    const KiriView::ImageDocumentSourceLoadSnapshot &snapshot,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    return KiriView::Bridge::ImageDocumentSourceLoadPolicyInput {
        sourceLoadKind(snapshot, request),
        request.preserveTwoPageSpreadTransition,
        snapshot.rightToLeftReadingEnabled,
        sourceWithinDisplayedComicBookArchive(snapshot, request),
        !request.containerNavigationUrl.isEmpty(),
    };
}

void appendSourceLoadRuntimeOperation(KiriView::ImageDocumentRuntimePlan &runtimePlan,
    KiriView::ImageDocumentSourceLoadEffect effect,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    using Effect = KiriView::ImageDocumentSourceLoadEffect;

    switch (effect) {
    case Effect::CancelFileDeletion:
        runtimePlan.push_back(KiriView::CancelFileDeletionOperation {});
        return;
    case Effect::CancelAllNavigation:
        runtimePlan.push_back(KiriView::CancelAllNavigationOperation {});
        return;
    case Effect::CancelPredecode:
        runtimePlan.push_back(KiriView::CancelPredecodeOperation {});
        return;
    case Effect::FinishSpreadTransition:
        runtimePlan.push_back(KiriView::FinishSpreadTransitionOperation {});
        return;
    case Effect::ResetRightToLeftReading:
        runtimePlan.push_back(KiriView::ResetRightToLeftReadingOperation {});
        return;
    case Effect::NotifyRightToLeftReadingChanged:
        runtimePlan.push_back(KiriView::NotifyRightToLeftReadingChangedOperation {});
        return;
    case Effect::ClearSecondaryPage:
        runtimePlan.push_back(KiriView::ClearSecondaryPageOperation {});
        return;
    case Effect::ClearLoadingContainerNavigationUrl:
        runtimePlan.push_back(KiriView::ClearLoadingContainerNavigationUrlOperation {});
        return;
    case Effect::SetLoadingContainerNavigationUrlToRequested:
        runtimePlan.push_back(KiriView::SetLoadingContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case Effect::SetContainerNavigationUrlToRequested:
        runtimePlan.push_back(KiriView::SetContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case Effect::PrepareSourceLoad:
        runtimePlan.push_back(KiriView::PrepareSourceLoadOperation { request });
        return;
    case Effect::SetSourceUrlToRequested:
        runtimePlan.push_back(KiriView::SetSourceUrlOperation {
            KiriView::ImageDocumentPageTarget { request.sourceUrl, request.sourceKind },
        });
        return;
    case Effect::BeginOpen:
        runtimePlan.push_back(KiriView::BeginOpenOperation {});
        return;
    }
}

KiriView::ImageDocumentRuntimePlan sourceLoadRuntimePlan(
    const KiriView::ImageDocumentSourceLoadPlan &sourceLoadPlan,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    KiriView::ImageDocumentRuntimePlan runtimePlan;
    runtimePlan.reserve(sourceLoadPlan.size());
    for (KiriView::ImageDocumentSourceLoadEffect effect : sourceLoadPlan) {
        appendSourceLoadRuntimeOperation(runtimePlan, effect, request);
    }
    return runtimePlan;
}
}

namespace KiriView::ImageOpenWorkflow {
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
