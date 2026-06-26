// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediapredecodeinputport.h"

#include "session/documentsessiondirectmediaactivityport.h"
#include "session/documentsessiondirectmediascopeport.h"
#include "session/documentsessionpublicprojection.h"
#include "session/documentsessionstate.h"

namespace kiriview {
DocumentSessionMediaPredecodeInputPort::DocumentSessionMediaPredecodeInputPort(
    const DocumentSessionState* state, const DocumentSessionDirectMediaActivityPort* activity,
    const DocumentSessionDirectMediaScopePort* scope,
    const DocumentSessionPublicImageLeafSnapshot* image)
    : m_state(state)
    , m_activity(activity)
    , m_scope(scope)
    , m_image(image)
{
}

DocumentSessionMediaPredecodeInput DocumentSessionMediaPredecodeInputPort::currentInput() const
{
    return DocumentSessionMediaPredecodeInput {
        m_activity->navigationActive(),
        m_state->documentKind(),
        m_image->ordinaryDirectMediaScopeActive,
        m_image->readyForInformation,
        m_scope->activeCursorUrl(),
        m_image->primaryDisplayedPredecodeImage,
        m_image->firstDisplayDecodeContext,
    };
}
}
