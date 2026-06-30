// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionpublicsnapshotinputport.h"

#include "session/documentsessiondirectmediaactivityport.h"
#include "session/documentsessiondirectmedianavigationinputport.h"
#include "session/documentsessionpublicsnapshotinputbuilder.h"
#include "session/documentsessionstate.h"

namespace kiriview {
DocumentSessionPublicSnapshotInputPort::DocumentSessionPublicSnapshotInputPort(
    const DocumentSessionState* state, const DocumentSessionDirectMediaActivityPort* activity,
    const DocumentSessionDirectMediaNavigationInputPort* directMediaNavigation,
    const DocumentSessionPublicImageLeafSnapshot* image,
    const DocumentSessionPublicVideoLeafSnapshot* video)
    : m_state(state)
    , m_activity(activity)
    , m_directMediaNavigation(directMediaNavigation)
    , m_image(image)
    , m_video(video)
{
}

DocumentSessionPublicSnapshotInput DocumentSessionPublicSnapshotInputPort::nextInput()
{
    DocumentSessionPublicSnapshotInputBuilderInput input;
    input.inputRevision = m_nextRevision++;
    input.session.sourceUrl = m_state->sourceUrl();
    input.session.documentKind = m_state->documentKind();
    input.session.sessionErrorString = m_state->sessionErrorString();
    input.session.fileDeletionInProgress = m_state->fileDeletionInProgress();
    input.session.directImageLoadMayUseImageDocumentSourceScope
        = m_activity->directImageSourceScopeEligible();
    input.session.directMediaNavigation = m_directMediaNavigation->currentInput();
    input.session.activeNavigationRevealIntent = m_state->activeNavigationRevealIntent();
    input.session.activeNavigationRevealDirection = m_state->activeNavigationRevealDirection();
    input.session.openedCollectionVideoActive = m_state->openedCollectionVideoActive();
    input.image = *m_image;
    input.video = *m_video;
    input.directMediaCursor = m_state->directMediaCursor();
    return buildDocumentSessionPublicSnapshotInput(input);
}
}
