// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIAACTIVITYPORT_H
#define KIRIVIEW_DOCUMENTSESSIONDIRECTMEDIAACTIVITYPORT_H

namespace kiriview {
class DocumentSessionDirectMediaScopePort;
class DocumentSessionState;

class DocumentSessionDirectMediaActivityPort final
{
public:
    DocumentSessionDirectMediaActivityPort(
        const DocumentSessionState *state, const DocumentSessionDirectMediaScopePort *scope);

    bool navigationActive() const;
    bool directImageSourceScopeEligible() const;

private:
    const DocumentSessionState *m_state = nullptr;
    const DocumentSessionDirectMediaScopePort *m_scope = nullptr;
};
}

#endif
