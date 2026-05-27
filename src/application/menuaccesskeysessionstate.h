// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MENUACCESSKEYSESSIONSTATE_H
#define KIRIVIEW_MENUACCESSKEYSESSIONSTATE_H

#include "application/menuaccesskeyinput.h"

namespace KiriView {
enum class MenuAccessKeyVisualEffect {
    None,
    Activate,
    Clear,
};

enum class MenuAccessKeyRoutingPhase {
    KeyPress,
    ShortcutOverride,
};

enum class MenuAccessKeySessionEvent {
    Begin,
    ReleaseAltKey,
    MenuUnavailable,
    Clear,
};

struct MenuAccessKeySessionPlan {
    MenuAccessKeyVisualEffect visualEffect = MenuAccessKeyVisualEffect::None;
    bool consumeEvent = false;
    bool triggerMnemonic = false;
    bool accessKeySessionActive = false;
};

class MenuAccessKeySessionState final
{
public:
    bool isActive() const;

    MenuAccessKeySessionPlan handleSessionEvent(MenuAccessKeySessionEvent event);
    MenuAccessKeySessionPlan routeOpenMenuKey(
        MenuAccessKeyInputKind input, MenuAccessKeyRoutingPhase phase);

private:
    MenuAccessKeySessionPlan beginSession();
    MenuAccessKeySessionPlan releaseAltKey();
    MenuAccessKeySessionPlan clearSession();

    bool m_active = false;
};
}

#endif
