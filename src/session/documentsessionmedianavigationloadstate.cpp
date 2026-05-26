// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessionmedianavigationloadstate.h"

namespace KiriView {
DocumentSessionMediaNavigationLoadState::DocumentSessionMediaNavigationLoadState(
    quint64 nextOperationId)
    : m_operation(nextOperationId)
{
}

DocumentSessionMediaNavigationLoad DocumentSessionMediaNavigationLoadState::start(
    const QUrl &currentUrl, const QUrl &parentUrl, quint64 cursorGeneration)
{
    const ImageAsyncScopedOperation<DocumentSessionMediaNavigationLoadScope> operation
        = m_operation.start(
            DocumentSessionMediaNavigationLoadScope { currentUrl, parentUrl, cursorGeneration });
    return DocumentSessionMediaNavigationLoad {
        operation.operationId,
        operation.scope.currentUrl,
        operation.scope.parentUrl,
        operation.scope.cursorGeneration,
    };
}

bool DocumentSessionMediaNavigationLoadState::accepts(
    const DocumentSessionMediaNavigationLoad &load) const
{
    return m_operation.accepts(load.operationId,
        DocumentSessionMediaNavigationLoadScope {
            load.currentUrl,
            load.parentUrl,
            load.cursorGeneration,
        });
}

bool DocumentSessionMediaNavigationLoadState::finish(const DocumentSessionMediaNavigationLoad &load)
{
    return m_operation.finish(load.operationId,
        DocumentSessionMediaNavigationLoadScope {
            load.currentUrl,
            load.parentUrl,
            load.cursorGeneration,
        });
}

void DocumentSessionMediaNavigationLoadState::cancel() { m_operation.cancel(); }
}
