// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionrouteplan.h"

#include "navigation/mediaformatregistry.h"

namespace {
void prependOperations(kiriview::DocumentSessionRoutePlan &plan,
    std::vector<kiriview::DocumentSessionRouteOperation> operations)
{
    plan.operations.insert(plan.operations.begin(), operations.cbegin(), operations.cend());
}

void appendSourceRoutingPreparation(kiriview::DocumentSessionRoutePlan &plan)
{
    prependOperations(plan,
        std::vector<kiriview::DocumentSessionRouteOperation> {
            kiriview::DocumentSessionRouteOperation {
                kiriview::ClearSessionErrorStringRouteOperation {} },
            kiriview::DocumentSessionRouteOperation {
                kiriview::CancelDirectMediaNavigationRouteOperation {} },
            kiriview::DocumentSessionRouteOperation {
                kiriview::CancelMediaDeletionRouteOperation {} },
            kiriview::DocumentSessionRouteOperation {
                kiriview::ClearDirectMediaNavigationRouteOperation {} },
        });
}

kiriview::DocumentSessionKind kindAfterDeletedDocumentClear(
    kiriview::DocumentSessionKind deletedKind)
{
    switch (deletedKind) {
    case kiriview::DocumentSessionKind::Image:
        return kiriview::DocumentSessionKind::Image;
    case kiriview::DocumentSessionKind::Video:
    case kiriview::DocumentSessionKind::Empty:
        return kiriview::DocumentSessionKind::Empty;
    }

    return kiriview::DocumentSessionKind::Empty;
}

std::vector<kiriview::DocumentSessionRouteOperation> clearDeletedDocumentOperations(
    kiriview::DocumentSessionKind deletedKind)
{
    switch (deletedKind) {
    case kiriview::DocumentSessionKind::Image:
        return { kiriview::DocumentSessionRouteOperation {
            kiriview::ClearImageDocumentRouteOperation {} } };
    case kiriview::DocumentSessionKind::Video:
        return {
            kiriview::DocumentSessionRouteOperation { kiriview::LeaveVideoModeRouteOperation {} },
            kiriview::DocumentSessionRouteOperation {
                kiriview::EnterEmptyDocumentRouteOperation {} },
        };
    case kiriview::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

void appendDirectImageCursorOperation(
    kiriview::DocumentSessionRoutePlan &plan, kiriview::DocumentSessionKind currentKind)
{
    if (currentKind == kiriview::DocumentSessionKind::Image) {
        plan.operations.push_back(
            kiriview::RequestDirectImageCursorRouteOperation { plan.sourceUrl });
        return;
    }

    plan.operations.push_back(
        kiriview::ClearThenRequestDirectImageCursorRouteOperation { plan.sourceUrl });
}

void appendEnterImageDocumentOperations(kiriview::DocumentSessionRoutePlan &plan)
{
    plan.operations.push_back(kiriview::LeaveVideoModeRouteOperation {});
    plan.operations.push_back(kiriview::EnterImageDocumentRouteOperation { plan.sourceUrl });
}

void appendRefreshableImageDocumentOperations(kiriview::DocumentSessionRoutePlan &plan)
{
    appendEnterImageDocumentOperations(plan);
    plan.operations.push_back(kiriview::SyncDirectImageCursorFromDocumentRouteOperation {});
    plan.operations.push_back(kiriview::UseImageDocumentSourceIdentityRouteOperation {});
    plan.operations.push_back(kiriview::RecomputePublicProjectionRouteOperation {});
    plan.operations.push_back(kiriview::RefreshDirectMediaNavigationAfterRoutingRouteOperation {});
}

kiriview::DocumentSessionRoutePlan emptyRoutePlan(const QUrl &sourceUrl)
{
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::Empty;
    plan.sourceUrl = sourceUrl;
    plan.operations.push_back(kiriview::ClearDirectMediaCursorRouteOperation {});
    plan.operations.push_back(kiriview::LeaveVideoModeRouteOperation {});
    plan.operations.push_back(kiriview::ClearImageDocumentRouteOperation {});
    plan.operations.push_back(kiriview::EnterEmptyDocumentRouteOperation {});
    plan.operations.push_back(kiriview::ClearSourceIdentityRouteOperation {});
    plan.operations.push_back(kiriview::RecomputePublicProjectionRouteOperation {});
    plan.operations.push_back(kiriview::ClearMediaPredecodeRouteOperation {});
    return plan;
}

kiriview::DocumentSessionRoutePlan directVideoRoutePlan(const QUrl &sourceUrl)
{
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::DirectVideo;
    plan.sourceUrl = sourceUrl;
    plan.operations.push_back(kiriview::SetDirectVideoCursorRouteOperation { sourceUrl });
    plan.operations.push_back(kiriview::ClearImageDocumentRouteOperation {});
    plan.operations.push_back(kiriview::EnterVideoDocumentRouteOperation { sourceUrl });
    plan.operations.push_back(kiriview::UseOriginalSourceIdentityRouteOperation { sourceUrl });
    plan.operations.push_back(kiriview::RecomputePublicProjectionRouteOperation {});
    plan.operations.push_back(kiriview::RefreshDirectMediaNavigationAfterRoutingRouteOperation {});
    return plan;
}

kiriview::DocumentSessionRoutePlan directImageRoutePlan(
    const QUrl &sourceUrl, kiriview::DocumentSessionKind currentKind)
{
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::DirectImage;
    plan.sourceUrl = sourceUrl;
    appendDirectImageCursorOperation(plan, currentKind);
    appendRefreshableImageDocumentOperations(plan);
    return plan;
}

kiriview::DocumentSessionRoutePlan imageDocumentRoutePlan(const QUrl &sourceUrl)
{
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::ImageDocument;
    plan.sourceUrl = sourceUrl;
    plan.operations.push_back(kiriview::ClearDirectMediaCursorRouteOperation {});
    appendRefreshableImageDocumentOperations(plan);
    return plan;
}

kiriview::DocumentSessionRoutePlan baseRoutePlan(
    const QUrl &sourceUrl, kiriview::DocumentSessionKind currentKind)
{
    if (sourceUrl.isEmpty()) {
        return emptyRoutePlan(sourceUrl);
    }

    if (kiriview::isSupportedDirectVideoUrl(sourceUrl)) {
        return directVideoRoutePlan(sourceUrl);
    }

    if (kiriview::isSupportedDirectImageUrl(sourceUrl)) {
        return directImageRoutePlan(sourceUrl, currentKind);
    }

    return imageDocumentRoutePlan(sourceUrl);
}
}

namespace kiriview {
DocumentSessionRoutePlan documentSessionRoutePlanForSourceUrl(
    const QUrl &sourceUrl, DocumentSessionKind currentKind)
{
    DocumentSessionRoutePlan plan = baseRoutePlan(sourceUrl, currentKind);
    appendSourceRoutingPreparation(plan);
    return plan;
}

DocumentSessionRoutePlan documentSessionRoutePlanForMediaUrl(
    const QUrl &url, DocumentSessionKind currentKind)
{
    return baseRoutePlan(url, currentKind);
}

DocumentSessionRoutePlan documentSessionRoutePlanAfterMediaDeletion(
    DocumentSessionKind deletedKind, std::optional<QUrl> fallbackUrl)
{
    if (!fallbackUrl.has_value()) {
        DocumentSessionRoutePlan plan = emptyRoutePlan(QUrl());
        prependOperations(plan,
            std::vector<DocumentSessionRouteOperation> {
                DocumentSessionRouteOperation { ClearDirectMediaNavigationRouteOperation {} },
            });
        return plan;
    }

    DocumentSessionRoutePlan plan
        = baseRoutePlan(*fallbackUrl, kindAfterDeletedDocumentClear(deletedKind));
    prependOperations(plan, clearDeletedDocumentOperations(deletedKind));
    return plan;
}
}
