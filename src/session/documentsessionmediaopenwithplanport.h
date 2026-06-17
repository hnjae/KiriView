// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIAOPENWITHPLANPORT_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIAOPENWITHPLANPORT_H

#include "session/mediaopenwithplan.h"

namespace kiriview {
class DocumentSessionState;
struct DocumentSessionPublicImageLeafSnapshot;
struct DocumentSessionPublicVideoLeafSnapshot;

class DocumentSessionMediaOpenWithPlanPort final
{
public:
    DocumentSessionMediaOpenWithPlanPort(const DocumentSessionState *state,
        const DocumentSessionPublicImageLeafSnapshot *image,
        const DocumentSessionPublicVideoLeafSnapshot *video);

    MediaOpenWithPlan currentPlan() const;

private:
    const DocumentSessionState *m_state = nullptr;
    const DocumentSessionPublicImageLeafSnapshot *m_image = nullptr;
    const DocumentSessionPublicVideoLeafSnapshot *m_video = nullptr;
};
}

#endif
