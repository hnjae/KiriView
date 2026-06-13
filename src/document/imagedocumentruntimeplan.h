// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTRUNTIMEPLAN_H
#define KIRIVIEW_IMAGEDOCUMENTRUNTIMEPLAN_H

#include "imagedocumentsourceloadrequest.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QString>
#include <QUrl>
#include <variant>
#include <vector>

namespace kiriview {
struct CancelFileDeletionOperation {
};
struct StopPresentationAnimationOperation {
};
struct ShutdownSpreadOperation {
};
struct ClearMediaEntrySourceOperation {
};
struct ClearPredecodeOperation {
};
struct CancelPredecodeOperation {
};
struct ScheduleAdjacentImagePredecodeOperation {
};
struct FinishSpreadTransitionOperation {
};
struct ResetRightToLeftReadingOperation {
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
    ImageDocumentPageTarget target;
};
struct LoadContainerImageOperation {
    ImageDocumentPageTarget target;
    QUrl containerUrl;
};
struct FinishEmptyContainerNavigationOperation {
    QUrl containerUrl;
};
struct FinishContainerNavigationLoadWithErrorOperation {
    QUrl containerUrl;
    QString errorString;
};
struct ReportContainerNavigationBoundaryOperation {
    NavigationDirection direction = NavigationDirection::Next;
};
struct ReportContainerNavigationListFailureOperation {
    ContainerNavigationListFailure failure;
};
struct LoadPageNavigationUrlOperation {
    ImageDocumentPageTarget target;
    bool preserveTwoPageSpreadTransition = false;
};
struct CancelOpenOperation {
};
struct ClearDisplayedImageLocationOperation {
};
struct ClearPresentationImageOperation {
};
struct ClearLoadingContainerNavigationUrlOperation {
};
struct SetLoadingContainerNavigationUrlOperation {
    QUrl url;
};
struct SetContainerNavigationUrlOperation {
    QUrl url;
};
struct PrepareSourceLoadOperation {
    ImageDocumentSourceLoadRequest request;
};
struct SetSourceUrlOperation {
    ImageDocumentPageTarget target;
};
struct BeginOpenOperation {
};
struct SetErrorStringOperation {
    QString errorString;
};
struct FinishEmptySourceLoadOperation {
};

using ImageDocumentRuntimeOperation = std::variant<CancelFileDeletionOperation,
    StopPresentationAnimationOperation, ShutdownSpreadOperation, ClearMediaEntrySourceOperation,
    ClearPredecodeOperation, CancelPredecodeOperation, ScheduleAdjacentImagePredecodeOperation,
    FinishSpreadTransitionOperation, ResetRightToLeftReadingOperation, ClearSecondaryPageOperation,
    NotifyRightToLeftReadingChangedOperation, ResetZoomOperation, PrepareFailedContainerOperation,
    CancelPageNavigationUpdateOperation, CancelNavigationOperation,
    CancelContainerNavigationOperation, CancelAllNavigationOperation, ClearPageNavigationOperation,
    UpdatePageNavigationOperation, LoadUrlOperation, LoadContainerImageOperation,
    FinishEmptyContainerNavigationOperation, FinishContainerNavigationLoadWithErrorOperation,
    ReportContainerNavigationBoundaryOperation, ReportContainerNavigationListFailureOperation,
    LoadPageNavigationUrlOperation, CancelOpenOperation, ClearDisplayedImageLocationOperation,
    ClearPresentationImageOperation, ClearLoadingContainerNavigationUrlOperation,
    SetLoadingContainerNavigationUrlOperation, SetContainerNavigationUrlOperation,
    PrepareSourceLoadOperation, SetSourceUrlOperation, BeginOpenOperation, SetErrorStringOperation,
    FinishEmptySourceLoadOperation>;

using ImageDocumentRuntimePlan = std::vector<ImageDocumentRuntimeOperation>;

ImageDocumentRuntimePlan imageDocumentClearImagePlan();
ImageDocumentRuntimePlan imageDocumentClearLoadingPresentationPlan();
ImageDocumentRuntimePlan imageDocumentClearDeletedImagePlan();
ImageDocumentRuntimePlan imageDocumentShutdownPlan();
}

#endif
