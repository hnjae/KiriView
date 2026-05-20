// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadpolicy.h"

#include "kiriview/src/policy/imagedocumentsourceloadpolicy.cxx.h"
#include "navigation/imagecontainer.h"

namespace {
KiriView::ImageDocumentSourceLoadKind sourceLoadKind(
    const KiriView::ImageDocumentSourceLoadSnapshot &snapshot,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    if (snapshot.currentSourceUrl == request.sourceUrl) {
        return KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    }

    return KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
}

bool sourceWithinDisplayedComicBookArchive(
    const KiriView::ImageDocumentSourceLoadSnapshot &snapshot,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    return snapshot.displayedArchiveDocument.isComicBook()
        && KiriView::archiveDocumentContainsUrl(
            snapshot.displayedArchiveDocument, request.sourceUrl);
}

KiriView::RustImageDocumentSourceLoadKind rustSourceLoadKind(
    KiriView::ImageDocumentSourceLoadKind loadKind)
{
    switch (loadKind) {
    case KiriView::ImageDocumentSourceLoadKind::CurrentSource:
        return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
    case KiriView::ImageDocumentSourceLoadKind::ReplacementSource:
        return KiriView::RustImageDocumentSourceLoadKind::ReplacementSource;
    }

    return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
}

void appendSourceLoadRuntimeOperation(KiriView::ImageDocumentRuntimePlan &runtimePlan,
    KiriView::RustImageDocumentSourceLoadOperation operation,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    using RustOperation = KiriView::RustImageDocumentSourceLoadOperation;

    switch (operation) {
    case RustOperation::CancelFileDeletion:
        runtimePlan.push_back(KiriView::CancelFileDeletionOperation {});
        return;
    case RustOperation::CancelAllNavigation:
        runtimePlan.push_back(KiriView::CancelAllNavigationOperation {});
        return;
    case RustOperation::CancelPredecode:
        runtimePlan.push_back(KiriView::CancelPredecodeOperation {});
        return;
    case RustOperation::FinishSpreadTransition:
        runtimePlan.push_back(KiriView::FinishSpreadTransitionOperation {});
        return;
    case RustOperation::ResetRightToLeftReading:
        runtimePlan.push_back(KiriView::ResetRightToLeftReadingOperation {});
        return;
    case RustOperation::NotifyRightToLeftReadingChanged:
        runtimePlan.push_back(KiriView::NotifyRightToLeftReadingChangedOperation {});
        return;
    case RustOperation::ClearSecondaryPage:
        runtimePlan.push_back(KiriView::ClearSecondaryPageOperation {});
        return;
    case RustOperation::ClearLoadingContainerNavigationUrl:
        runtimePlan.push_back(KiriView::ClearLoadingContainerNavigationUrlOperation {});
        return;
    case RustOperation::SetLoadingContainerNavigationUrlToRequested:
        runtimePlan.push_back(KiriView::SetLoadingContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case RustOperation::SetContainerNavigationUrlToRequested:
        runtimePlan.push_back(KiriView::SetContainerNavigationUrlOperation {
            request.containerNavigationUrl,
        });
        return;
    case RustOperation::PrepareSourceLoad:
        runtimePlan.push_back(KiriView::PrepareSourceLoadOperation { request });
        return;
    case RustOperation::SetSourceUrlToRequested:
        runtimePlan.push_back(KiriView::SetSourceUrlOperation { request.sourceUrl });
        return;
    case RustOperation::BeginOpen:
        runtimePlan.push_back(KiriView::BeginOpenOperation {});
        return;
    }
}

KiriView::RustImageDocumentSourceLoadPolicyInput rustSourceLoadPolicyInput(
    const KiriView::ImageDocumentSourceLoadPolicyInput &input)
{
    return KiriView::RustImageDocumentSourceLoadPolicyInput {
        rustSourceLoadKind(input.loadKind),
        input.preserveTwoPageSpreadTransition,
        input.rightToLeftReadingEnabled,
        input.sourceWithinDisplayedComicBookArchive,
        input.hasRequestedContainerNavigationUrl,
    };
}

KiriView::ImageDocumentRuntimePlan runtimePlanFromRust(
    const KiriView::RustImageDocumentSourceLoadPlan &rustPlan,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    KiriView::ImageDocumentRuntimePlan plan;
    plan.reserve(rustPlan.operations.size());
    for (KiriView::RustImageDocumentSourceLoadOperation operation : rustPlan.operations) {
        appendSourceLoadRuntimeOperation(plan, operation, request);
    }
    return plan;
}

}

namespace KiriView {
ImageDocumentSourceLoadPolicyInput imageDocumentSourceLoadPolicyInput(
    const ImageDocumentSourceLoadSnapshot &snapshot, const ImageDocumentSourceLoadRequest &request)
{
    return ImageDocumentSourceLoadPolicyInput {
        sourceLoadKind(snapshot, request),
        request.preserveTwoPageSpreadTransition,
        snapshot.rightToLeftReadingEnabled,
        sourceWithinDisplayedComicBookArchive(snapshot, request),
        !request.containerNavigationUrl.isEmpty(),
    };
}
}

namespace KiriView::ImageDocumentSourceLoadPolicy {
ImageDocumentRuntimePlan plan(
    const ImageDocumentSourceLoadPolicyInput &input, const ImageDocumentSourceLoadRequest &request)
{
    return runtimePlanFromRust(
        rustImageDocumentSourceLoadPlan(rustSourceLoadPolicyInput(input)), request);
}
}
