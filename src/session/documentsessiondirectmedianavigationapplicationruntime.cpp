// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmedianavigationapplicationruntime.h"

#include "async/imagecallback.h"
#include "navigation/navigationlogging.h"

#include <QDebug>
#include <utility>

namespace kiriview {
DocumentSessionDirectMediaNavigationApplicationRuntime::
    DocumentSessionDirectMediaNavigationApplicationRuntime(
        DocumentSessionDirectMediaNavigationApplicationPorts ports)
    : m_ports(std::move(ports))
{
}

void DocumentSessionDirectMediaNavigationApplicationRuntime::applyInactiveRefresh(
    bool clearPredecode)
{
    invokeIfSet(m_ports.setDirectMediaNavigation, DirectMediaNavigationBoundaryState {}, false,
        std::vector<DirectMediaNavigationCandidate> {});
    invokeIfSet(m_ports.applyRevealAction,
        DocumentSessionDirectMediaNavigationRevealAction::ProgrammaticSync);
    invokeIfSet(m_ports.publishProjection);
    if (clearPredecode) {
        invokeIfSet(m_ports.clearPredecode);
    }
}

void DocumentSessionDirectMediaNavigationApplicationRuntime::applyRefresh(
    ActiveNavigationSourceKind sourceKind, const ActiveNavigationSnapshot& previousSnapshot,
    DocumentSessionDirectMediaNavigationRefreshResult result)
{
    const QString errorString = result.errorString;
    const DocumentSessionDirectMediaNavigationRefreshApplication application
        = documentSessionDirectMediaNavigationRefreshApplication(
            sourceKind, previousSnapshot, std::move(result));
    if (!application.known) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation refresh failed"
                                       << "error" << errorString;
    } else {
        qCDebug(kiriviewNavigationLog)
            << "direct media navigation refresh finished"
            << "candidates" << application.candidates.size() << "currentNumber"
            << application.boundaryState.currentNumber << "count" << application.boundaryState.count
            << "canPrevious" << application.boundaryState.canOpenPrevious << "canNext"
            << application.boundaryState.canOpenNext;
    }

    invokeIfSet(m_ports.setDirectMediaNavigation, application.boundaryState, application.known,
        application.candidates);
    invokeIfSet(m_ports.applyRevealAction, application.revealAction);
    invokeIfSet(m_ports.publishProjection);
    if (application.schedulePredecode) {
        invokeIfSet(m_ports.schedulePredecode, application.candidates);
    }
}

void DocumentSessionDirectMediaNavigationApplicationRuntime::applyOpen(
    const QUrl& activeDirectMediaCursorUrl, DocumentSessionDirectMediaNavigationOpenResult result)
{
    const QString errorString = result.errorString;
    const DocumentSessionDirectMediaNavigationOpenApplication application
        = documentSessionDirectMediaNavigationOpenApplication(
            activeDirectMediaCursorUrl, std::move(result));
    if (!application.known) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation open failed"
                                       << "error" << errorString;
    } else {
        qCDebug(kiriviewNavigationLog)
            << "direct media navigation open finished"
            << "candidates" << application.candidates.size() << "currentNumber"
            << application.boundaryState.currentNumber << "count" << application.boundaryState.count
            << "targetUrl" << application.routeTargetUrl.value_or(QUrl());
    }

    invokeIfSet(m_ports.setDirectMediaNavigation, application.boundaryState, application.known,
        application.candidates);
    invokeIfSet(m_ports.applyRevealAction, application.revealAction);
    invokeIfSet(m_ports.publishProjection);
    if (application.schedulePredecode) {
        invokeIfSet(m_ports.schedulePredecode, application.candidates);
    }
    if (application.routeTargetUrl.has_value()) {
        invokeIfSet(m_ports.routeMediaUrl, *application.routeTargetUrl);
    }
}
}
