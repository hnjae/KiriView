// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOW_H
#define KIRIVIEW_IMAGEOPENWORKFLOW_H

#include "imagedocumenteffects.h"
#include "imageloadtypes.h"

#include <QString>
#include <QUrl>
#include <vector>

namespace KiriView {
class ImageDocumentState;

enum class ImageOpenSourceLoadAction {
    CancelNavigationAndPredecode,
    FinishSpreadTransition,
    ResetRightToLeftReading,
    NotifyRightToLeftReading,
    ClearSecondaryPage,
    ClearLoadingContainerNavigationUrl,
    UpdateContainerNavigationUrl,
    SetLoadingContainerNavigationUrl,
    SetSourceUrl,
    BeginOpen,
};

struct ImageOpenSourceLoadPlan {
    std::vector<ImageOpenSourceLoadAction> actions;
};

struct ImageOpenSourceLoadRequest {
    bool sourceUrlChanged = false;
    bool preserveTwoPageSpreadTransition = false;
    bool resetRightToLeftReading = false;
    bool rightToLeftReadingEnabled = false;
    bool containerNavigationUrlEmpty = false;
};

namespace ImageOpenWorkflow {
    ImageOpenSourceLoadPlan sourceLoadPlan(const ImageOpenSourceLoadRequest &request);
    ImageDocumentEffects beginSourceLoad(ImageDocumentState &state, bool hasImage);
    ImageDocumentEffects finishEmptySourceLoad(ImageDocumentState &state);
    ImageDocumentEffects finishSuccessfulImageLoad(
        ImageDocumentState &state, const ImageLoadSession &session);
    ImageDocumentEffects finishLoadWithError(ImageDocumentState &state,
        const ImageLoadSession &session, bool hasImage, const QString &errorString);
    ImageDocumentEffects finishContainerNavigationLoadWithError(
        ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString);
    ImageDocumentEffects finishAnimationLoadWithError(
        ImageDocumentState &state, const QString &errorString);
}
}

#endif
