// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionmediadeletionplan.h"

#include <optional>
#include <utility>

namespace {
void appendDeletedMediaCandidate(
    std::vector<KiriView::MediaNavigationCandidate> *candidates, const QUrl &currentUrl)
{
    candidates->push_back(KiriView::MediaNavigationCandidate { currentUrl, currentUrl.fileName() });
}

KiriView::DocumentSessionMediaDeletionDocumentClear clearDocumentForKind(
    KiriView::DocumentSessionKind kind)
{
    switch (kind) {
    case KiriView::DocumentSessionKind::Image:
        return KiriView::DocumentSessionMediaDeletionDocumentClear::Image;
    case KiriView::DocumentSessionKind::Video:
        return KiriView::DocumentSessionMediaDeletionDocumentClear::Video;
    case KiriView::DocumentSessionKind::Empty:
        return KiriView::DocumentSessionMediaDeletionDocumentClear::None;
    }

    return KiriView::DocumentSessionMediaDeletionDocumentClear::None;
}

std::optional<QUrl> preferredMediaDeletionFallback(
    const KiriView::DocumentSessionMediaDeletionFallbackPlan &fallbackPlan)
{
    if (fallbackPlan.preferredFallbackUrl.has_value()) {
        return fallbackPlan.preferredFallbackUrl;
    }

    return fallbackPlan.fallbackUrl;
}
}

namespace KiriView {
DocumentSessionMediaDeletionStartPlan documentSessionMediaDeletionStartPlan(
    FileDeletionMode mode, std::vector<MediaNavigationCandidate> candidates, const QUrl &currentUrl)
{
    const DocumentSessionMediaDeletionFallbackPlan fallbackPlan
        = documentSessionMediaDeletionFallbackPlan(std::move(candidates), currentUrl);
    if (!fallbackPlan.hasTarget()) {
        return {};
    }

    return DocumentSessionMediaDeletionStartPlan {
        true,
        FileDeletionRequest { fallbackPlan.targetUrl, mode },
        fallbackPlan,
    };
}

DocumentSessionMediaDeletionFallbackPlan documentSessionMediaDeletionFallbackPlan(
    std::vector<MediaNavigationCandidate> candidates, const QUrl &currentUrl)
{
    const QUrl identityUrl = mediaNavigationSourceUrl(currentUrl);
    if (identityUrl.isEmpty()) {
        return {};
    }

    if (!mediaNavigationCandidateIndex(candidates, identityUrl).has_value()) {
        appendDeletedMediaCandidate(&candidates, identityUrl);
        sortMediaNavigationCandidates(&candidates);
    }

    return DocumentSessionMediaDeletionFallbackPlan {
        identityUrl,
        adjacentMediaNavigationUrl(candidates, identityUrl, NavigationDirection::Next),
        adjacentMediaNavigationUrl(candidates, identityUrl, NavigationDirection::Previous),
    };
}

DocumentSessionMediaDeletionCompletionPlan documentSessionMediaDeletionCompletionPlan(
    DocumentSessionKind currentKind, const DocumentSessionMediaDeletionFallbackPlan &fallbackPlan,
    FileDeletionResult result)
{
    switch (fileDeletionCompletionAction(result)) {
    case FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback: {
        DocumentSessionMediaDeletionCompletionPlan plan;
        plan.clearDocument = clearDocumentForKind(currentKind);

        const std::optional<QUrl> fallbackUrl = preferredMediaDeletionFallback(fallbackPlan);
        if (fallbackUrl.has_value()) {
            plan.followUp = DocumentSessionMediaDeletionFollowUp::OpenFallback;
            plan.fallbackUrl = *fallbackUrl;
            return plan;
        }

        plan.followUp = DocumentSessionMediaDeletionFollowUp::ClearSession;
        plan.clearMediaNavigation = true;
        plan.clearPredecode = true;
        return plan;
    }
    case FileDeletionCompletionAction::Ignore:
        return {};
    case FileDeletionCompletionAction::ReportFailure:
        return DocumentSessionMediaDeletionCompletionPlan {
            DocumentSessionMediaDeletionDocumentClear::None,
            DocumentSessionMediaDeletionFollowUp::ReportFailure,
            {},
            false,
            false,
        };
    }

    return DocumentSessionMediaDeletionCompletionPlan {
        DocumentSessionMediaDeletionDocumentClear::None,
        DocumentSessionMediaDeletionFollowUp::ReportFailure,
        {},
        false,
        false,
    };
}
}
