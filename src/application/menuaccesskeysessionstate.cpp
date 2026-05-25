// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeysessionstate.h"

namespace KiriView {
bool MenuAccessKeySessionState::isActive() const { return m_active; }

MenuAccessKeySessionTransition MenuAccessKeySessionState::beginSession()
{
    m_active = true;
    return { MenuAccessKeyVisualEffect::Activate, false };
}

MenuAccessKeySessionTransition MenuAccessKeySessionState::releaseAltKey()
{
    if (!m_active) {
        return clearSession();
    }

    m_active = false;
    return { MenuAccessKeyVisualEffect::Clear, true };
}

MenuAccessKeySessionTransition MenuAccessKeySessionState::menuUnavailable()
{
    return clearSession();
}

MenuAccessKeySessionTransition MenuAccessKeySessionState::clearSession()
{
    m_active = false;
    return { MenuAccessKeyVisualEffect::Clear, false };
}

MenuAccessKeyRoutePlan MenuAccessKeySessionState::routeOpenMenuKey(
    MenuAccessKeyInputKind input, MenuAccessKeyRoutingPhase phase)
{
    switch (input) {
    case MenuAccessKeyInputKind::AltKey: {
        const MenuAccessKeySessionTransition transition = beginSession();
        return { transition.visualEffect, true, false, m_active };
    }
    case MenuAccessKeyInputKind::AltMnemonic: {
        const MenuAccessKeySessionTransition transition = beginSession();
        if (phase == MenuAccessKeyRoutingPhase::ShortcutOverride) {
            return { transition.visualEffect, true, false, m_active };
        }
        return { transition.visualEffect, false, true, m_active };
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
