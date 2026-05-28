// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONPLAN_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONPLAN_H

#include "document/filedeletion.h"
#include "navigation/directmedianavigationmodel.h"
#include "session/documentsessionrouteplan.h"
#include "session/documentsessiontypes.h"

#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
struct DocumentSessionMediaDeletionFallbackPlan {
    QUrl targetUrl;
    std::optional<QUrl> preferredFallbackUrl;
    std::optional<QUrl> fallbackUrl;

    bool hasTarget() const { return !targetUrl.isEmpty(); }
};

struct DocumentSessionMediaDeletionStartPlan {
    bool shouldStartDeletion = false;
    FileDeletionRequest request;
    DocumentSessionMediaDeletionFallbackPlan fallbackPlan;
};

struct DocumentSessionMediaDeletionCompletionPlan {
    DocumentSessionRoutePlan routePlan;
    bool reportFailure = false;

    bool hasRoutePlan() const { return !routePlan.operations.empty(); }
};

DocumentSessionMediaDeletionStartPlan documentSessionMediaDeletionStartPlan(FileDeletionMode mode,
    std::vector<DirectMediaNavigationCandidate> candidates, const QUrl &currentUrl);
DocumentSessionMediaDeletionFallbackPlan documentSessionMediaDeletionFallbackPlan(
    std::vector<DirectMediaNavigationCandidate> candidates, const QUrl &currentUrl);
DocumentSessionMediaDeletionCompletionPlan documentSessionMediaDeletionCompletionPlan(
    DocumentSessionKind currentKind, const DocumentSessionMediaDeletionFallbackPlan &fallbackPlan,
    FileDeletionResult result);
}

#endif
