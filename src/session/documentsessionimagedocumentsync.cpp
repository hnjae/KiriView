// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionimagedocumentsync.h"

namespace {
bool sameActiveNavigationSnapshot(const kiriview::ImageDocumentPageActiveNavigationSnapshot& left,
    const kiriview::ImageDocumentPageActiveNavigationSnapshot& right)
{
    return left.known == right.known && left.canOpenPrevious == right.canOpenPrevious
        && left.canOpenNext == right.canOpenNext && left.atKnownFirst == right.atKnownFirst
        && left.atKnownLast == right.atKnownLast && left.currentNumber == right.currentNumber
        && left.count == right.count;
}
}

namespace kiriview {
DocumentSessionImageDocumentSyncPlan documentSessionImageDocumentSyncPlan(
    const DocumentSessionImageDocumentSyncInput& input)
{
    if (input.routingSource || input.documentKind != DocumentSessionKind::Image) {
        return {};
    }

    DocumentSessionImageDocumentSyncPlan plan;
    plan.active = true;
    plan.setSourceIdentity = true;
    plan.sourceIdentityUrl = input.image.sourceUrl;
    plan.syncFileDeletionProgress = !input.directImageLoadMayUseImageDocumentSourceScope;
    plan.fileDeletionInProgress = input.image.fileDeletionInProgress;

    if (input.directMediaScopeChanged || !input.directMediaNavigationActive) {
        plan.directMediaOperation
            = DocumentSessionImageDocumentSyncDirectMediaOperation::RefreshDirectMediaNavigation;
    } else if (input.directMediaNavigationKnown) {
        plan.directMediaOperation = DocumentSessionImageDocumentSyncDirectMediaOperation::
            CacheDisplayedMediaPredecodeImages;
    }

    plan.projectionOperation
        = sameActiveNavigationSnapshot(input.previousPageNavigation, input.image.pageNavigation)
        ? DocumentSessionImageDocumentSyncProjectionOperation::RecomputePublicProjection
        : DocumentSessionImageDocumentSyncProjectionOperation::PublishImagePageActiveNavigation;

    return plan;
}
}
