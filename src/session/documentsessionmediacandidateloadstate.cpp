// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionmediacandidateloadstate.h"

namespace KiriView {
DocumentSessionMediaCandidateLoadState::DocumentSessionMediaCandidateLoadState(
    quint64 nextOperationId)
    : m_operation(nextOperationId)
{
}

DocumentSessionMediaCandidateLoad DocumentSessionMediaCandidateLoadState::start(
    const QUrl &currentUrl)
{
    m_currentUrl = currentUrl;
    return DocumentSessionMediaCandidateLoad {
        m_operation.start(),
        currentUrl,
    };
}

bool DocumentSessionMediaCandidateLoadState::accepts(
    const DocumentSessionMediaCandidateLoad &load) const
{
    return m_operation.accepts(load.operationId) && m_currentUrl == load.currentUrl;
}

bool DocumentSessionMediaCandidateLoadState::finish(const DocumentSessionMediaCandidateLoad &load)
{
    if (!accepts(load)) {
        return false;
    }

    m_currentUrl = QUrl();
    return m_operation.finish(load.operationId);
}

void DocumentSessionMediaCandidateLoadState::cancel()
{
    m_currentUrl = QUrl();
    m_operation.cancel();
}
}
