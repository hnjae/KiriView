// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessiondirectmedianavigationinputport.h"

#include "session/documentsessionstate.h"

namespace kiriview {
DocumentSessionDirectMediaNavigationInputPort::DocumentSessionDirectMediaNavigationInputPort(
    const DocumentSessionState* state)
    : m_state(state)
{
}

DirectMediaActiveNavigationInput DocumentSessionDirectMediaNavigationInputPort::currentInput() const
{
    return DirectMediaActiveNavigationInput { m_state->directMediaNavigationState(),
        m_state->directMediaNavigationKnown() };
}
}
