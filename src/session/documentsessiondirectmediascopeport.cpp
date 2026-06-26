// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmediascopeport.h"

#include "session/documentsessionstate.h"

namespace kiriview {
DocumentSessionDirectMediaScopePort::DocumentSessionDirectMediaScopePort(
    const DocumentSessionState* state)
    : m_state(state)
{
}

DirectMediaScope DocumentSessionDirectMediaScopePort::currentScope() const
{
    return m_state->directMediaScope();
}

QUrl DocumentSessionDirectMediaScopePort::activeCursorUrl() const
{
    return m_state->directMediaCursorUrl();
}

bool DocumentSessionDirectMediaScopePort::cursorMatches(const DirectMediaScope& scope) const
{
    return directMediaScopeMatchesCursor(m_state->directMediaCursor(), scope);
}
}
