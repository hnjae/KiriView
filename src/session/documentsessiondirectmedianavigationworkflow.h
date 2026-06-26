// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONWORKFLOW_H
#define KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONWORKFLOW_H

#include "session/activenavigationprojection.h"
#include "session/documentsessiondirectmedianavigationruntime.h"

#include <QUrl>
#include <optional>
#include <vector>

namespace kiriview {
enum class DocumentSessionDirectMediaNavigationRevealAction {
    None,
    Clear,
    ProgrammaticSync,
    UsePendingOrProgrammaticSync,
    UsePendingOrProgrammaticSyncAndKeepPending,
};

struct DocumentSessionDirectMediaNavigationRefreshApplication
{
    DirectMediaNavigationBoundaryState boundaryState;
    bool known = false;
    std::vector<DirectMediaNavigationCandidate> candidates;
    DocumentSessionDirectMediaNavigationRevealAction revealAction
        = DocumentSessionDirectMediaNavigationRevealAction::None;
    bool schedulePredecode = false;
};

struct DocumentSessionDirectMediaNavigationOpenApplication
{
    DirectMediaNavigationBoundaryState boundaryState;
    bool known = false;
    std::vector<DirectMediaNavigationCandidate> candidates;
    DocumentSessionDirectMediaNavigationRevealAction revealAction
        = DocumentSessionDirectMediaNavigationRevealAction::None;
    bool schedulePredecode = false;
    std::optional<QUrl> routeTargetUrl;
};

DocumentSessionDirectMediaNavigationRefreshApplication
documentSessionDirectMediaNavigationRefreshApplication(ActiveNavigationSourceKind sourceKind,
    ActiveNavigationSnapshot previousSnapshot,
    DocumentSessionDirectMediaNavigationRefreshResult result);

DocumentSessionDirectMediaNavigationOpenApplication
documentSessionDirectMediaNavigationOpenApplication(
    const QUrl& activeDirectMediaCursorUrl, DocumentSessionDirectMediaNavigationOpenResult result);
}

#endif
