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
    const DirectMediaScope &scope)
{
    const ImageAsyncScopedOperation<DirectMediaScope> operation = m_operation.start(scope);
    return DocumentSessionMediaNavigationLoad {
        operation.operationId,
        operation.scope,
    };
}

bool DocumentSessionMediaNavigationLoadState::accepts(
    const DocumentSessionMediaNavigationLoad &load) const
{
    return m_operation.accepts(load.operationId, load.scope);
}

bool DocumentSessionMediaNavigationLoadState::finish(const DocumentSessionMediaNavigationLoad &load)
{
    return m_operation.finish(load.operationId, load.scope);
}

void DocumentSessionMediaNavigationLoadState::cancel() { m_operation.cancel(); }
}
