// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmediaactivityport.h"

#include "session/documentsessiondirectmediascopeport.h"
#include "session/documentsessionstate.h"

namespace kiriview {
DocumentSessionDirectMediaActivityPort::DocumentSessionDirectMediaActivityPort(
    const DocumentSessionState *state, const DocumentSessionDirectMediaScopePort *scope)
    : m_state(state)
    , m_scope(scope)
{
}

bool DocumentSessionDirectMediaActivityPort::navigationActive() const
{
    return m_state->documentKind() == DocumentSessionKind::Video
        || directImageSourceScopeEligible();
}

bool DocumentSessionDirectMediaActivityPort::directImageSourceScopeEligible() const
{
    return m_state->documentKind() == DocumentSessionKind::Image
        && !m_scope->activeCursorUrl().isEmpty();
}
}
