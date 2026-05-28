// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionrouteplan.h"

#include "navigation/mediaformatregistry.h"

namespace {
void prependOperations(KiriView::DocumentSessionRoutePlan &plan,
    std::vector<KiriView::DocumentSessionRouteOperation> operations)
{
    plan.operations.insert(plan.operations.begin(), operations.cbegin(), operations.cend());
}

void appendSourceRoutingPreparation(KiriView::DocumentSessionRoutePlan &plan)
{
    prependOperations(plan,
        std::vector<KiriView::DocumentSessionRouteOperation> {
            KiriView::DocumentSessionRouteOperation {
                KiriView::ClearSessionErrorStringRouteOperation {} },
            KiriView::DocumentSessionRouteOperation {
                KiriView::CancelDirectMediaNavigationRouteOperation {} },
            KiriView::DocumentSessionRouteOperation {
                KiriView::CancelMediaDeletionRouteOperation {} },
            KiriView::DocumentSessionRouteOperation {
                KiriView::ClearDirectMediaNavigationRouteOperation {} },
        });
}

KiriView::DocumentSessionKind kindAfterDeletedDocumentClear(
    KiriView::DocumentSessionKind deletedKind)
{
    switch (deletedKind) {
    case KiriView::DocumentSessionKind::Image:
        return KiriView::DocumentSessionKind::Image;
    case KiriView::DocumentSessionKind::Video:
    case KiriView::DocumentSessionKind::Empty:
        return KiriView::DocumentSessionKind::Empty;
    }

    return KiriView::DocumentSessionKind::Empty;
}

std::vector<KiriView::DocumentSessionRouteOperation> clearDeletedDocumentOperations(
    KiriView::DocumentSessionKind deletedKind)
{
    switch (deletedKind) {
    case KiriView::DocumentSessionKind::Image:
        return { KiriView::DocumentSessionRouteOperation {
            KiriView::ClearImageDocumentRouteOperation {} } };
    case KiriView::DocumentSessionKind::Video:
        return {
            KiriView::DocumentSessionRouteOperation { KiriView::LeaveVideoModeRouteOperation {} },
            KiriView::DocumentSessionRouteOperation {
                KiriView::EnterEmptyDocumentRouteOperation {} },
        };
    case KiriView::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

void appendDirectImageCursorOperation(
    KiriView::DocumentSessionRoutePlan &plan, KiriView::DocumentSessionKind currentKind)
{
    if (currentKind == KiriView::DocumentSessionKind::Image) {
        plan.operations.push_back(
            KiriView::RequestDirectImageCursorRouteOperation { plan.sourceUrl });
        return;
    }

    plan.operations.push_back(
        KiriView::ClearThenRequestDirectImageCursorRouteOperation { plan.sourceUrl });
}

void appendEnterImageDocumentOperations(KiriView::DocumentSessionRoutePlan &plan)
{
    plan.operations.push_back(KiriView::LeaveVideoModeRouteOperation {});
    plan.operations.push_back(KiriView::EnterImageDocumentRouteOperation { plan.sourceUrl });
}

void appendRefreshableImageDocumentOperations(KiriView::DocumentSessionRoutePlan &plan)
{
    appendEnterImageDocumentOperations(plan);
    plan.operations.push_back(KiriView::SyncDirectImageCursorFromDocumentRouteOperation {});
    plan.operations.push_back(KiriView::UseImageDocumentSourceIdentityRouteOperation {});
    plan.operations.push_back(KiriView::RecomputePublicProjectionRouteOperation {});
    plan.operations.push_back(KiriView::RefreshDirectMediaNavigationAfterRoutingRouteOperation {});
}

KiriView::DocumentSessionRoutePlan emptyRoutePlan(const QUrl &sourceUrl)
{
    KiriView::DocumentSessionRoutePlan plan;
    plan.kind = KiriView::DocumentSessionRouteKind::Empty;
    plan.sourceUrl = sourceUrl;
    plan.operations.push_back(KiriView::ClearDirectMediaCursorRouteOperation {});
    plan.operations.push_back(KiriView::LeaveVideoModeRouteOperation {});
    plan.operations.push_back(KiriView::ClearImageDocumentRouteOperation {});
    plan.operations.push_back(KiriView::EnterEmptyDocumentRouteOperation {});
    plan.operations.push_back(KiriView::ClearSourceIdentityRouteOperation {});
    plan.operations.push_back(KiriView::RecomputePublicProjectionRouteOperation {});
    plan.operations.push_back(KiriView::ClearMediaPredecodeRouteOperation {});
    return plan;
}

KiriView::DocumentSessionRoutePlan directVideoRoutePlan(const QUrl &sourceUrl)
{
    KiriView::DocumentSessionRoutePlan plan;
    plan.kind = KiriView::DocumentSessionRouteKind::DirectVideo;
    plan.sourceUrl = sourceUrl;
    plan.operations.push_back(KiriView::SetDirectVideoCursorRouteOperation { sourceUrl });
    plan.operations.push_back(KiriView::ClearImageDocumentRouteOperation {});
    plan.operations.push_back(KiriView::EnterVideoDocumentRouteOperation { sourceUrl });
    plan.operations.push_back(KiriView::UseOriginalSourceIdentityRouteOperation { sourceUrl });
    plan.operations.push_back(KiriView::RecomputePublicProjectionRouteOperation {});
    plan.operations.push_back(KiriView::RefreshDirectMediaNavigationAfterRoutingRouteOperation {});
    return plan;
}

KiriView::DocumentSessionRoutePlan directImageRoutePlan(
    const QUrl &sourceUrl, KiriView::DocumentSessionKind currentKind)
{
    KiriView::DocumentSessionRoutePlan plan;
    plan.kind = KiriView::DocumentSessionRouteKind::DirectImage;
    plan.sourceUrl = sourceUrl;
    appendDirectImageCursorOperation(plan, currentKind);
    appendRefreshableImageDocumentOperations(plan);
    return plan;
}

KiriView::DocumentSessionRoutePlan imageDocumentRoutePlan(const QUrl &sourceUrl)
{
    KiriView::DocumentSessionRoutePlan plan;
    plan.kind = KiriView::DocumentSessionRouteKind::ImageDocument;
    plan.sourceUrl = sourceUrl;
    plan.operations.push_back(KiriView::ClearDirectMediaCursorRouteOperation {});
    appendRefreshableImageDocumentOperations(plan);
    return plan;
}

KiriView::DocumentSessionRoutePlan baseRoutePlan(
    const QUrl &sourceUrl, KiriView::DocumentSessionKind currentKind)
{
    if (sourceUrl.isEmpty()) {
        return emptyRoutePlan(sourceUrl);
    }

    if (KiriView::isSupportedDirectVideoUrl(sourceUrl)) {
        return directVideoRoutePlan(sourceUrl);
    }

    if (KiriView::isSupportedDirectImageUrl(sourceUrl)) {
        return directImageRoutePlan(sourceUrl, currentKind);
    }

    return imageDocumentRoutePlan(sourceUrl);
}
}

namespace KiriView {
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
