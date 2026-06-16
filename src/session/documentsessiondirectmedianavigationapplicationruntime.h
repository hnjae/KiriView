// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONAPPLICATIONRUNTIME_H
#define KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONAPPLICATIONRUNTIME_H

#include "session/documentsessiondirectmedianavigationworkflow.h"

#include <QUrl>
#include <functional>
#include <vector>

namespace kiriview {
struct DocumentSessionDirectMediaNavigationApplicationPorts {
    std::function<void(
        DirectMediaNavigationBoundaryState, bool, std::vector<DirectMediaNavigationCandidate>)>
        setDirectMediaNavigation;
    std::function<void(DocumentSessionDirectMediaNavigationRevealAction)> applyRevealAction;
    std::function<void()> publishProjection;
    std::function<void()> clearPredecode;
    std::function<void(const std::vector<DirectMediaNavigationCandidate> &)> schedulePredecode;
    std::function<void(const QUrl &)> routeMediaUrl;
};

class DocumentSessionDirectMediaNavigationApplicationRuntime final
{
public:
    explicit DocumentSessionDirectMediaNavigationApplicationRuntime(
        DocumentSessionDirectMediaNavigationApplicationPorts ports);

    void applyInactiveRefresh(bool clearPredecode);
    void applyRefresh(ActiveNavigationSourceKind sourceKind,
        const ActiveNavigationSnapshot &previousSnapshot,
        DocumentSessionDirectMediaNavigationRefreshResult result);
    void applyOpen(const QUrl &activeDirectMediaCursorUrl,
        DocumentSessionDirectMediaNavigationOpenResult result);

private:
    DocumentSessionDirectMediaNavigationApplicationPorts m_ports;
};
}

#endif
