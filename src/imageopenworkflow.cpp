// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"

#include <utility>

namespace {
enum class FatalLoadZoomPolicy {
    PreserveZoom,
    ResetZoom,
};

class ImageOpenTransition final
{
public:
    explicit ImageOpenTransition(KiriView::ImageDocumentState &state)
        : m_state(state)
        , m_batch(m_state.beginChangeBatch())
    {
    }

    void clearImage() { add(KiriView::ImageDocumentEffect::clearImage()); }

    void resetZoom() { add(KiriView::ImageDocumentEffect::resetZoom()); }

    void updatePageNavigation() { add(KiriView::ImageDocumentEffect::updatePageNavigation()); }

    void scheduleAdjacentImagePredecode()
    {
        add(KiriView::ImageDocumentEffect::scheduleAdjacentImagePredecode());
    }

    void prepareFailedContainer(const QUrl &containerUrl)
    {
        add(KiriView::ImageDocumentEffect::prepareFailedContainer(containerUrl));
    }

    void finishTrackedLoad()
    {
        m_state.clearLoadingContainerNavigationUrl();
        m_state.setLoading(false);
    }

    void finishSuccessfulTrackedLoad()
    {
        m_state.setErrorString(QString());
        finishTrackedLoad();
        m_state.setStatus(KiriView::ImageDocumentStatus::Ready);
    }

    template <typename BeforeError>
    void finishTrackedLoadWithError(
        const QString &errorString, KiriView::ImageDocumentStatus status, BeforeError beforeError)
    {
        finishTrackedLoad();
        beforeError();
        finishWithError(errorString, status);
    }

    void finishTrackedLoadWithError(
        const QString &errorString, KiriView::ImageDocumentStatus status)
    {
        finishTrackedLoadWithError(errorString, status, []() { });
    }

    KiriView::ImageDocumentEffects takeEffects() { return std::move(m_effects); }

private:
    void add(KiriView::ImageDocumentEffect effect) { m_effects.add(std::move(effect)); }

    void finishWithError(const QString &errorString, KiriView::ImageDocumentStatus status)
    {
        m_state.setErrorString(errorString);
        m_state.setStatus(status);
    }

    KiriView::ImageDocumentState &m_state;
    KiriView::ImageDocumentState::ChangeBatch m_batch;
    KiriView::ImageDocumentEffects m_effects;
};

KiriView::ImageDocumentEffects finishClearedLoadWithError(
    KiriView::ImageDocumentState &state, const QString &errorString, FatalLoadZoomPolicy zoomPolicy)
{
    ImageOpenTransition transition(state);
    transition.clearImage();
    if (zoomPolicy == FatalLoadZoomPolicy::ResetZoom) {
        transition.resetZoom();
    }
    transition.finishTrackedLoadWithError(errorString, KiriView::ImageDocumentStatus::Error,
        [&]() { state.setContainerNavigationUrl(QUrl()); });
    return transition.takeEffects();
}
}

namespace KiriView {
ImageDocumentEffects ImageOpenWorkflow::beginSourceLoad(ImageDocumentState &state, bool hasImage)
{
    ImageOpenTransition transition(state);
    if (!hasImage && state.loadingContainerNavigationUrl().isEmpty()) {
        state.setContainerNavigationUrl(QUrl());
    }

    state.setLoading(true);
    if (!hasImage) {
        transition.clearImage();
        transition.resetZoom();
        state.setStatus(ImageDocumentStatus::Loading);
    } else {
        state.setStatus(ImageDocumentStatus::Ready);
    }
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishEmptySourceLoad(ImageDocumentState &state)
{
    ImageOpenTransition transition(state);
    transition.clearImage();
    transition.resetZoom();
    transition.finishTrackedLoad();
    state.setContainerNavigationUrl(QUrl());
    state.setStatus(ImageDocumentStatus::Null);
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishSuccessfulImageLoad(
    ImageDocumentState &state, const ImageLoadSession &session)
{
    ImageOpenTransition transition(state);
    state.setSourceUrl(session.location.imageUrl());
    state.setDisplayedImageLocation(session.location);
    if (!session.request.containerNavigationUrl().isEmpty()) {
        state.setContainerNavigationUrl(session.request.containerNavigationUrl());
    } else {
        state.setContainerNavigationUrl(containerNavigationUrlForLocation(session.location));
    }
    transition.finishSuccessfulTrackedLoad();
    transition.updatePageNavigation();
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishContainerNavigationLoadWithError(
    ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    ImageOpenTransition transition(state);
    transition.clearImage();
    transition.prepareFailedContainer(containerUrl);
    transition.finishTrackedLoadWithError(errorString, ImageDocumentStatus::Error, [&]() {
        state.setContainerNavigationUrl(containerUrl);
        state.setSourceUrl(containerUrl);
    });
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishReplacementLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    ImageOpenTransition transition(state);
    transition.finishTrackedLoadWithError(errorString, ImageDocumentStatus::Ready);

    if (!state.displayedUrl().isEmpty()) {
        state.setSourceUrl(state.displayedUrl());
    }

    transition.updatePageNavigation();
    transition.scheduleAdjacentImagePredecode();
    return transition.takeEffects();
}

ImageDocumentEffects ImageOpenWorkflow::finishInitialLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    return finishClearedLoadWithError(state, errorString, FatalLoadZoomPolicy::PreserveZoom);
}

ImageDocumentEffects ImageOpenWorkflow::finishAnimationLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    return finishClearedLoadWithError(state, errorString, FatalLoadZoomPolicy::ResetZoom);
}
}
