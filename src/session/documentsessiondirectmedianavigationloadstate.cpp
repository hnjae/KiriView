// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "documentsessiondirectmedianavigationloadstate.h"

namespace kiriview {
DocumentSessionDirectMediaNavigationLoadState::DocumentSessionDirectMediaNavigationLoadState(
    quint64 nextOperationId)
    : m_operation(nextOperationId)
{
}

DocumentSessionDirectMediaNavigationLoad DocumentSessionDirectMediaNavigationLoadState::start(
    const DirectMediaScope& scope)
{
    const ImageAsyncScopedOperation<DirectMediaScope> operation = m_operation.start(scope);
    return DocumentSessionDirectMediaNavigationLoad {
        operation.operationId,
        operation.scope,
    };
}

bool DocumentSessionDirectMediaNavigationLoadState::accepts(
    const DocumentSessionDirectMediaNavigationLoad& load) const
{
    return m_operation.accepts(load.operationId, load.scope);
}

bool DocumentSessionDirectMediaNavigationLoadState::finish(
    const DocumentSessionDirectMediaNavigationLoad& load)
{
    return m_operation.finish(load.operationId, load.scope);
}

void DocumentSessionDirectMediaNavigationLoadState::cancel() { m_operation.cancel(); }
}
