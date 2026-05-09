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

enum class ImageOpenFailureTarget {
    ContainerNavigation,
    Replacement,
    Initial,
};

enum class ImageOpenSourceTarget {
    EmptySource,
    LoadSource,
};

struct ImageOpenSourceLoadPlan {
    bool finishSpreadTransition = false;
    bool resetRightToLeftReading = false;
    bool clearLoadingContainerNavigationUrl = false;
    bool updateContainerNavigationUrl = false;
    bool cancelNavigationAndPredecode = false;
    bool clearSecondaryPage = false;
    bool setLoadingContainerNavigationUrl = false;
    bool setSourceUrl = false;
    bool beginOpen = false;
};

class ImageOpenWorkflow
{
public:
    static ImageOpenSourceTarget sourceTargetForOpen(const ImageDocumentState &state);
    static ImageOpenSourceLoadPlan sourceLoadPlan(bool sourceUrlChanged,
        bool preserveTwoPageSpreadTransition, bool resetRightToLeftReading,
        bool containerNavigationUrlEmpty);
    static ImageOpenFailureTarget failureTargetForLoadError(
        const ImageLoadSession &session, bool hasImage);
    static ImageDocumentEffects beginSourceLoad(ImageDocumentState &state, bool hasImage);
    static ImageDocumentEffects finishEmptySourceLoad(ImageDocumentState &state);
    static ImageDocumentEffects finishSuccessfulImageLoad(
        ImageDocumentState &state, const ImageLoadSession &session);
    static ImageDocumentEffects finishContainerNavigationLoadWithError(
        ImageDocumentState &state, const QUrl &containerUrl, const QString &errorString);
    static ImageDocumentEffects finishReplacementLoadWithError(
        ImageDocumentState &state, const QString &errorString);
    static ImageDocumentEffects finishInitialLoadWithError(
        ImageDocumentState &state, const QString &errorString);
    static ImageDocumentEffects finishAnimationLoadWithError(
        ImageDocumentState &state, const QString &errorString);
};
}

#endif
