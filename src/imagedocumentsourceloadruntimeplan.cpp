// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadruntimeplan.h"

namespace {
void appendSourceLoadRuntimeOperation(KiriView::ImageDocumentRuntimePlan &runtimePlan,
    KiriView::ImageDocumentSourceLoadOperation operation,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    switch (operation) {
    case KiriView::ImageDocumentSourceLoadOperation::CancelFileDeletion:
        runtimePlan.push_back(KiriView::CancelFileDeletionOperation {});
        return;
    case KiriView::ImageDocumentSourceLoadOperation::CancelNavigationAndPredecode:
        runtimePlan.push_back(KiriView::CancelAllNavigationOperation {});
        runtimePlan.push_back(KiriView::CancelPredecodeOperation {});
        return;
    case KiriView::ImageDocumentSourceLoadOperation::FinishSpreadTransition:
        runtimePlan.push_back(KiriView::FinishSpreadTransitionOperation {});
        return;
    case KiriView::ImageDocumentSourceLoadOperation::ResetRightToLeftReading:
        runtimePlan.push_back(KiriView::ResetRightToLeftReadingOperation {});
        return;
    case KiriView::ImageDocumentSourceLoadOperation::NotifyRightToLeftReadingChanged:
        runtimePlan.push_back(KiriView::NotifyRightToLeftReadingChangedOperation {});
        return;
    case KiriView::ImageDocumentSourceLoadOperation::ClearSecondaryPage:
        runtimePlan.push_back(KiriView::ClearSecondaryPageOperation {});
        return;
    case KiriView::ImageDocumentSourceLoadOperation::ClearLoadingContainerNavigationUrl:
        runtimePlan.push_back(KiriView::ClearLoadingContainerNavigationUrlOperation {});
        return;
    case KiriView::ImageDocumentSourceLoadOperation::SetLoadingContainerNavigationUrlToRequested:
        runtimePlan.push_back(KiriView::SetLoadingContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case KiriView::ImageDocumentSourceLoadOperation::SetContainerNavigationUrlToRequested:
        runtimePlan.push_back(KiriView::SetContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case KiriView::ImageDocumentSourceLoadOperation::PrepareSourceLoad:
        runtimePlan.push_back(KiriView::PrepareSourceLoadOperation { request });
        return;
    case KiriView::ImageDocumentSourceLoadOperation::SetSourceUrlToRequested:
        runtimePlan.push_back(KiriView::SetSourceUrlOperation { request.sourceUrl });
        return;
    case KiriView::ImageDocumentSourceLoadOperation::BeginOpen:
        runtimePlan.push_back(KiriView::BeginOpenOperation {});
        return;
    }
}
}

namespace KiriView {
ImageDocumentRuntimePlan imageDocumentSourceLoadRuntimePlan(
    const ImageDocumentSourceLoadRequest &request, const ImageDocumentSourceLoadPlan &plan)
{
    ImageDocumentRuntimePlan runtimePlan;
    runtimePlan.reserve(plan.operations.size());
    for (ImageDocumentSourceLoadOperation operation : plan.operations) {
        appendSourceLoadRuntimeOperation(runtimePlan, operation, request);
    }
    return runtimePlan;
}
}
