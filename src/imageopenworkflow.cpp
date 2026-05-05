// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"

namespace {
void finishTrackedLoad(KiriView::ImageDocumentState &state)
{
    state.clearLoadingContainerNavigationUrl();
    state.setLoading(false);
}

void finishSuccessfulTrackedLoad(KiriView::ImageDocumentState &state)
{
    state.setErrorString(QString());
    finishTrackedLoad(state);
    state.setStatus(KiriView::ImageDocumentStatus::Ready);
}

void finishWithError(KiriView::ImageDocumentState &state, const QString &errorString,
    KiriView::ImageDocumentStatus status)
{
    state.setErrorString(errorString);
    state.setStatus(status);
}

template <typename BeforeError>
void finishTrackedLoadWithError(KiriView::ImageDocumentState &state, const QString &errorString,
    KiriView::ImageDocumentStatus status, BeforeError beforeError)
{
    finishTrackedLoad(state);
    beforeError();
    finishWithError(state, errorString, status);
}

void finishTrackedLoadWithError(KiriView::ImageDocumentState &state, const QString &errorString,
    KiriView::ImageDocumentStatus status)
{
    finishTrackedLoadWithError(state, errorString, status, []() { });
}
}

namespace KiriView {
ImageDocumentEffects ImageOpenWorkflow::beginSourceLoad(ImageDocumentState &state, bool hasImage)
{
    [[maybe_unused]] auto batch = state.beginChangeBatch();
    ImageDocumentEffects effects;
    if (!hasImage && state.loadingContainerNavigationUrl().isEmpty()) {
        state.setContainerNavigationUrl(QUrl());
    }

    state.setLoading(true);
    if (!hasImage) {
        effects.add(ImageDocumentEffect::clearImage());
        effects.add(ImageDocumentEffect::resetZoom());
        state.setStatus(ImageDocumentStatus::Loading);
    } else {
        state.setStatus(ImageDocumentStatus::Ready);
    }
    return effects;
}

ImageDocumentEffects ImageOpenWorkflow::finishEmptySourceLoad(ImageDocumentState &state)
{
    [[maybe_unused]] auto batch = state.beginChangeBatch();
    ImageDocumentEffects effects;
    effects.add(ImageDocumentEffect::clearImage());
    effects.add(ImageDocumentEffect::resetZoom());
    finishTrackedLoad(state);
    state.setContainerNavigationUrl(QUrl());
    state.setStatus(ImageDocumentStatus::Null);
    return effects;
}

ImageDocumentEffects ImageOpenWorkflow::finishSuccessfulImageLoad(
    ImageDocumentState &state, const ImageLoadSession &session)
{
    [[maybe_unused]] auto batch = state.beginChangeBatch();
    ImageDocumentEffects effects;
    state.setSourceUrl(session.location.imageUrl());
    state.setDisplayedImageLocation(session.location);
    if (!session.request.containerNavigationUrl().isEmpty()) {
        state.setContainerNavigationUrl(session.request.containerNavigationUrl());
    } else {
        state.setContainerNavigationUrl(containerNavigationUrlForLocation(session.location));
    }
    finishSuccessfulTrackedLoad(state);
    effects.add(ImageDocumentEffect::updatePageNavigation());
    return effects;
}

ImageDocumentEffects ImageOpenWorkflow::finishContainerNavigationLoadWithError(
    ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    [[maybe_unused]] auto batch = state.beginChangeBatch();
    ImageDocumentEffects effects;
    effects.add(ImageDocumentEffect::clearImage());
    effects.add(ImageDocumentEffect::prepareFailedContainer(containerUrl));
    finishTrackedLoadWithError(state, errorString, ImageDocumentStatus::Error, [&]() {
        state.setContainerNavigationUrl(containerUrl);
        state.setSourceUrl(containerUrl);
    });
    return effects;
}

ImageDocumentEffects ImageOpenWorkflow::finishReplacementLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    [[maybe_unused]] auto batch = state.beginChangeBatch();
    ImageDocumentEffects effects;
    finishTrackedLoadWithError(state, errorString, ImageDocumentStatus::Ready);

    if (!state.displayedUrl().isEmpty()) {
        state.setSourceUrl(state.displayedUrl());
    }

    effects.add(ImageDocumentEffect::updatePageNavigation());
    effects.add(ImageDocumentEffect::scheduleAdjacentImagePredecode());
    return effects;
}

ImageDocumentEffects ImageOpenWorkflow::finishInitialLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    [[maybe_unused]] auto batch = state.beginChangeBatch();
    ImageDocumentEffects effects;
    effects.add(ImageDocumentEffect::clearImage());
    finishTrackedLoadWithError(state, errorString, ImageDocumentStatus::Error,
        [&]() { state.setContainerNavigationUrl(QUrl()); });
    return effects;
}

ImageDocumentEffects ImageOpenWorkflow::finishAnimationLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    [[maybe_unused]] auto batch = state.beginChangeBatch();
    ImageDocumentEffects effects;
    effects.add(ImageDocumentEffect::clearImage());
    effects.add(ImageDocumentEffect::resetZoom());
    finishTrackedLoadWithError(state, errorString, ImageDocumentStatus::Error,
        [&]() { state.setContainerNavigationUrl(QUrl()); });
    return effects;
}
}
