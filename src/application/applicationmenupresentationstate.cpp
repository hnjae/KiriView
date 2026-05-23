// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationmenupresentationstate.h"

namespace KiriView::ApplicationActions {
ApplicationMenuPresentationState::ApplicationMenuPresentationState(
    KiriViewApplication::MenuPresentation presentation)
    : m_presentation(presentationForStoredValue(storedValueForPresentation(presentation)))
{
}

KiriViewApplication::MenuPresentation ApplicationMenuPresentationState::presentation() const
{
    return m_presentation;
}

int ApplicationMenuPresentationState::storedValue() const
{
    return storedValueForPresentation(m_presentation);
}

bool ApplicationMenuPresentationState::setPresentation(
    KiriViewApplication::MenuPresentation presentation)
{
    const KiriViewApplication::MenuPresentation normalizedPresentation
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

KiriViewApplication::MenuPresentation ApplicationMenuPresentationState::presentationForStoredValue(
    int value)
{
    if (value == static_cast<int>(KiriViewApplication::MenuBar)) {
        return KiriViewApplication::MenuBar;
    }

    return KiriViewApplication::HamburgerMenu;
}

int ApplicationMenuPresentationState::storedValueForPresentation(
    KiriViewApplication::MenuPresentation presentation)
{
    if (presentation == KiriViewApplication::MenuBar) {
        return static_cast<int>(KiriViewApplication::MenuBar);
    }

    return static_cast<int>(KiriViewApplication::HamburgerMenu);
}
}
