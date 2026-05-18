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
        return clearVisuals();
    }

    m_active = false;
    return { MenuAccessKeyVisualEffect::Clear, true };
}

MenuAccessKeySessionTransition MenuAccessKeySessionState::menuUnavailable() const
{
    return clearVisuals();
}

MenuAccessKeySessionTransition MenuAccessKeySessionState::clearVisuals() const
{
    return { MenuAccessKeyVisualEffect::Clear, false };
}

void MenuAccessKeySessionState::reset() { m_active = false; }
}
