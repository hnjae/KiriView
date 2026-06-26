// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionrouteplan.h"

#include "navigation/mediaformatregistry.h"

namespace {
void prependMutations(kiriview::DocumentSessionRoutePlan& plan,
    std::vector<kiriview::DocumentSessionRouteMutation> mutations)
{
    plan.mutations.insert(plan.mutations.begin(), mutations.cbegin(), mutations.cend());
}

void appendSourceRoutingPreparation(kiriview::DocumentSessionRoutePlan& plan)
{
    prependMutations(plan,
        std::vector<kiriview::DocumentSessionRouteMutation> {
            kiriview::DocumentSessionRouteMutation {
                kiriview::ClearSessionErrorStringRouteOperation {} },
            kiriview::DocumentSessionRouteMutation {
                kiriview::CancelDirectMediaNavigationRouteOperation {} },
            kiriview::DocumentSessionRouteMutation {
                kiriview::CancelMediaDeletionRouteOperation {} },
            kiriview::DocumentSessionRouteMutation {
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

std::vector<kiriview::DocumentSessionRouteMutation> clearDeletedDocumentMutations(
    kiriview::DocumentSessionKind deletedKind)
{
    switch (deletedKind) {
    case kiriview::DocumentSessionKind::Image:
        return { kiriview::DocumentSessionRouteMutation {
            kiriview::ClearImageDocumentRouteOperation {} } };
    case kiriview::DocumentSessionKind::Video:
        return {
            kiriview::DocumentSessionRouteMutation { kiriview::LeaveVideoModeRouteOperation {} },
            kiriview::DocumentSessionRouteMutation {
                kiriview::EnterEmptyDocumentRouteOperation {} },
        };
    case kiriview::DocumentSessionKind::Empty:
        return {};
    }

    return {};
}

void appendDirectImageCursorOperation(
    kiriview::DocumentSessionRoutePlan& plan, kiriview::DocumentSessionKind currentKind)
{
    if (currentKind == kiriview::DocumentSessionKind::Image) {
        plan.mutations.push_back(
            kiriview::RequestDirectImageCursorRouteOperation { plan.sourceUrl });
        return;
    }

    plan.mutations.push_back(
        kiriview::ClearThenRequestDirectImageCursorRouteOperation { plan.sourceUrl });
}

void appendEnterImageDocumentMutations(kiriview::DocumentSessionRoutePlan& plan)
{
    plan.mutations.push_back(kiriview::LeaveVideoModeRouteOperation {});
    plan.mutations.push_back(kiriview::EnterImageDocumentRouteOperation { plan.sourceUrl });
}

void appendRefreshableImageDocumentRoute(kiriview::DocumentSessionRoutePlan& plan)
{
    appendEnterImageDocumentMutations(plan);
    plan.mutations.push_back(kiriview::SyncDirectImageCursorFromDocumentRouteOperation {});
    plan.mutations.push_back(kiriview::UseImageDocumentSourceIdentityRouteOperation {});
    plan.publishPublicProjection = true;
    plan.followUpEffects.push_back(
        kiriview::RefreshDirectMediaNavigationAfterRoutingRouteEffect {});
}

kiriview::DocumentSessionRoutePlan emptyRoutePlan(const QUrl& sourceUrl)
{
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::Empty;
    plan.sourceUrl = sourceUrl;
    plan.mutations.push_back(kiriview::ClearDirectMediaCursorRouteOperation {});
    plan.mutations.push_back(kiriview::LeaveVideoModeRouteOperation {});
    plan.mutations.push_back(kiriview::ClearImageDocumentRouteOperation {});
    plan.mutations.push_back(kiriview::EnterEmptyDocumentRouteOperation {});
    plan.mutations.push_back(kiriview::ClearSourceIdentityRouteOperation {});
    plan.publishPublicProjection = true;
    plan.followUpEffects.push_back(kiriview::ClearMediaPredecodeRouteEffect {});
    return plan;
}

kiriview::DocumentSessionRoutePlan directVideoRoutePlan(const QUrl& sourceUrl)
{
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::DirectVideo;
    plan.sourceUrl = sourceUrl;
    plan.mutations.push_back(kiriview::SetDirectVideoCursorRouteOperation { sourceUrl });
    plan.mutations.push_back(kiriview::ClearImageDocumentRouteOperation {});
    plan.mutations.push_back(kiriview::EnterVideoDocumentRouteOperation { sourceUrl });
    plan.mutations.push_back(kiriview::UseOriginalSourceIdentityRouteOperation { sourceUrl });
    plan.publishPublicProjection = true;
    plan.followUpEffects.push_back(
        kiriview::RefreshDirectMediaNavigationAfterRoutingRouteEffect {});
    return plan;
}

kiriview::DocumentSessionRoutePlan directImageRoutePlan(
    const QUrl& sourceUrl, kiriview::DocumentSessionKind currentKind)
{
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::DirectImage;
    plan.sourceUrl = sourceUrl;
    appendDirectImageCursorOperation(plan, currentKind);
    appendRefreshableImageDocumentRoute(plan);
    return plan;
}

kiriview::DocumentSessionRoutePlan imageDocumentRoutePlan(const QUrl& sourceUrl)
{
    kiriview::DocumentSessionRoutePlan plan;
    plan.kind = kiriview::DocumentSessionRouteKind::ImageDocument;
    plan.sourceUrl = sourceUrl;
    plan.mutations.push_back(kiriview::ClearDirectMediaCursorRouteOperation {});
    appendRefreshableImageDocumentRoute(plan);
    return plan;
}

kiriview::DocumentSessionRoutePlan baseRoutePlan(
    const QUrl& sourceUrl, kiriview::DocumentSessionKind currentKind)
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
    const QUrl& sourceUrl, DocumentSessionKind currentKind)
{
    DocumentSessionRoutePlan plan = baseRoutePlan(sourceUrl, currentKind);
    appendSourceRoutingPreparation(plan);
    return plan;
}

DocumentSessionRoutePlan documentSessionRoutePlanForMediaUrl(
    const QUrl& url, DocumentSessionKind currentKind)
{
    return baseRoutePlan(url, currentKind);
}

DocumentSessionRoutePlan documentSessionRoutePlanAfterMediaDeletion(
    DocumentSessionKind deletedKind, std::optional<QUrl> fallbackUrl)
{
    if (!fallbackUrl.has_value()) {
        DocumentSessionRoutePlan plan = emptyRoutePlan(QUrl());
        prependMutations(plan,
            std::vector<DocumentSessionRouteMutation> {
                DocumentSessionRouteMutation { ClearDirectMediaNavigationRouteOperation {} },
            });
        return plan;
    }

    DocumentSessionRoutePlan plan
        = baseRoutePlan(*fallbackUrl, kindAfterDeletedDocumentClear(deletedKind));
    prependMutations(plan, clearDeletedDocumentMutations(deletedKind));
    return plan;
}
}
