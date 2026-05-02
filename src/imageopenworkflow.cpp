// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopenworkflow.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"

namespace KiriView {
ImageOpenCommands ImageOpenWorkflow::beginSourceLoad(ImageDocumentState &state, bool hasImage)
{
    ImageOpenCommands commands;
    if (!hasImage && state.loadingContainerNavigationUrl().isEmpty()) {
        state.setContainerNavigationUrl(QUrl());
    }

    state.setLoading(true);
    if (!hasImage) {
        commands.clearImage = true;
        commands.resetZoom = true;
        state.setStatus(ImageDocumentStatus::Loading);
    } else {
        state.setStatus(ImageDocumentStatus::Ready);
    }
    return commands;
}

ImageOpenCommands ImageOpenWorkflow::finishEmptySourceLoad(ImageDocumentState &state)
{
    ImageOpenCommands commands;
    commands.clearImage = true;
    commands.resetZoom = true;
    state.setLoading(false);
    state.clearLoadingContainerNavigationUrl();
    state.setContainerNavigationUrl(QUrl());
    state.setStatus(ImageDocumentStatus::Null);
    return commands;
}

ImageOpenCommands ImageOpenWorkflow::finishSuccessfulImageLoad(
    ImageDocumentState &state, const ImageLoadSession &session)
{
    ImageOpenCommands commands;
    state.setSourceUrl(session.location.imageUrl);
    state.setDisplayedImageLocation(session.location);
    if (!session.request.containerNavigationUrl().isEmpty()) {
        state.setContainerNavigationUrl(session.request.containerNavigationUrl());
    } else {
        state.setContainerNavigationUrl(containerNavigationUrlForImage(
            session.location.imageUrl, session.location.comicBookRootUrl));
    }
    state.clearLoadingContainerNavigationUrl();
    state.setErrorString(QString());
    state.setLoading(false);
    state.setStatus(ImageDocumentStatus::Ready);
    commands.updatePageNavigation = true;
    return commands;
}

ImageOpenCommands ImageOpenWorkflow::finishContainerNavigationLoadWithError(
    ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString)
{
    ImageOpenCommands commands;
    commands.clearImage = true;
    commands.failedContainerUrl = containerUrl;
    state.clearLoadingContainerNavigationUrl();
    state.setLoading(false);
    state.setContainerNavigationUrl(containerUrl);
    state.setSourceUrl(containerUrl);
    state.setErrorString(errorString);
    state.setStatus(ImageDocumentStatus::Error);
    return commands;
}

ImageOpenCommands ImageOpenWorkflow::finishReplacementLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    ImageOpenCommands commands;
    state.clearLoadingContainerNavigationUrl();
    state.setLoading(false);
    state.setErrorString(errorString);
    state.setStatus(ImageDocumentStatus::Ready);

    if (!state.displayedUrl().isEmpty()) {
        state.setSourceUrl(state.displayedUrl());
    }

    commands.updatePageNavigation = true;
    commands.scheduleAdjacentPredecode = true;
    return commands;
}

ImageOpenCommands ImageOpenWorkflow::finishInitialLoadWithError(
    ImageDocumentState &state, const QString &errorString)
{
    ImageOpenCommands commands;
    commands.clearImage = true;
    state.clearLoadingContainerNavigationUrl();
    state.setLoading(false);
    state.setContainerNavigationUrl(QUrl());
    state.setErrorString(errorString);
    state.setStatus(ImageDocumentStatus::Error);
    return commands;
}
}
