// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionrouteplan.h"

#include "navigation/mediaformatregistry.h"

namespace {
KiriView::DocumentSessionRouteCursorAction directImageCursorActionFor(
    KiriView::DocumentSessionKind currentKind)
{
    return currentKind == KiriView::DocumentSessionKind::Image
        ? KiriView::DocumentSessionRouteCursorAction::RequestDirectImage
        : KiriView::DocumentSessionRouteCursorAction::ClearThenRequestDirectImage;
}

KiriView::DocumentSessionRoutePlan baseRoutePlan(
    const QUrl &sourceUrl, KiriView::DocumentSessionKind currentKind)
{
    if (sourceUrl.isEmpty()) {
        KiriView::DocumentSessionRoutePlan plan;
        plan.kind = KiriView::DocumentSessionRouteKind::Empty;
        plan.sourceUrl = sourceUrl;
        plan.cursorAction = KiriView::DocumentSessionRouteCursorAction::Clear;
        plan.sourceIdentityAction = KiriView::DocumentSessionRouteSourceIdentityAction::Clear;
        plan.document.clear = KiriView::DocumentSessionRouteDocumentClear::ImageAndVideo;
        plan.document.enter = KiriView::DocumentSessionRouteDocumentEnter::Empty;
        plan.predecode.clear = true;
        return plan;
    }

    if (KiriView::isSupportedDirectVideoUrl(sourceUrl)) {
        KiriView::DocumentSessionRoutePlan plan;
        plan.kind = KiriView::DocumentSessionRouteKind::DirectVideo;
        plan.sourceUrl = sourceUrl;
        plan.cursorAction = KiriView::DocumentSessionRouteCursorAction::SetDirectVideo;
        plan.sourceIdentityAction
            = KiriView::DocumentSessionRouteSourceIdentityAction::UseOriginalUrl;
        plan.document.clear = KiriView::DocumentSessionRouteDocumentClear::Image;
        plan.document.enter = KiriView::DocumentSessionRouteDocumentEnter::Video;
        plan.mediaNavigation.refreshAfterRouting = true;
        return plan;
    }

    if (KiriView::isSupportedDirectImageUrl(sourceUrl)) {
        KiriView::DocumentSessionRoutePlan plan;
        plan.kind = KiriView::DocumentSessionRouteKind::DirectImage;
        plan.sourceUrl = sourceUrl;
        plan.cursorAction = directImageCursorActionFor(currentKind);
        plan.sourceIdentityAction
            = KiriView::DocumentSessionRouteSourceIdentityAction::UseImageDocumentSourceUrl;
        plan.document.clear = KiriView::DocumentSessionRouteDocumentClear::Video;
        plan.document.enter = KiriView::DocumentSessionRouteDocumentEnter::Image;
        plan.document.syncDirectImageCursorFromDocument = true;
        plan.mediaNavigation.refreshAfterRouting = true;
        return plan;
    }

    KiriView::DocumentSessionRoutePlan plan;
    plan.kind = KiriView::DocumentSessionRouteKind::ImageDocument;
    plan.sourceUrl = sourceUrl;
    plan.cursorAction = KiriView::DocumentSessionRouteCursorAction::Clear;
    plan.sourceIdentityAction
        = KiriView::DocumentSessionRouteSourceIdentityAction::UseImageDocumentSourceUrl;
    plan.document.clear = KiriView::DocumentSessionRouteDocumentClear::Video;
    plan.document.enter = KiriView::DocumentSessionRouteDocumentEnter::Image;
    plan.document.syncDirectImageCursorFromDocument = true;
    plan.mediaNavigation.refreshAfterRouting = true;
    return plan;
}
}

namespace KiriView {
DocumentSessionRoutePlan documentSessionRoutePlanForSourceUrl(
    const QUrl &sourceUrl, DocumentSessionKind currentKind)
{
    DocumentSessionRoutePlan plan = baseRoutePlan(sourceUrl, currentKind);
    plan.mediaNavigation.clearBeforeRouting = true;
    return plan;
}

DocumentSessionRoutePlan documentSessionRoutePlanForMediaUrl(
    const QUrl &url, DocumentSessionKind currentKind)
{
    return baseRoutePlan(url, currentKind);
}
}
