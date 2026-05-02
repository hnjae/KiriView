// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"

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
    state.setLoading(false);
    state.clearLoadingContainerNavigationUrl();
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
        state.setContainerNavigationUrl(containerNavigationUrlForImage(
            session.location.imageUrl(), session.location.comicBookRootUrl()));
    }
    state.clearLoadingContainerNavigationUrl();
    state.setErrorString(QString());
    state.setLoading(false);
    state.setStatus(ImageDocumentStatus::Ready);
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
    state.clearLoadingContainerNavigationUrl();
    state.setLoading(false);
    state.setContainerNavigationUrl(containerUrl);
    state.setSourceUrl(containerUrl);
    state.setErrorString(errorString);
    state.setStatus(ImageDocumentStatus::Error);
    return effects;
}

ImageDocumentEffects ImageOpenWorkflow::finishReplacementLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    [[maybe_unused]] auto batch = state.beginChangeBatch();
    ImageDocumentEffects effects;
    state.clearLoadingContainerNavigationUrl();
    state.setLoading(false);
    state.setErrorString(errorString);
    state.setStatus(ImageDocumentStatus::Ready);

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
    state.clearLoadingContainerNavigationUrl();
    state.setLoading(false);
    state.setContainerNavigationUrl(QUrl());
    state.setErrorString(errorString);
    state.setStatus(ImageDocumentStatus::Error);
    return effects;
}

ImageDocumentEffects ImageOpenWorkflow::finishAnimationLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    [[maybe_unused]] auto batch = state.beginChangeBatch();
    ImageDocumentEffects effects;
    effects.add(ImageDocumentEffect::clearImage());
    effects.add(ImageDocumentEffect::resetZoom());
    state.setLoading(false);
    state.setContainerNavigationUrl(QUrl());
    state.setErrorString(errorString);
    state.setStatus(ImageDocumentStatus::Error);
    return effects;
}
}
