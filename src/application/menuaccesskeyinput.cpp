// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeyinput.h"

#include <QKeyEvent>

namespace KiriView {
bool menuAccessKeyIsMnemonicKeyPress(const QKeyEvent &event)
{
    if (event.key() == Qt::Key_Alt) {
        return false;
    }

    Qt::KeyboardModifiers modifiers = event.modifiers();
    modifiers
        &= ~(Qt::AltModifier | Qt::ShiftModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier);
    return modifiers == Qt::NoModifier;
}

bool menuAccessKeyIsAltMnemonicKeyPress(const QKeyEvent &event)
{
    return menuAccessKeyIsMnemonicKeyPress(event) && (event.modifiers() & Qt::AltModifier);
}

MenuAccessKeyInputKind menuAccessKeyInputKind(const QKeyEvent &event)
{
    if (event.key() == Qt::Key_Alt) {
        return MenuAccessKeyInputKind::AltKey;
    }
    if (menuAccessKeyIsAltMnemonicKeyPress(event)) {
        return MenuAccessKeyInputKind::AltMnemonic;
    }
    if (menuAccessKeyIsMnemonicKeyPress(event)) {
        return MenuAccessKeyInputKind::Mnemonic;
    }
    return MenuAccessKeyInputKind::Other;
}
}
