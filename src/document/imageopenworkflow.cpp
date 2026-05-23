// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "bridge/imageopenworkflowconversion.h"
#include "navigation/imagecontainer.h"

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
    return snapshot.displayedArchiveDocument.isComicBook()
        && KiriView::archiveDocumentContainsUrl(
            snapshot.displayedArchiveDocument, request.sourceUrl);
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
