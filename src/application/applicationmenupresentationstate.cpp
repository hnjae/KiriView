// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationmenupresentationstate.h"

namespace KiriView::ApplicationActions {
ApplicationMenuPresentationState::ApplicationMenuPresentationState(MenuPresentation presentation)
    : m_presentation(presentationForStoredValue(storedValueForPresentation(presentation)))
{
}

MenuPresentation ApplicationMenuPresentationState::presentation() const { return m_presentation; }

int ApplicationMenuPresentationState::storedValue() const
{
    return storedValueForPresentation(m_presentation);
}

bool ApplicationMenuPresentationState::setPresentation(MenuPresentation presentation)
{
    const MenuPresentation normalizedPresentation
        = presentationForStoredValue(storedValueForPresentation(presentation));
    if (m_presentation == normalizedPresentation) {
        return false;
    }

    m_presentation = normalizedPresentation;
    return true;
}

bool ApplicationMenuPresentationState::setStoredValue(int value)
{
    return setPresentation(presentationForStoredValue(value));
}

MenuPresentation ApplicationMenuPresentationState::presentationForStoredValue(int value)
{
    if (value == static_cast<int>(MenuPresentation::MenuBar)) {
        return MenuPresentation::MenuBar;
    }

    return MenuPresentation::HamburgerMenu;
}

int ApplicationMenuPresentationState::storedValueForPresentation(MenuPresentation presentation)
{
    if (presentation == MenuPresentation::MenuBar) {
        return static_cast<int>(MenuPresentation::MenuBar);
    }

    return static_cast<int>(MenuPresentation::HamburgerMenu);
}
}
