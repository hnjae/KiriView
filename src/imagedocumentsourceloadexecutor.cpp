// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadexecutor.h"

#include "imagecallback.h"

namespace {
void executeImageDocumentSourceLoadOperation(KiriView::ImageDocumentSourceLoadOperation operation,
    const KiriView::ImageDocumentSourceLoadRequest &request,
    const KiriView::ImageDocumentSourceLoadOperations &operations)
{
    switch (operation) {
    case KiriView::ImageDocumentSourceLoadOperation::CancelNavigationAndPredecode:
        KiriView::invokeIfSet(operations.cancelNavigationAndPredecode);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::FinishSpreadTransition:
        KiriView::invokeIfSet(operations.finishSpreadTransition);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::ResetRightToLeftReading:
        KiriView::invokeIfSet(operations.resetRightToLeftReading);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged:
        KiriView::invokeIfSet(operations.notifyRightToLeftReadingChanged);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::ClearSecondaryPage:
        KiriView::invokeIfSet(operations.clearSecondaryPage);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl:
        KiriView::invokeIfSet(operations.clearLoadingContainerNavigationUrl);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested:
        KiriView::invokeIfSet(
            operations.setLoadingContainerNavigationUrl, request.containerNavigationUrl);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::SetContainerNavigationUrlToRequested:
        KiriView::invokeIfSet(operations.setContainerNavigationUrl, request.containerNavigationUrl);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::PrepareSourceLoad:
        KiriView::invokeIfSet(operations.prepareSourceLoad, request);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::SetSourceUrlToRequested:
        KiriView::invokeIfSet(operations.setSourceUrl, request.sourceUrl);
        return;
    case KiriView::ImageDocumentSourceLoadOperation::BeginOpen:
        KiriView::invokeIfSet(operations.beginOpen);
        return;
    }
}
}

namespace KiriView {
void executeImageDocumentSourceLoadPlan(const ImageDocumentSourceLoadRequest &request,
    const ImageDocumentSourceLoadPlan &plan, const ImageDocumentSourceLoadOperations &operations)
{
    for (ImageDocumentSourceLoadOperation operation : plan.operations) {
        executeImageDocumentSourceLoadOperation(operation, request, operations);
    }
}
}
