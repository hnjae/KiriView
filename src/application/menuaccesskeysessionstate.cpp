// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeysessionstate.h"

namespace kiriview {
bool MenuAccessKeySessionState::isActive() const { return m_active; }

MenuAccessKeySessionPlan MenuAccessKeySessionState::handleSessionEvent(
    MenuAccessKeySessionEvent event)
{
    switch (event) {
    case MenuAccessKeySessionEvent::Begin:
        return beginSession();
    case MenuAccessKeySessionEvent::ReleaseAltKey:
        return releaseAltKey();
    case MenuAccessKeySessionEvent::MenuUnavailable:
    case MenuAccessKeySessionEvent::Clear:
        return clearSession();
    }

    return {};
}

MenuAccessKeySessionPlan MenuAccessKeySessionState::beginSession()
{
    m_active = true;
    return { MenuAccessKeyVisualEffect::Activate, false, false, m_active };
}

MenuAccessKeySessionPlan MenuAccessKeySessionState::releaseAltKey()
{
    if (!m_active) {
        return clearSession();
    }

    m_active = false;
    return { MenuAccessKeyVisualEffect::Clear, true, false, m_active };
}

MenuAccessKeySessionPlan MenuAccessKeySessionState::clearSession()
{
    m_active = false;
    return { MenuAccessKeyVisualEffect::Clear, false, false, m_active };
}

MenuAccessKeySessionPlan MenuAccessKeySessionState::routeOpenMenuKey(
    MenuAccessKeyInputKind input, MenuAccessKeyRoutingPhase phase)
{
    switch (input) {
    case MenuAccessKeyInputKind::AltKey: {
        MenuAccessKeySessionPlan plan = beginSession();
        plan.consumeEvent = true;
        return plan;
    }
    case MenuAccessKeyInputKind::AltMnemonic: {
        MenuAccessKeySessionPlan plan = beginSession();
        if (phase == MenuAccessKeyRoutingPhase::ShortcutOverride) {
            plan.consumeEvent = true;
            return plan;
        }
        plan.triggerMnemonic = true;
        return plan;
    }
    case MenuAccessKeyInputKind::Mnemonic:
        if (phase == MenuAccessKeyRoutingPhase::ShortcutOverride) {
            return {};
        }
        return { MenuAccessKeyVisualEffect::None, false, true, m_active };
    case MenuAccessKeyInputKind::Other:
        return {};
    }

    return {};
}

}
