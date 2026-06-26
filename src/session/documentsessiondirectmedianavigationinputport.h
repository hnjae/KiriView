// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONINPUTPORT_H
#define KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIANAVIGATIONINPUTPORT_H

#include "session/activenavigationprojection.h"

namespace kiriview {
class DocumentSessionState;

class DocumentSessionDirectMediaNavigationInputPort final
{
public:
    explicit DocumentSessionDirectMediaNavigationInputPort(const DocumentSessionState* state);

    DirectMediaActiveNavigationInput currentInput() const;

private:
    const DocumentSessionState* m_state = nullptr;
};
}

#endif
