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

enum class ImageOpenRightToLeftReadingNotification {
    None,
    BeforeOpen,
    AfterOpen,
};

struct ImageOpenSourceLoadPlan {
    bool finishSpreadTransition = false;
    bool resetRightToLeftReading = false;
    ImageOpenRightToLeftReadingNotification rightToLeftReadingNotification
        = ImageOpenRightToLeftReadingNotification::None;
    bool clearLoadingContainerNavigationUrl = false;
    bool updateContainerNavigationUrl = false;
    bool cancelNavigationAndPredecode = false;
    bool clearSecondaryPage = false;
    bool setLoadingContainerNavigationUrl = false;
    bool setSourceUrl = false;
    bool beginOpen = false;
};

struct ImageOpenSourceLoadRequest {
    bool sourceUrlChanged = false;
    bool preserveTwoPageSpreadTransition = false;
    bool resetRightToLeftReading = false;
    bool rightToLeftReadingEnabled = false;
    bool containerNavigationUrlEmpty = false;
};

class ImageOpenWorkflow
{
public:
    static ImageOpenSourceLoadPlan sourceLoadPlan(const ImageOpenSourceLoadRequest &request);
    static ImageDocumentEffects beginSourceLoad(ImageDocumentState &state, bool hasImage);
    static ImageDocumentEffects finishEmptySourceLoad(ImageDocumentState &state);
    static ImageDocumentEffects finishSuccessfulImageLoad(
        ImageDocumentState &state, const ImageLoadSession &session);
    static ImageDocumentEffects finishLoadWithError(ImageDocumentState &state,
        const ImageLoadSession &session, bool hasImage, const QString &errorString);
    static ImageDocumentEffects finishContainerNavigationLoadWithError(
        ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString);
    static ImageDocumentEffects finishAnimationLoadWithError(
        ImageDocumentState &state, const QString &errorString);
};
}

#endif
