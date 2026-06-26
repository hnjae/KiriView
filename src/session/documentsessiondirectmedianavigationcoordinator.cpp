// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessiondirectmedianavigationcoordinator.h"

#include "navigation/navigationlogging.h"

#include <QDebug>
#include <utility>

namespace {
void logDirectMediaScope(const char* message, const kiriview::DirectMediaScope& scope)
{
    qCDebug(kiriviewNavigationLog) << message << "currentUrl" << scope.currentUrl << "parentUrl"
                                   << scope.parentUrl << "generation" << scope.generation;
}
}

namespace kiriview {
DocumentSessionDirectMediaNavigationCoordinator::DocumentSessionDirectMediaNavigationCoordinator(
    DirectMediaNavigationCandidateProvider provider,
    DocumentSessionDirectMediaNavigationCoordinatorPorts ports)
    : m_ports(std::move(ports))
    , m_navigationRuntime(std::move(provider))
    , m_applicationRuntime(DocumentSessionDirectMediaNavigationApplicationPorts {
          [this](DirectMediaNavigationBoundaryState state, bool known,
              std::vector<DirectMediaNavigationCandidate> candidates) {
              if (m_ports.setDirectMediaNavigation) {
                  m_ports.setDirectMediaNavigation(state, known, std::move(candidates));
              }
          },
          [this](DocumentSessionDirectMediaNavigationRevealAction action) {
              if (m_ports.applyRevealAction) {
                  m_ports.applyRevealAction(action);
              }
          },
          [this]() {
              if (m_ports.recomputePublicProjection) {
                  m_ports.recomputePublicProjection();
              }
          },
          [this]() {
              if (m_ports.clearPredecode) {
                  m_ports.clearPredecode();
              }
          },
          [this](const std::vector<DirectMediaNavigationCandidate>& candidates) {
              if (m_ports.schedulePredecode) {
                  m_ports.schedulePredecode(candidates);
              }
          },
          [this](const QUrl& url) {
              if (m_ports.openMediaUrl) {
                  m_ports.openMediaUrl(url);
              }
          },
      })
{
}

DocumentSessionDirectMediaNavigationCoordinator::~DocumentSessionDirectMediaNavigationCoordinator()
{
    cancel();
}

void DocumentSessionDirectMediaNavigationCoordinator::cancel() { m_navigationRuntime.cancel(); }

void DocumentSessionDirectMediaNavigationCoordinator::refresh(QObject* receiver)
{
    if (!navigationActive()) {
        qCDebug(kiriviewNavigationLog) << "direct media navigation refresh skipped"
                                       << "reason"
                                       << "inactive"
                                       << "cursorUrl" << activeCursorUrl();
        m_applicationRuntime.applyInactiveRefresh(!directImageSourceScopeEligible());
        return;
    }

    const DirectMediaScope scope = currentScope();
    logDirectMediaScope("direct media navigation refresh requested", scope);
    m_navigationRuntime.refresh(
        receiver, scope,
        [this](const DirectMediaScope& acceptedScope) { return cursorMatches(acceptedScope); },
        [this](DocumentSessionDirectMediaNavigationRefreshResult result) {
            m_applicationRuntime.applyRefresh(
                activeNavigationSourceKind(), activeNavigationSnapshot(), std::move(result));
        });
}

void DocumentSessionDirectMediaNavigationCoordinator::openPrevious(QObject* receiver)
{
    open(receiver, previousDirectMediaNavigationOpenRequest());
}

void DocumentSessionDirectMediaNavigationCoordinator::openNext(QObject* receiver)
{
    open(receiver, nextDirectMediaNavigationOpenRequest());
}

void DocumentSessionDirectMediaNavigationCoordinator::openAtNumber(
    QObject* receiver, int mediaNumber)
{
    open(receiver, numberedDirectMediaNavigationOpenRequest(mediaNumber));
}

void DocumentSessionDirectMediaNavigationCoordinator::open(
    QObject* receiver, DirectMediaNavigationOpenRequest request)
{
    if (!navigationActive()) {
        return;
    }

    m_navigationRuntime.open(
        receiver, currentScope(), request,
        [this](const DirectMediaScope& acceptedScope) { return cursorMatches(acceptedScope); },
        [this](DocumentSessionDirectMediaNavigationOpenResult result) {
            m_applicationRuntime.applyOpen(activeCursorUrl(), std::move(result));
        });
}

bool DocumentSessionDirectMediaNavigationCoordinator::navigationActive() const
{
    return m_ports.navigationActive && m_ports.navigationActive();
}

bool DocumentSessionDirectMediaNavigationCoordinator::directImageSourceScopeEligible() const
{
    return m_ports.directImageSourceScopeEligible && m_ports.directImageSourceScopeEligible();
}

DirectMediaScope DocumentSessionDirectMediaNavigationCoordinator::currentScope() const
{
    if (m_ports.currentScope) {
        return m_ports.currentScope();
    }

    return {};
}

bool DocumentSessionDirectMediaNavigationCoordinator::cursorMatches(
    const DirectMediaScope& scope) const
{
    return !m_ports.cursorMatches || m_ports.cursorMatches(scope);
}

QUrl DocumentSessionDirectMediaNavigationCoordinator::activeCursorUrl() const
{
    if (m_ports.activeCursorUrl) {
        return m_ports.activeCursorUrl();
    }

    return {};
}

ActiveNavigationSourceKind
DocumentSessionDirectMediaNavigationCoordinator::activeNavigationSourceKind() const
{
    if (m_ports.activeNavigationSourceKind) {
        return m_ports.activeNavigationSourceKind();
    }

    return ActiveNavigationSourceKind::None;
}

ActiveNavigationSnapshot
DocumentSessionDirectMediaNavigationCoordinator::activeNavigationSnapshot() const
{
    if (m_ports.activeNavigationSnapshot) {
        return m_ports.activeNavigationSnapshot();
    }

    return {};
}
}
