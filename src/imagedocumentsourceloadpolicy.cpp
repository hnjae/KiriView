// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadpolicy.h"

#include "imagecontainer.h"
#include "kiriview/src/imagedocumentsourceloadpolicy.cxx.h"

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

KiriView::ImageDocumentSourceLoadOperation sourceLoadOperation(
    KiriView::RustImageDocumentSourceLoadOperation operation)
{
    using Operation = KiriView::ImageDocumentSourceLoadOperation;
    using RustOperation = KiriView::RustImageDocumentSourceLoadOperation;

    switch (operation) {
    case RustOperation::CancelFileDeletion:
        return Operation::CancelFileDeletion;
    case RustOperation::CancelNavigationAndPredecode:
        return Operation::CancelNavigationAndPredecode;
    case RustOperation::FinishSpreadTransition:
        return Operation::FinishSpreadTransition;
    case RustOperation::ResetRightToLeftReading:
        return Operation::ResetRightToLeftReading;
    case RustOperation::NotifyRightToLeftReadingChanged:
        return Operation::NotifyRightToLeftReadingChanged;
    case RustOperation::ClearSecondaryPage:
        return Operation::ClearSecondaryPage;
    case RustOperation::ClearLoadingContainerNavigationUrl:
        return Operation::ClearLoadingContainerNavigationUrl;
    case RustOperation::SetLoadingContainerNavigationUrlToRequested:
        return Operation::SetLoadingContainerNavigationUrlToRequested;
    case RustOperation::SetContainerNavigationUrlToRequested:
        return Operation::SetContainerNavigationUrlToRequested;
    case RustOperation::PrepareSourceLoad:
        return Operation::PrepareSourceLoad;
    case RustOperation::SetSourceUrlToRequested:
        return Operation::SetSourceUrlToRequested;
    case RustOperation::BeginOpen:
        return Operation::BeginOpen;
    }

    return Operation::BeginOpen;
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

KiriView::ImageDocumentSourceLoadPlan sourceLoadPlan(
    const KiriView::RustImageDocumentSourceLoadPlan &rustPlan)
{
    KiriView::ImageDocumentSourceLoadPlan plan;
    plan.operations.reserve(rustPlan.operations.size());
    for (KiriView::RustImageDocumentSourceLoadOperation operation : rustPlan.operations) {
        plan.operations.push_back(sourceLoadOperation(operation));
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
ImageDocumentSourceLoadPlan plan(const ImageDocumentSourceLoadPolicyInput &input)
{
    return sourceLoadPlan(rustImageDocumentSourceLoadPlan(rustSourceLoadPolicyInput(input)));
}
}
