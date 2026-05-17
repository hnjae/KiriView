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
    bool hasImage, bool loadingContainerNavigationUrlEmpty)
{
    KiriView::ImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind::BeginSourceLoad);
    event.has_image = hasImage;
    event.loading_container_navigation_url_empty = loadingContainerNavigationUrlEmpty;
    return event;
}

KiriView::ImageOpenWorkflowEvent successfulImageLoadEvent(const KiriView::ImageLoadSession &session)
{
    KiriView::ImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind::FinishSuccessfulImageLoad);
    event.request_container_navigation_url_empty
        = session.request.containerNavigationUrl().isEmpty();
    return event;
}

KiriView::ImageOpenWorkflowEvent sourceLoadErrorEvent(
    bool containerNavigationUrlEmpty, bool hasImage, bool displayedUrlEmpty)
{
    KiriView::ImageOpenWorkflowEvent event
        = imageOpenWorkflowEvent(KiriView::ImageOpenWorkflowEventKind::FinishSourceLoadWithError);
    event.container_navigation_url_empty = containerNavigationUrlEmpty;
    event.has_image = hasImage;
    event.displayed_url_empty = displayedUrlEmpty;
    return event;
}
}

namespace KiriView::ImageOpenWorkflow {
ImageDocumentEffects beginSourceLoad(ImageDocumentState &state, bool hasImage)
{
    return applyImageOpenTransition(state,
        rustImageOpenTransition(
            beginSourceLoadEvent(hasImage, state.loadingContainerNavigationUrl().isEmpty())));
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
            sourceLoadErrorEvent(containerUrl.isEmpty(), hasImage, displayedUrl.isEmpty())),
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
