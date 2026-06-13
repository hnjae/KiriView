// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionmediadeletionplan.h"

#include <optional>
#include <utility>

namespace {
void appendDeletedDirectMediaNavigationCandidate(
    std::vector<kiriview::DirectMediaNavigationCandidate> *candidates, const QUrl &currentUrl)
{
    candidates->push_back(
        kiriview::DirectMediaNavigationCandidate { currentUrl, currentUrl.fileName() });
}

std::optional<QUrl> preferredMediaDeletionFallback(
    const kiriview::DocumentSessionMediaDeletionFallbackPlan &fallbackPlan)
{
    if (fallbackPlan.preferredFallbackUrl.has_value()) {
        return fallbackPlan.preferredFallbackUrl;
    }

    return fallbackPlan.fallbackUrl;
}
}

namespace kiriview {
DocumentSessionMediaDeletionStartPlan documentSessionMediaDeletionStartPlan(FileDeletionMode mode,
    std::vector<DirectMediaNavigationCandidate> candidates, const QUrl &currentUrl)
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
    std::vector<DirectMediaNavigationCandidate> candidates, const QUrl &currentUrl)
{
    const QUrl identityUrl = directMediaNavigationSourceUrl(currentUrl);
    if (identityUrl.isEmpty()) {
        return {};
    }

    if (!directMediaNavigationCandidateIndex(candidates, identityUrl).has_value()) {
        appendDeletedDirectMediaNavigationCandidate(&candidates, identityUrl);
        sortDirectMediaNavigationCandidates(&candidates);
    }

    return DocumentSessionMediaDeletionFallbackPlan {
        identityUrl,
        adjacentDirectMediaNavigationUrl(candidates, identityUrl, NavigationDirection::Next),
        adjacentDirectMediaNavigationUrl(candidates, identityUrl, NavigationDirection::Previous),
    };
}

DocumentSessionMediaDeletionCompletionPlan documentSessionMediaDeletionCompletionPlan(
    DocumentSessionKind currentKind, const DocumentSessionMediaDeletionFallbackPlan &fallbackPlan,
    FileDeletionResult result)
{
    switch (fileDeletionCompletionAction(result)) {
    case FileDeletionCompletionAction::ClearDeletedTargetAndOpenFallback: {
        return DocumentSessionMediaDeletionCompletionPlan {
            documentSessionRoutePlanAfterMediaDeletion(
                currentKind, preferredMediaDeletionFallback(fallbackPlan)),
            false,
        };
    }
    case FileDeletionCompletionAction::Ignore:
        return {};
    case FileDeletionCompletionAction::ReportFailure:
        return DocumentSessionMediaDeletionCompletionPlan { {}, true };
    }

    return DocumentSessionMediaDeletionCompletionPlan { {}, true };
}
}
