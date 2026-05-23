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
    const QUrl &currentUrl, const QUrl &parentUrl, quint64 cursorGeneration)
{
    const ImageAsyncScopedOperation<DocumentSessionMediaCandidateLoadScope> operation
        = m_operation.start(
            DocumentSessionMediaCandidateLoadScope { currentUrl, parentUrl, cursorGeneration });
    return DocumentSessionMediaCandidateLoad {
        operation.operationId,
        operation.scope.currentUrl,
        operation.scope.parentUrl,
        operation.scope.cursorGeneration,
    };
}

bool DocumentSessionMediaCandidateLoadState::accepts(
    const DocumentSessionMediaCandidateLoad &load) const
{
    return m_operation.accepts(load.operationId,
        DocumentSessionMediaCandidateLoadScope {
            load.currentUrl,
            load.parentUrl,
            load.cursorGeneration,
        });
}

bool DocumentSessionMediaCandidateLoadState::finish(const DocumentSessionMediaCandidateLoad &load)
{
    return m_operation.finish(load.operationId,
        DocumentSessionMediaCandidateLoadScope {
            load.currentUrl,
            load.parentUrl,
            load.cursorGeneration,
        });
}

void DocumentSessionMediaCandidateLoadState::cancel() { m_operation.cancel(); }
}
