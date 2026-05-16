// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeyrouter.h"

#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMetaObject>
#include <QQuickItem>
#include <QVariant>

MenuAccessKeyRouter::MenuAccessKeyRouter(QObject *parent)
    : QObject(parent)
{
    if (QCoreApplication::instance() != nullptr) {
        QCoreApplication::instance()->installEventFilter(this);
    }
}

MenuAccessKeyRouter::~MenuAccessKeyRouter()
{
    if (QCoreApplication::instance() != nullptr) {
        QCoreApplication::instance()->removeEventFilter(this);
    }
}

QObject *MenuAccessKeyRouter::menu() const { return m_menu; }

void MenuAccessKeyRouter::setMenu(QObject *menu)
{
    if (m_menu == menu) {
        return;
    }

    m_menu = menu;
    resetAltTracking();
    Q_EMIT menuChanged();
}

bool MenuAccessKeyRouter::isEnabled() const { return m_enabled; }

void MenuAccessKeyRouter::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;
    resetAltTracking();
    Q_EMIT enabledChanged();
}

bool MenuAccessKeyRouter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)

    if (!m_enabled || m_menu.isNull()) {
        return false;
    }

    switch (event->type()) {
    case QEvent::KeyPress:
        return handleKeyPress(static_cast<QKeyEvent *>(event));
    case QEvent::KeyRelease:
        return handleKeyRelease(static_cast<QKeyEvent *>(event));
    default:
        return false;
    }
}

bool MenuAccessKeyRouter::handleKeyPress(QKeyEvent *event)
{
    QObject *menu = openMenu();
    if (menu == nullptr) {
        resetAltTracking();
        return false;
    }

    if (event->key() == Qt::Key_Alt) {
        m_altPressedInOpenMenu = true;
        return true;
    }

    if (!isAltMnemonicKeyPress(*event)) {
        return false;
    }

    QObject *item = menuItemForMnemonic(menu, *event);
    if (item == nullptr) {
        return false;
    }

    if (!clickMenuItem(item)) {
        return false;
    }

    resetAltTracking();
    return true;
}

bool MenuAccessKeyRouter::handleKeyRelease(QKeyEvent *event)
{
    if (event->key() != Qt::Key_Alt) {
        return false;
    }

    QObject *menu = openMenu();
    if (menu == nullptr) {
        resetAltTracking();
        return false;
    }

    if (!m_altPressedInOpenMenu) {
        return false;
    }

    resetAltTracking();
    return true;
}

QObject *MenuAccessKeyRouter::openMenu() const
{
    if (m_menu.isNull() || !isOpenMenu(m_menu)) {
        return nullptr;
    }

    return m_menu;
}

void MenuAccessKeyRouter::resetAltTracking() { m_altPressedInOpenMenu = false; }

bool MenuAccessKeyRouter::isMenu(QObject *object)
{
    return object != nullptr && object->inherits("QQuickMenu");
}

bool MenuAccessKeyRouter::isOpenMenu(QObject *object)
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

bool MenuAccessKeyRouter::isEnabledMenuItem(QObject *object)
{
    const QVariant enabled = object->property("enabled");
    return !enabled.isValid() || enabled.toBool();
}

bool MenuAccessKeyRouter::isAltMnemonicKeyPress(const QKeyEvent &event)
{
    if (event.key() == Qt::Key_Alt) {
        return false;
    }

    Qt::KeyboardModifiers modifiers = event.modifiers();
    if (!(modifiers & Qt::AltModifier)) {
        return false;
    }

    modifiers
        &= ~(Qt::AltModifier | Qt::ShiftModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier);
    return modifiers == Qt::NoModifier;
}

QQuickItem *MenuAccessKeyRouter::itemAt(QObject *menu, int index)
{
    QQuickItem *item = nullptr;
    QMetaObject::invokeMethod(
        menu, "itemAt", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, index));
    return item;
}

QObject *MenuAccessKeyRouter::menuItemForMnemonic(QObject *menu, const QKeyEvent &event)
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

bool MenuAccessKeyRouter::itemMatchesMnemonic(QObject *item, const QKeyEvent &event)
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

bool MenuAccessKeyRouter::clickMenuItem(QObject *item)
{
    return QMetaObject::invokeMethod(item, "click", Qt::DirectConnection);
}
