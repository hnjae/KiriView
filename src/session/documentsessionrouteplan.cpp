// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionrouteplan.h"

#include "navigation/mediaformatregistry.h"

namespace {
void appendSourceRoutingPreparation(KiriView::DocumentSessionRoutePlan &plan)
{
    plan.operations.insert(plan.operations.begin(),
        {
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
}
