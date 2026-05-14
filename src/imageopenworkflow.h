// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEOPENWORKFLOW_H
#define KIRIVIEW_IMAGEOPENWORKFLOW_H

#include "imagedocumenteffects.h"
#include "imageloadtypes.h"
#include "kiriview/src/imageopenworkflow.cxx.h"

#include <QString>
#include <QUrl>

namespace KiriView {
class ImageDocumentState;

namespace ImageOpenWorkflow {
    ImageSourceLoadPlan sourceLoadPlan(const ImageSourceLoadPolicyInput &input);
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
