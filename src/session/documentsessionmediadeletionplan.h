// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONPLAN_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIADELETIONPLAN_H

#include "document/filedeletion.h"
#include "navigation/medianavigationmodel.h"
#include "session/documentsessiontypes.h"

#include <QUrl>
#include <optional>
#include <vector>

namespace KiriView {
enum class DocumentSessionMediaDeletionDocumentClear {
    None,
    Image,
    Video,
};

enum class DocumentSessionMediaDeletionFollowUp {
    None,
    OpenFallback,
    ClearSession,
    ReportFailure,
};

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
    DocumentSessionMediaDeletionDocumentClear clearDocument
        = DocumentSessionMediaDeletionDocumentClear::None;
    DocumentSessionMediaDeletionFollowUp followUp = DocumentSessionMediaDeletionFollowUp::None;
    QUrl fallbackUrl;
    bool clearMediaNavigation = false;
    bool clearPredecode = false;
};

DocumentSessionMediaDeletionStartPlan documentSessionMediaDeletionStartPlan(FileDeletionMode mode,
    std::vector<MediaNavigationCandidate> candidates, const QUrl &currentUrl);
DocumentSessionMediaDeletionFallbackPlan documentSessionMediaDeletionFallbackPlan(
    std::vector<MediaNavigationCandidate> candidates, const QUrl &currentUrl);
DocumentSessionMediaDeletionCompletionPlan documentSessionMediaDeletionCompletionPlan(
    DocumentSessionKind currentKind, const DocumentSessionMediaDeletionFallbackPlan &fallbackPlan,
    FileDeletionResult result);
}

#endif
