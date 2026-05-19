// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEFFECTPLAN_H
#define KIRIVIEW_IMAGEDOCUMENTEFFECTPLAN_H

#include "imagedocumenteffects.h"

#include <QString>
#include <QUrl>
#include <variant>
#include <vector>

namespace KiriView {
enum class ImageDocumentRuntimeOperationKind {
    CancelFileDeletion,
    StopPresentationAnimation,
    ShutdownSpread,
    ClearArchiveSession,
    ClearPredecode,
    CancelPredecode,
    ScheduleAdjacentImagePredecode,
    FinishSpreadTransition,
    ClearSecondaryPage,
    NotifyRightToLeftReadingChanged,
    ResetZoom,
    PrepareFailedContainer,
    CancelPageNavigationUpdate,
    CancelNavigation,
    CancelContainerNavigation,
    CancelAllNavigation,
    ClearPageNavigation,
    UpdatePageNavigation,
    LoadUrl,
    LoadContainerImage,
    FinishEmptyContainerNavigation,
    FinishContainerNavigationLoadWithError,
    LoadPageNavigationUrl,
    CancelOpen,
    ClearDisplayedImageLocation,
    ClearPresentationImage,
    SetSourceUrl,
    SetErrorString,
    FinishEmptySourceLoad,
};

struct CancelFileDeletionOperation {
};
struct StopPresentationAnimationOperation {
};
struct ShutdownSpreadOperation {
};
struct ClearArchiveSessionOperation {
};
struct ClearPredecodeOperation {
};
struct CancelPredecodeOperation {
};
struct ScheduleAdjacentImagePredecodeOperation {
};
struct FinishSpreadTransitionOperation {
};
struct ClearSecondaryPageOperation {
};
struct NotifyRightToLeftReadingChangedOperation {
};
struct ResetZoomOperation {
};
struct PrepareFailedContainerOperation {
    QUrl containerUrl;
};
struct CancelPageNavigationUpdateOperation {
};
struct CancelNavigationOperation {
};
struct CancelContainerNavigationOperation {
};
struct CancelAllNavigationOperation {
};
struct ClearPageNavigationOperation {
};
struct UpdatePageNavigationOperation {
};
struct LoadUrlOperation {
    QUrl url;
};
struct LoadContainerImageOperation {
    QUrl imageUrl;
    QUrl containerUrl;
};
struct FinishEmptyContainerNavigationOperation {
    QUrl containerUrl;
};
struct FinishContainerNavigationLoadWithErrorOperation {
    QUrl containerUrl;
    QString errorString;
};
struct LoadPageNavigationUrlOperation {
    QUrl url;
    bool preserveTwoPageSpreadTransition = false;
};
struct CancelOpenOperation {
};
struct ClearDisplayedImageLocationOperation {
};
struct ClearPresentationImageOperation {
};
struct SetSourceUrlOperation {
    QUrl url;
};
struct SetErrorStringOperation {
    QString errorString;
};
struct FinishEmptySourceLoadOperation {
};

using ImageDocumentRuntimeOperation = std::variant<CancelFileDeletionOperation,
    StopPresentationAnimationOperation, ShutdownSpreadOperation, ClearArchiveSessionOperation,
    ClearPredecodeOperation, CancelPredecodeOperation, ScheduleAdjacentImagePredecodeOperation,
    FinishSpreadTransitionOperation, ClearSecondaryPageOperation,
    NotifyRightToLeftReadingChangedOperation, ResetZoomOperation, PrepareFailedContainerOperation,
    CancelPageNavigationUpdateOperation, CancelNavigationOperation,
    CancelContainerNavigationOperation, CancelAllNavigationOperation, ClearPageNavigationOperation,
    UpdatePageNavigationOperation, LoadUrlOperation, LoadContainerImageOperation,
    FinishEmptyContainerNavigationOperation, FinishContainerNavigationLoadWithErrorOperation,
    LoadPageNavigationUrlOperation, CancelOpenOperation, ClearDisplayedImageLocationOperation,
    ClearPresentationImageOperation, SetSourceUrlOperation, SetErrorStringOperation,
    FinishEmptySourceLoadOperation>;

using ImageDocumentRuntimePlan = std::vector<ImageDocumentRuntimeOperation>;

ImageDocumentRuntimeOperationKind imageDocumentRuntimeOperationKind(
    const ImageDocumentRuntimeOperation &operation);
ImageDocumentRuntimePlan imageDocumentEffectPlan(const ImageDocumentEffect &effect);
ImageDocumentRuntimePlan imageDocumentShutdownPlan();
}

#endif
