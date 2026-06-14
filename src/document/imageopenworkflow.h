// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOW_H
#define KIRIVIEW_IMAGEOPENWORKFLOW_H

#include "imagedocumentruntimeplan.h"
#include "imagedocumentsourceloadrequest.h"
#include "imageopenapplicationplan.h"
#include "imageopentransition.h"
#include "location/imagelocation.h"
#include "metadata/embeddedmetadata.h"

#include <vector>

namespace kiriview {
struct ImageDocumentSourceLoadSnapshot {
    QUrl currentSourceUrl;
    OpenedCollectionScopeLocation displayedOpenedCollectionScope;
    bool rightToLeftReadingEnabled = false;
};

struct ImageOpenBeginSourceLoadSnapshot {
    bool hasImage = false;
    bool hasLoadingContainerNavigationTarget = false;
};

struct ImageOpenSuccessfulImageLoadSnapshot {
    bool hasRequestContainerNavigationTarget = false;
};

struct ImageOpenLoadErrorSnapshot {
    bool hasContainerNavigationTarget = false;
    bool hasImage = false;
    bool hasDisplayedUrl = false;
};

enum class ImageDocumentSourceLoadEffect {
    CancelFileDeletion,
    CancelAllNavigation,
    CancelPredecode,
    FinishSpreadTransition,
    ResetRightToLeftReading,
    NotifyRightToLeftReadingChanged,
    ClearSecondaryPage,
    ClearLoadingContainerNavigationUrl,
    SetLoadingContainerNavigationUrlToRequested,
    SetContainerNavigationUrlToRequested,
    PrepareSourceLoad,
    SetSourceUrlToRequested,
    BeginOpen,
};

using ImageDocumentSourceLoadPlan = std::vector<ImageDocumentSourceLoadEffect>;

namespace ImageOpenWorkflow {
    ImageDocumentRuntimePlan sourceLoadPlan(const ImageDocumentSourceLoadSnapshot &snapshot,
        const ImageDocumentSourceLoadRequest &request);
    ImageOpenApplicationPlan beginSourceLoadPlan(ImageOpenBeginSourceLoadSnapshot snapshot);
    ImageOpenApplicationPlan finishEmptySourceLoadPlan();
    ImageOpenApplicationPlan resolveSourceImagePlan(const ImageLoadSession &session);
    ImageOpenApplicationPlan finishUnsupportedOpenedCollectionVideoLoadPlan(
        const ImageLoadSession &session);
    ImageOpenApplicationPlan finishSuccessfulImageLoadPlan(
        ImageOpenSuccessfulImageLoadSnapshot snapshot, const ImageLoadSession &session);
    ImageOpenApplicationPlan finishSuccessfulImageLoadPlan(
        ImageOpenSuccessfulImageLoadSnapshot snapshot, const ImageLoadSession &session,
        EmbeddedMetadata metadata);
    ImageOpenApplicationPlan finishLoadWithErrorPlan(ImageOpenLoadErrorSnapshot snapshot,
        const ImageLoadSession &session, const QUrl &displayedUrl, ImageLoadFailure failure);
    ImageOpenApplicationPlan finishContainerNavigationLoadWithErrorPlan(
        const QUrl &containerUrl, const QString &errorString);
    ImageOpenApplicationPlan finishAnimationLoadWithErrorPlan(const QString &errorString);
    ImageOpenTransition beginSourceLoadTransition(ImageOpenBeginSourceLoadSnapshot snapshot);
    ImageOpenTransition finishEmptySourceLoadTransition();
    ImageOpenTransition resolveSourceImageTransition();
    ImageOpenTransition finishUnsupportedOpenedCollectionVideoLoadTransition();
    ImageOpenTransition finishSuccessfulImageLoadTransition(
        ImageOpenSuccessfulImageLoadSnapshot snapshot);
    ImageOpenTransition finishLoadWithErrorTransition(ImageOpenLoadErrorSnapshot snapshot);
    ImageOpenTransition finishContainerNavigationLoadWithErrorTransition();
    ImageOpenTransition finishAnimationLoadWithErrorTransition();
}
}

#endif
