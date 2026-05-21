// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeymenuruntime.h"

#include <QKeyEvent>
#include <QKeySequence>
#include <QMetaObject>
#include <QQuickItem>
#include <QVariant>

namespace KiriView {
QObject *MenuAccessKeyMenuRuntime::menu() const { return m_menu; }

bool MenuAccessKeyMenuRuntime::setMenu(QObject *menu)
{
    if (m_menu == menu) {
        return false;
    }

    setAccessKeysActive(false);
    m_menu = menu;
    return true;
}

QObject *MenuAccessKeyMenuRuntime::openMenu() const
{
    if (m_menu.isNull() || !isOpenMenu(m_menu)) {
        return nullptr;
    }

    return deepestOpenMenu(m_menu);
}

bool MenuAccessKeyMenuRuntime::triggerMnemonic(QKeyEvent *event, bool accessKeySessionActive)
{
    QObject *menu = openMenu();
    if (menu == nullptr || !isMnemonicKeyPress(*event)) {
        return false;
    }

    QObject *item = menuItemForMnemonic(menu, *event);
    if (item == nullptr) {
        return isAltMnemonicKeyPress(*event);
    }

    QObject *subMenu = subMenuForItem(item);
    if (subMenu != nullptr) {
        const bool opened = openSubMenu(subMenu, item);
        if (opened && (accessKeySessionActive || (event->modifiers() & Qt::AltModifier))) {
            setMenuAccessKeysActive(subMenu, true);
        }
        return opened;
    }

    return clickMenuItem(item);
}

void MenuAccessKeyMenuRuntime::setAccessKeysActive(bool active)
{
    setMenuAccessKeysActive(m_menu, active);
}

bool MenuAccessKeyMenuRuntime::isMenu(QObject *object)
{
    return object != nullptr && object->inherits("QQuickMenu");
}

bool MenuAccessKeyMenuRuntime::isOpenMenu(QObject *object)
{
    if (!isMenu(object)) {
        return false;
    }

    const QVariant opened = object->property("opened");
    if (opened.isValid() && opened.toBool()) {
        return true;
    }

    const QVariant visible = object->property("visible");
    return visible.isValid() && visible.toBool();
}

bool MenuAccessKeyMenuRuntime::isEnabledMenuItem(QObject *object)
{
    const QVariant enabled = object->property("enabled");
    return !enabled.isValid() || enabled.toBool();
}

bool MenuAccessKeyMenuRuntime::isMnemonicKeyPress(const QKeyEvent &event)
{
    if (event.key() == Qt::Key_Alt) {
        return false;
    }

    Qt::KeyboardModifiers modifiers = event.modifiers();
    modifiers
        &= ~(Qt::AltModifier | Qt::ShiftModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier);
    return modifiers == Qt::NoModifier;
}

bool MenuAccessKeyMenuRuntime::isAltMnemonicKeyPress(const QKeyEvent &event)
{
    return isMnemonicKeyPress(event) && (event.modifiers() & Qt::AltModifier);
}

QQuickItem *MenuAccessKeyMenuRuntime::itemAt(QObject *menu, int index)
{
    QQuickItem *item = nullptr;
    QMetaObject::invokeMethod(
        menu, "itemAt", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, index));
    return item;
}

QObject *MenuAccessKeyMenuRuntime::subMenuForItem(QObject *item)
{
    if (item == nullptr) {
        return nullptr;
    }

    return item->property("subMenu").value<QObject *>();
}

QObject *MenuAccessKeyMenuRuntime::deepestOpenMenu(QObject *menu)
{
    if (!isOpenMenu(menu)) {
        return nullptr;
    }

    QObject *deepest = menu;
    bool ok = false;
    const int count = menu->property("count").toInt(&ok);
    if (!ok) {
        return deepest;
    }

    for (int index = 0; index < count; ++index) {
        QObject *subMenu = subMenuForItem(itemAt(menu, index));
        QObject *openSubMenu = deepestOpenMenu(subMenu);
        if (openSubMenu != nullptr) {
            deepest = openSubMenu;
        }
    }

    return deepest;
}

QObject *MenuAccessKeyMenuRuntime::menuItemForMnemonic(QObject *menu, const QKeyEvent &event)
{
    bool ok = false;
    const int count = menu->property("count").toInt(&ok);
    if (!ok) {
        return nullptr;
    }

    for (int index = 0; index < count; ++index) {
        QObject *item = itemAt(menu, index);
        if (item != nullptr && itemMatchesMnemonic(item, event)) {
            return item;
        }
    }

    return nullptr;
}

bool MenuAccessKeyMenuRuntime::itemMatchesMnemonic(QObject *item, const QKeyEvent &event)
{
    if (!isEnabledMenuItem(item)) {
        return false;
    }

    const QString text = item->property("text").toString();
    if (text.isEmpty()) {
        return false;
    }

    const QKeySequence mnemonic = QKeySequence::mnemonic(text);
    if (mnemonic.isEmpty()) {
        return false;
    }

    const QKeyCombination combination = mnemonic[0];
    return (combination.keyboardModifiers() & Qt::AltModifier)
        && combination.key() == static_cast<Qt::Key>(event.key());
}

bool MenuAccessKeyMenuRuntime::openSubMenu(QObject *subMenu, QObject *item)
{
    QQuickItem *menuItem = qobject_cast<QQuickItem *>(item);
    if (menuItem == nullptr) {
        return false;
    }

    // clang-format off
    if (QMetaObject::invokeMethod(subMenu, "popup", Qt::DirectConnection,
            Q_ARG(QQuickItem*, menuItem), Q_ARG(QQuickItem*, menuItem))) {
        // clang-format on
        return true;
    }

    // clang-format off
    return QMetaObject::invokeMethod(
        subMenu, "popup", Qt::DirectConnection, Q_ARG(QQuickItem*, menuItem));
    // clang-format on
}

void MenuAccessKeyMenuRuntime::setMenuAccessKeysActive(QObject *menu, bool active)
{
    if (!isMenu(menu)) {
        return;
    }

    bool ok = false;
    const int count = menu->property("count").toInt(&ok);
    if (!ok) {
        return;
    }

    for (int index = 0; index < count; ++index) {
        QObject *item = itemAt(menu, index);
        if (item == nullptr) {
            continue;
        }

        item->setProperty("accessKeysActive", active);
        setMenuAccessKeysActive(subMenuForItem(item), active);
    }
}

bool MenuAccessKeyMenuRuntime::clickMenuItem(QObject *item)
{
    return QMetaObject::invokeMethod(item, "click", Qt::DirectConnection);
}
}
