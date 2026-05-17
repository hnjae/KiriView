// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationshortcutpolicy.h"

#include <QStringList>

namespace {
bool shortcutHasCommandModifier(const QKeySequence &shortcut)
{
    const Qt::KeyboardModifiers commandModifiers
        = Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;

    for (int index = 0; index < shortcut.count(); ++index) {
        const Qt::KeyboardModifiers shortcutModifiers
            = shortcut[static_cast<uint>(index)].keyboardModifiers();
        if ((shortcutModifiers & commandModifiers) != Qt::NoModifier) {
            return true;
        }
    }

    return false;
}

bool isAsciiPrintableKey(Qt::Key key) { return key >= Qt::Key_Space && key <= Qt::Key_AsciiTilde; }

bool isDeletionOrNavigationKey(Qt::Key key)
{
    switch (key) {
    case Qt::Key_Backspace:
    case Qt::Key_Delete:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
        return true;
    default:
        return false;
    }
}

bool shortcutCombinationIsMenuDisplaySafe(QKeyCombination combination)
{
    const Qt::Key key = combination.key();
    if (isDeletionOrNavigationKey(key)) {
        return false;
    }

    const Qt::KeyboardModifiers commandModifiers
        = Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;
    if ((combination.keyboardModifiers() & commandModifiers) == Qt::NoModifier
        && isAsciiPrintableKey(key)) {
        return false;
    }

    return key != Qt::Key_unknown;
}

bool shortcutIsMenuDisplaySafe(const QKeySequence &shortcut)
{
    if (shortcut.isEmpty()) {
        return false;
    }

    for (int index = 0; index < shortcut.count(); ++index) {
        if (!shortcutCombinationIsMenuDisplaySafe(shortcut[static_cast<uint>(index)])) {
            return false;
        }
    }

    return true;
}

bool shortcutCombinationIsUnmodifiedAsciiPrintable(QKeyCombination combination)
{
    return combination.keyboardModifiers() == Qt::NoModifier
        && isAsciiPrintableKey(combination.key());
}

bool shortcutIsUnmodifiedAsciiPrintable(const QKeySequence &shortcut)
{
    return shortcut.count() == 1 && shortcutCombinationIsUnmodifiedAsciiPrintable(shortcut[0]);
}

QKeySequence shortcutAlias(const QKeySequence &shortcut)
{
    if (shortcut.count() != 1) {
        return {};
    }

    const QKeyCombination combination = shortcut[0];
    const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
    const Qt::KeyboardModifiers allowedModifiers = Qt::ControlModifier | Qt::ShiftModifier;
    if ((modifiers & Qt::ControlModifier) == Qt::NoModifier
        || (modifiers & ~allowedModifiers) != Qt::NoModifier
        || !isAsciiPrintableKey(combination.key())) {
        return {};
    }

    return QKeySequence(QKeyCombination(modifiers & ~Qt::ControlModifier, combination.key()));
}
}

namespace KiriView::ApplicationActions {
QList<QKeySequence> filterShortcutsByCommandModifier(
    const QList<QKeySequence> &shortcuts, bool requireCommandModifier)
{
    QList<QKeySequence> filteredShortcuts;
    filteredShortcuts.reserve(shortcuts.size());

    for (const QKeySequence &shortcut : shortcuts) {
        if (!shortcut.isEmpty() && shortcutHasCommandModifier(shortcut) == requireCommandModifier) {
            filteredShortcuts.push_back(shortcut);
        }
    }

    return filteredShortcuts;
}

QKeySequence menuShortcut(const QList<QKeySequence> &shortcuts)
{
    for (const QKeySequence &shortcut : shortcuts) {
        if (shortcutIsMenuDisplaySafe(shortcut)) {
            return shortcut;
        }
    }

    return {};
}

QList<QKeySequence> shortcutAliases(const QList<QKeySequence> &shortcuts)
{
    QList<QKeySequence> aliases;
    aliases.reserve(shortcuts.size());

    for (const QKeySequence &shortcut : shortcuts) {
        const QKeySequence alias = shortcutAlias(shortcut);
        if (!alias.isEmpty() && !aliases.contains(alias)) {
            aliases.push_back(alias);
        }
    }

    return aliases;
}

QString shortcutListText(const QList<QKeySequence> &shortcuts)
{
    QStringList texts;
    texts.reserve(shortcuts.size());
    for (const QKeySequence &shortcut : shortcuts) {
        if (!shortcut.isEmpty()) {
            texts.push_back(shortcut.toString(QKeySequence::NativeText));
        }
    }

    return texts.join(QStringLiteral(" / "));
}

QList<QKeySequence> sanitizeShortcuts(const QList<QKeySequence> &shortcuts)
{
    QList<QKeySequence> sanitizedShortcuts;
    sanitizedShortcuts.reserve(shortcuts.size());

    for (const QKeySequence &shortcut : shortcuts) {
        if (!shortcutIsUnmodifiedAsciiPrintable(shortcut)) {
            sanitizedShortcuts.push_back(shortcut);
        }
    }

    return sanitizedShortcuts;
}
}
