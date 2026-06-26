// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONPUBLICSNAPSHOTINPUTPORT_H
#define KIRIVIEW_DOCUMENTSESSIONPUBLICSNAPSHOTINPUTPORT_H

#include "session/documentsessionpublicprojection.h"

#include <QtGlobal>

namespace kiriview {
class DocumentSessionDirectMediaActivityPort;
class DocumentSessionDirectMediaNavigationInputPort;
class DocumentSessionState;

class DocumentSessionPublicSnapshotInputPort final
{
public:
    DocumentSessionPublicSnapshotInputPort(const DocumentSessionState* state,
        const DocumentSessionDirectMediaActivityPort* activity,
        const DocumentSessionDirectMediaNavigationInputPort* directMediaNavigation,
        const DocumentSessionPublicImageLeafSnapshot* image,
        const DocumentSessionPublicVideoLeafSnapshot* video);

    DocumentSessionPublicSnapshotInput nextInput();

private:
    const DocumentSessionState* m_state = nullptr;
    const DocumentSessionDirectMediaActivityPort* m_activity = nullptr;
    const DocumentSessionDirectMediaNavigationInputPort* m_directMediaNavigation = nullptr;
    const DocumentSessionPublicImageLeafSnapshot* m_image = nullptr;
    const DocumentSessionPublicVideoLeafSnapshot* m_video = nullptr;
    quint64 m_nextRevision = 1;
};
}

#endif
