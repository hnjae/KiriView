// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONMEDIAPREDECODEINPUTPORT_H
#define KIRIVIEW_DOCUMENTSESSIONMEDIAPREDECODEINPUTPORT_H

#include "session/documentsessionmediapredecoderuntime.h"

namespace kiriview {
class DocumentSessionDirectMediaActivityPort;
class DocumentSessionDirectMediaScopePort;
struct DocumentSessionPublicImageLeafSnapshot;
class DocumentSessionState;

class DocumentSessionMediaPredecodeInputPort final
{
public:
    DocumentSessionMediaPredecodeInputPort(const DocumentSessionState* state,
        const DocumentSessionDirectMediaActivityPort* activity,
        const DocumentSessionDirectMediaScopePort* scope,
        const DocumentSessionPublicImageLeafSnapshot* image);

    DocumentSessionMediaPredecodeInput currentInput() const;

private:
    const DocumentSessionState* m_state = nullptr;
    const DocumentSessionDirectMediaActivityPort* m_activity = nullptr;
    const DocumentSessionDirectMediaScopePort* m_scope = nullptr;
    const DocumentSessionPublicImageLeafSnapshot* m_image = nullptr;
};
}

#endif
