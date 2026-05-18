// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MENUACCESSKEYSESSIONSTATE_H
#define KIRIVIEW_MENUACCESSKEYSESSIONSTATE_H

namespace KiriView {
enum class MenuAccessKeyVisualEffect {
    None,
    Activate,
    Clear,
};

struct MenuAccessKeySessionTransition {
    MenuAccessKeyVisualEffect visualEffect = MenuAccessKeyVisualEffect::None;
    bool consumeEvent = false;
};

class MenuAccessKeySessionState final
{
public:
    bool isActive() const;

    MenuAccessKeySessionTransition beginSession();
    MenuAccessKeySessionTransition releaseAltKey();
    MenuAccessKeySessionTransition menuUnavailable() const;
    MenuAccessKeySessionTransition clearVisuals() const;
    void reset();

private:
    bool m_active = false;
};
}

#endif
