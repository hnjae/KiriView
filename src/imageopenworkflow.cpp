// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagedocumentstate.h"
#include "imageopentransitionapplier.h"
#include "kiriview/src/imageopenworkflow.cxx.h"

namespace {
KiriView::ImageOpenWorkflowEvent imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind kind)
{
    KiriView::ImageOpenWorkflowEvent event {};
    event.kind = kind;
    return event;
}

KiriView::ImageOpenWorkflowEvent beginSourceLoadEvent(
    bool hasImage, bool hasLoadingContainerNavigationTarget)
{
    KiriView::ImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind::BeginSourceLoad);
    event.begin_source_load.has_image = hasImage;
    event.begin_source_load.has_loading_container_navigation_target
        = hasLoadingContainerNavigationTarget;
    return event;
}

KiriView::ImageOpenWorkflowEvent successfulImageLoadEvent(const KiriView::ImageLoadSession &session)
{
    KiriView::ImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind::FinishSuccessfulImageLoad);
    event.successful_image_load.has_request_container_navigation_target
        = !session.request.containerNavigationUrl().isEmpty();
    return event;
}

KiriView::ImageOpenWorkflowEvent sourceLoadErrorEvent(
    bool hasContainerNavigationTarget, bool hasImage, bool hasDisplayedUrl)
{
    KiriView::ImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind::FinishSourceLoadWithError);
    event.source_load_error.has_container_navigation_target = hasContainerNavigationTarget;
    event.source_load_error.has_image = hasImage;
    event.source_load_error.has_displayed_url = hasDisplayedUrl;
    return event;
}
}

namespace KiriView::ImageOpenWorkflow {
ImageDocumentEffects beginSourceLoad(ImageDocumentState &state, bool hasImage)
{
    return applyImageOpenTransition(state,
        rustImageOpenTransition(
            beginSourceLoadEvent(hasImage, !state.loadingContainerNavigationUrl().isEmpty())));
}

ImageDocumentEffects finishEmptySourceLoad(ImageDocumentState &state)
{
    return applyImageOpenTransition(state,
        rustImageOpenTransition(
            imageOpenWorkflowEvent(ImageOpenWorkflowEventKind::FinishEmptySourceLoad)));
}

ImageDocumentEffects finishSuccessfulImageLoad(
    ImageDocumentState &state, const ImageLoadSession &session)
{
    return applyImageOpenTransition(state,
        rustImageOpenTransition(successfulImageLoadEvent(session)),
        ImageOpenTransitionContext::successfulImageLoad(session));
}

ImageDocumentEffects finishLoadWithError(ImageDocumentState &state, const ImageLoadSession &session,
    bool hasImage, const QString &errorString)
{
    const QUrl containerUrl = session.request.containerNavigationUrl();
    const QUrl displayedUrl = state.displayedUrl();
    return applyImageOpenTransition(state,
        rustImageOpenTransition(
            sourceLoadErrorEvent(!containerUrl.isEmpty(), hasImage, !displayedUrl.isEmpty())),
        ImageOpenTransitionContext::sourceLoadError(session, displayedUrl, errorString));
}

ImageDocumentEffects finishContainerNavigationLoadWithError(
    ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    return applyImageOpenTransition(state,
        rustImageOpenTransition(imageOpenWorkflowEvent(
            ImageOpenWorkflowEventKind::FinishContainerNavigationLoadWithError)),
        ImageOpenTransitionContext::containerNavigationError(containerUrl, errorString));
}

ImageDocumentEffects finishAnimationLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    return applyImageOpenTransition(state,
        rustImageOpenTransition(
            imageOpenWorkflowEvent(ImageOpenWorkflowEventKind::FinishAnimationLoadWithError)),
        ImageOpenTransitionContext::animationError(errorString));
}
}
