// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediaopenwithplanport.h"

#include "session/documentsessionpublicprojection.h"
#include "session/documentsessionstate.h"

namespace kiriview {
DocumentSessionMediaOpenWithPlanPort::DocumentSessionMediaOpenWithPlanPort(
    const DocumentSessionState *state, const DocumentSessionPublicImageLeafSnapshot *image,
    const DocumentSessionPublicVideoLeafSnapshot *video)
    : m_state(state)
    , m_image(image)
    , m_video(video)
{
}

MediaOpenWithPlan DocumentSessionMediaOpenWithPlanPort::currentPlan() const
{
    return mediaOpenWithPlan(MediaOpenWithPlanInput {
        m_state->documentKind(),
        m_image->readyForInformation,
        m_image->displayedUrl,
        m_image->displayedOpenedCollectionScope,
        m_video->ready,
        m_video->sourceUrl,
    });
}
}
