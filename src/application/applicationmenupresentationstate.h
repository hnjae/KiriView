// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONMENUPRESENTATIONSTATE_H
#define KIRIVIEW_APPLICATIONMENUPRESENTATIONSTATE_H

#include "application/applicationtypes.h"

namespace kiriview::ApplicationActions {
class ApplicationMenuPresentationState final
{
public:
    explicit ApplicationMenuPresentationState(
        MenuPresentation presentation = MenuPresentation::HamburgerMenu);

    MenuPresentation presentation() const;
    int storedValue() const;

    bool setPresentation(MenuPresentation presentation);
    bool setStoredValue(int value);

    static MenuPresentation presentationForStoredValue(int value);
    static int storedValueForPresentation(MenuPresentation presentation);

private:
    MenuPresentation m_presentation = MenuPresentation::HamburgerMenu;
};
}

#endif
