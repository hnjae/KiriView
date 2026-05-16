// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOW_H
#define KIRIVIEW_IMAGEOPENWORKFLOW_H

#include "imagedocumenteffects.h"
#include "imageloadtypes.h"

#include <QString>
#include <QUrl>

namespace KiriView {
class ImageDocumentState;
class ImageSpreadPresentationController;
struct ImageDocumentSourceLoadRequest;

enum class ImageDocumentSourceLoadKind {
    CurrentSource,
    ReplacementSource,
};

enum class ImageDocumentRightToLeftReadingReset {
    Keep,
    ResetInactive,
    ResetActive,
};

enum class ImageDocumentRightToLeftReadingTransition {
    Keep,
    Reset,
    ResetAndNotifyBeforeSourceState,
    ResetAndNotifyAfterOpen,
};

enum class ImageDocumentSourceLoadUrlTarget {
    Unchanged,
    Empty,
    RequestedContainerNavigation,
    RequestedSource,
};

struct ImageDocumentSourceLoadPolicyInput {
    ImageDocumentSourceLoadKind loadKind = ImageDocumentSourceLoadKind::CurrentSource;
    bool preserveTwoPageSpreadTransition = false;
    ImageDocumentRightToLeftReadingReset rightToLeftReadingReset
        = ImageDocumentRightToLeftReadingReset::Keep;
    bool hasRequestedContainerNavigationUrl = false;
};

struct ImageDocumentSourceLoadPlan {
    bool cancelNavigationAndPredecode = false;
    bool finishSpreadTransition = false;
    ImageDocumentRightToLeftReadingTransition rightToLeftReadingTransition
        = ImageDocumentRightToLeftReadingTransition::Keep;
    bool clearSecondaryPage = false;
    ImageDocumentSourceLoadUrlTarget loadingContainerNavigationUrl
        = ImageDocumentSourceLoadUrlTarget::Unchanged;
    ImageDocumentSourceLoadUrlTarget containerNavigationUrl
        = ImageDocumentSourceLoadUrlTarget::Unchanged;
    ImageDocumentSourceLoadUrlTarget sourceUrl = ImageDocumentSourceLoadUrlTarget::Unchanged;
    bool beginOpen = false;
};

namespace ImageOpenWorkflow {
    ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentSourceLoadPolicyInput &input);
    ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentState &state,
        const ImageSpreadPresentationController &spreadController,
        const ImageDocumentSourceLoadRequest &request);
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
