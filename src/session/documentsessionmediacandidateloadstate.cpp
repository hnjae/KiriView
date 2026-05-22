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
    const ImageAsyncScopedOperation<QUrl> operation = m_operation.start(currentUrl);
    return DocumentSessionMediaCandidateLoad {
        operation.operationId,
        operation.scope,
    };
}

bool DocumentSessionMediaCandidateLoadState::accepts(
    const DocumentSessionMediaCandidateLoad &load) const
{
    return m_operation.accepts(load.operationId, load.currentUrl);
}

bool DocumentSessionMediaCandidateLoadState::finish(const DocumentSessionMediaCandidateLoad &load)
{
    return m_operation.finish(load.operationId, load.currentUrl);
}

void DocumentSessionMediaCandidateLoadState::cancel() { m_operation.cancel(); }
}
