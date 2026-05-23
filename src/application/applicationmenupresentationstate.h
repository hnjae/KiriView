// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONMENUPRESENTATIONSTATE_H
#define KIRIVIEW_APPLICATIONMENUPRESENTATIONSTATE_H

#include "facade/kiriviewapplication.h"

namespace KiriView::ApplicationActions {
class ApplicationMenuPresentationState final
{
public:
    explicit ApplicationMenuPresentationState(
        KiriViewApplication::MenuPresentation presentation = KiriViewApplication::HamburgerMenu);

    KiriViewApplication::MenuPresentation presentation() const;
    int storedValue() const;

    bool setPresentation(KiriViewApplication::MenuPresentation presentation);
    bool setStoredValue(int value);

    static KiriViewApplication::MenuPresentation presentationForStoredValue(int value);
    static int storedValueForPresentation(KiriViewApplication::MenuPresentation presentation);

private:
    KiriViewApplication::MenuPresentation m_presentation = KiriViewApplication::HamburgerMenu;
};
}

#endif
