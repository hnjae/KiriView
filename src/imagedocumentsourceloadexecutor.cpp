// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadexecutor.h"

#include "imagecallback.h"

namespace {
void applyRightToLeftReadingTransition(
    KiriView::ImageDocumentRightToLeftReadingTransition transition,
    const KiriView::ImageDocumentSourceLoadOperations &operations, bool notifyBeforeSourceState,
    bool notifyAfterOpen)
{
    switch (transition) {
    case KiriView::ImageDocumentRightToLeftReadingTransition::Keep:
        return;
    case KiriView::ImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState:
        if (notifyBeforeSourceState) {
            KiriView::invokeIfSet(operations.resetRightToLeftReading);
            KiriView::invokeIfSet(operations.notifyRightToLeftReadingChanged);
        }
        return;
    case KiriView::ImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen:
        if (notifyBeforeSourceState) {
            KiriView::invokeIfSet(operations.resetRightToLeftReading);
        }
        if (notifyAfterOpen) {
            KiriView::invokeIfSet(operations.notifyRightToLeftReadingChanged);
        }
        return;
    }
}

void applyLoadingContainerNavigationUrlTarget(
    KiriView::ImageDocumentSourceLoadPendingContainerTarget target,
    const KiriView::ImageDocumentSourceLoadRequest &request,
    const KiriView::ImageDocumentSourceLoadOperations &operations)
{
    switch (target) {
    case KiriView::ImageDocumentSourceLoadPendingContainerTarget::Unchanged:
        return;
    case KiriView::ImageDocumentSourceLoadPendingContainerTarget::Empty:
        KiriView::invokeIfSet(operations.clearLoadingContainerNavigationUrl);
        return;
    case KiriView::ImageDocumentSourceLoadPendingContainerTarget::RequestedContainerNavigation:
        KiriView::invokeIfSet(
            operations.setLoadingContainerNavigationUrl, request.containerNavigationUrl);
        return;
    }
}

void applyContainerNavigationUrlTarget(KiriView::ImageDocumentSourceLoadContainerTarget target,
    const KiriView::ImageDocumentSourceLoadRequest &request,
    const KiriView::ImageDocumentSourceLoadOperations &operations)
{
    switch (target) {
    case KiriView::ImageDocumentSourceLoadContainerTarget::Unchanged:
        return;
    case KiriView::ImageDocumentSourceLoadContainerTarget::RequestedContainerNavigation:
        KiriView::invokeIfSet(operations.setContainerNavigationUrl, request.containerNavigationUrl);
        return;
    }
}

void applySourceLoadUrlTarget(KiriView::ImageDocumentSourceLoadSourceTarget target,
    const KiriView::ImageDocumentSourceLoadRequest &request,
    const KiriView::ImageDocumentSourceLoadOperations &operations)
{
    switch (target) {
    case KiriView::ImageDocumentSourceLoadSourceTarget::Unchanged:
        return;
    case KiriView::ImageDocumentSourceLoadSourceTarget::RequestedSource:
        KiriView::invokeIfSet(operations.prepareSourceLoad, request);
        KiriView::invokeIfSet(operations.setSourceUrl, request.sourceUrl);
        return;
    }
}
}

namespace KiriView {
void executeImageDocumentSourceLoadPlan(const ImageDocumentSourceLoadRequest &request,
    const ImageDocumentSourceLoadPlan &plan, const ImageDocumentSourceLoadOperations &operations)
{
    if (plan.cancelNavigationAndPredecode) {
        invokeIfSet(operations.cancelNavigationAndPredecode);
    }
    if (plan.finishSpreadTransition) {
        invokeIfSet(operations.finishSpreadTransition);
    }
    applyRightToLeftReadingTransition(plan.rightToLeftReadingTransition, operations, true, false);
    if (plan.clearSecondaryPage) {
        invokeIfSet(operations.clearSecondaryPage);
    }
    applyLoadingContainerNavigationUrlTarget(
        plan.loadingContainerNavigationUrl, request, operations);
    applyContainerNavigationUrlTarget(plan.containerNavigationUrl, request, operations);
    applySourceLoadUrlTarget(plan.sourceUrl, request, operations);
    if (plan.beginOpen) {
        invokeIfSet(operations.beginOpen);
    }
    applyRightToLeftReadingTransition(plan.rightToLeftReadingTransition, operations, false, true);
}
}
