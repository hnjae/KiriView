// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionrouteplan.h"

#include "decoding/imageformatregistry.h"
#include "navigation/mediaformatregistry.h"

#include <QString>

namespace {
template <typename Predicate> bool matchesUrlFileNameOrString(const QUrl &url, Predicate predicate)
{
    const QString fileName = url.fileName(QUrl::PrettyDecoded);
    if (predicate(fileName)) {
        return true;
    }

    return predicate(url.toString(QUrl::PrettyDecoded));
}

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
        plan.clearImageDocument = true;
        plan.clearVideo = true;
        plan.enterEmpty = true;
        plan.clearPredecode = true;
        return plan;
    }

    if (KiriView::isDocumentSessionDirectVideoUrl(sourceUrl)) {
        KiriView::DocumentSessionRoutePlan plan;
        plan.kind = KiriView::DocumentSessionRouteKind::DirectVideo;
        plan.sourceUrl = sourceUrl;
        plan.cursorAction = KiriView::DocumentSessionRouteCursorAction::SetDirectVideo;
        plan.sourceIdentityAction
            = KiriView::DocumentSessionRouteSourceIdentityAction::UseOriginalUrl;
        plan.clearImageDocument = true;
        plan.enterVideo = true;
        plan.refreshMediaNavigation = true;
        return plan;
    }

    if (KiriView::isDocumentSessionDirectImageUrl(sourceUrl)) {
        KiriView::DocumentSessionRoutePlan plan;
        plan.kind = KiriView::DocumentSessionRouteKind::DirectImage;
        plan.sourceUrl = sourceUrl;
        plan.cursorAction = directImageCursorActionFor(currentKind);
        plan.sourceIdentityAction
            = KiriView::DocumentSessionRouteSourceIdentityAction::UseImageDocumentSourceUrl;
        plan.clearVideo = true;
        plan.enterImage = true;
        plan.syncDirectImageCursorFromDocument = true;
        plan.refreshMediaNavigation = true;
        return plan;
    }

    KiriView::DocumentSessionRoutePlan plan;
    plan.kind = KiriView::DocumentSessionRouteKind::ImageDocument;
    plan.sourceUrl = sourceUrl;
    plan.cursorAction = KiriView::DocumentSessionRouteCursorAction::Clear;
    plan.sourceIdentityAction
        = KiriView::DocumentSessionRouteSourceIdentityAction::UseImageDocumentSourceUrl;
    plan.clearVideo = true;
    plan.enterImage = true;
    plan.syncDirectImageCursorFromDocument = true;
    plan.refreshMediaNavigation = true;
    return plan;
}
}

namespace KiriView {
bool isDocumentSessionDirectVideoUrl(const QUrl &url)
{
    return matchesUrlFileNameOrString(url, KiriView::isSupportedDirectVideoFileName);
}

bool isDocumentSessionDirectImageUrl(const QUrl &url)
{
    return matchesUrlFileNameOrString(url, KiriView::isSupportedImageFileName);
}

DocumentSessionRoutePlan documentSessionRoutePlanForSourceUrl(
    const QUrl &sourceUrl, DocumentSessionKind currentKind)
{
    DocumentSessionRoutePlan plan = baseRoutePlan(sourceUrl, currentKind);
    plan.clearMediaNavigation = true;
    return plan;
}

DocumentSessionRoutePlan documentSessionRoutePlanForMediaUrl(
    const QUrl &url, DocumentSessionKind currentKind)
{
    return baseRoutePlan(url, currentKind);
}
}
