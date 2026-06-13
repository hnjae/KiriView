// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MENUACCESSKEYINPUT_H
#define KIRIVIEW_MENUACCESSKEYINPUT_H

class QKeyEvent;

namespace kiriview {
enum class MenuAccessKeyInputKind {
    AltKey,
    AltMnemonic,
    Mnemonic,
    Other,
};

bool menuAccessKeyIsMnemonicKeyPress(const QKeyEvent &event);
bool menuAccessKeyIsAltMnemonicKeyPress(const QKeyEvent &event);
MenuAccessKeyInputKind menuAccessKeyInputKind(const QKeyEvent &event);
}

#endif
