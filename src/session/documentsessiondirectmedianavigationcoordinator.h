// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONCOORDINATOR_H
#define KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONCOORDINATOR_H

#include "navigation/directmedianavigationcandidateprovider.h"
#include "session/activenavigationprojection.h"
#include "session/documentsessiondirectmedianavigationapplicationruntime.h"
#include "session/documentsessiondirectmedianavigationruntime.h"

#include <QUrl>
#include <functional>
#include <vector>

class QObject;

namespace kiriview {
struct DocumentSessionDirectMediaNavigationCoordinatorPorts
{
    std::function<bool()> navigationActive;
    std::function<bool()> directImageSourceScopeEligible;
    std::function<DirectMediaScope()> currentScope;
    std::function<bool(const DirectMediaScope&)> cursorMatches;
    std::function<QUrl()> activeCursorUrl;
    std::function<ActiveNavigationSourceKind()> activeNavigationSourceKind;
    std::function<ActiveNavigationSnapshot()> activeNavigationSnapshot;
    std::function<void(
        DirectMediaNavigationBoundaryState, bool, std::vector<DirectMediaNavigationCandidate>)>
        setDirectMediaNavigation;
    std::function<void(DocumentSessionDirectMediaNavigationRevealAction)> applyRevealAction;
    std::function<void()> recomputePublicProjection;
    std::function<void()> clearPredecode;
    std::function<void(const std::vector<DirectMediaNavigationCandidate>&)> schedulePredecode;
    std::function<void(const QUrl&)> openMediaUrl;
};

class DocumentSessionDirectMediaNavigationCoordinator final
{
public:
    DocumentSessionDirectMediaNavigationCoordinator(
        DirectMediaNavigationCandidateProvider provider = {},
        DocumentSessionDirectMediaNavigationCoordinatorPorts ports = {});
    ~DocumentSessionDirectMediaNavigationCoordinator();

    void cancel();
    void refresh(QObject* receiver);
    void openPrevious(QObject* receiver);
    void openNext(QObject* receiver);
    void openAtNumber(QObject* receiver, int mediaNumber);
    void open(QObject* receiver, DirectMediaNavigationOpenRequest request);

private:
    bool navigationActive() const;
    bool directImageSourceScopeEligible() const;
    DirectMediaScope currentScope() const;
    bool cursorMatches(const DirectMediaScope& scope) const;
    QUrl activeCursorUrl() const;
    ActiveNavigationSourceKind activeNavigationSourceKind() const;
    ActiveNavigationSnapshot activeNavigationSnapshot() const;

    DocumentSessionDirectMediaNavigationCoordinatorPorts m_ports;
    DocumentSessionDirectMediaNavigationRuntime m_navigationRuntime;
    DocumentSessionDirectMediaNavigationApplicationRuntime m_applicationRuntime;
};
}

#endif
