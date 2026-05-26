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
    return snapshot.displayedImagePageScope.isComicBook()
        && KiriView::imagePageScopeContainsUrl(snapshot.displayedImagePageScope, request.sourceUrl);
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
}

namespace KiriView::ImageOpenWorkflow {
ImageDocumentRuntimePlan sourceLoadPlan(
    const ImageDocumentSourceLoadSnapshot &snapshot, const ImageDocumentSourceLoadRequest &request)
{
    const Bridge::ImageDocumentSourceLoadPolicyInput input
        = sourceLoadPolicyInput(snapshot, request);
    return imageDocumentRuntimePlanFromBridge(
        rustImageDocumentSourceLoadPlan(rustImageDocumentSourceLoadPolicyInput(input)), request);
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
