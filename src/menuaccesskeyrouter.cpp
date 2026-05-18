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

    setMenuAccessKeysActive(m_menu, false);
    QObject::disconnect(m_menuClosedConnection);
    m_menuClosedConnection = {};
    m_menu = menu;
    resetAltTracking();
    if (m_menu != nullptr) {
        m_menuClosedConnection
            = QObject::connect(m_menu, SIGNAL(closed()), this, SLOT(clearMenuAccessKeys()));
    }
    Q_EMIT menuChanged();
}

bool MenuAccessKeyRouter::isEnabled() const { return m_enabled; }

void MenuAccessKeyRouter::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    if (!enabled) {
        setMenuAccessKeysActive(m_menu, false);
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
    case QEvent::ShortcutOverride:
        return handleShortcutOverride(static_cast<QKeyEvent *>(event));
    default:
        return false;
    }
}

bool MenuAccessKeyRouter::handleKeyPress(QKeyEvent *event)
{
    return routeOpenMenuKey(event, KeyRoutingMode::TriggerMnemonic);
}

bool MenuAccessKeyRouter::handleKeyRelease(QKeyEvent *event)
{
    if (event->key() != Qt::Key_Alt) {
        return false;
    }

    const KiriView::MenuAccessKeySessionTransition transition = m_accessKeySession.releaseAltKey();
    applySessionTransition(transition);
    if (transition.consumeEvent) {
        event->accept();
    }
    return transition.consumeEvent;
}

bool MenuAccessKeyRouter::handleShortcutOverride(QKeyEvent *event)
{
    return routeOpenMenuKey(event, KeyRoutingMode::ClaimShortcutOverride);
}

bool MenuAccessKeyRouter::routeOpenMenuKey(QKeyEvent *event, KeyRoutingMode mode)
{
    QObject *menu = openMenuOrClearAccessKeys();
    if (menu == nullptr) {
        return false;
    }

    if (event->key() == Qt::Key_Alt) {
        beginAccessKeySession();
        event->accept();
        return true;
    }

    if (isAltMnemonicKeyPress(*event)) {
        beginAccessKeySession();
        if (mode == KeyRoutingMode::ClaimShortcutOverride) {
            event->accept();
            return true;
        }
    } else if (mode == KeyRoutingMode::ClaimShortcutOverride) {
        return false;
    }

    const bool handled = triggerMnemonic(event, menu);
    if (handled) {
        event->accept();
    }
    return handled;
}

bool MenuAccessKeyRouter::triggerMnemonic(QKeyEvent *event, QObject *menu)
{
    if (!isMnemonicKeyPress(*event)) {
        return false;
    }

    QObject *item = menuItemForMnemonic(menu, *event);
    if (item == nullptr) {
        return isAltMnemonicKeyPress(*event);
    }

    QObject *subMenu = subMenuForItem(item);
    if (subMenu != nullptr) {
        const bool opened = openSubMenu(subMenu, item);
        if (opened && (m_accessKeySession.isActive() || (event->modifiers() & Qt::AltModifier))) {
            setMenuAccessKeysActive(subMenu, true);
        }
        return opened;
    }

    return clickMenuItem(item);
}

QObject *MenuAccessKeyRouter::openMenu() const
{
    if (m_menu.isNull() || !isOpenMenu(m_menu)) {
        return nullptr;
    }

    return deepestOpenMenu(m_menu);
}

QObject *MenuAccessKeyRouter::openMenuOrClearAccessKeys()
{
    QObject *menu = openMenu();
    if (menu != nullptr) {
        return menu;
    }

    applySessionTransition(m_accessKeySession.menuUnavailable());
    return nullptr;
}

void MenuAccessKeyRouter::beginAccessKeySession()
{
    applySessionTransition(m_accessKeySession.beginSession());
}

void MenuAccessKeyRouter::applySessionTransition(
    KiriView::MenuAccessKeySessionTransition transition)
{
    switch (transition.visualEffect) {
    case KiriView::MenuAccessKeyVisualEffect::None:
        return;
    case KiriView::MenuAccessKeyVisualEffect::Activate:
        setMenuAccessKeysActive(m_menu, true);
        return;
    case KiriView::MenuAccessKeyVisualEffect::Clear:
        setMenuAccessKeysActive(m_menu, false);
        return;
    }
}

void MenuAccessKeyRouter::clearAccessKeySessionVisuals()
{
    applySessionTransition(m_accessKeySession.clearVisuals());
}

void MenuAccessKeyRouter::clearMenuAccessKeys() { clearAccessKeySessionVisuals(); }

void MenuAccessKeyRouter::resetAltTracking() { m_accessKeySession.reset(); }

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

bool MenuAccessKeyRouter::isMnemonicKeyPress(const QKeyEvent &event)
{
    if (event.key() == Qt::Key_Alt) {
        return false;
    }

    Qt::KeyboardModifiers modifiers = event.modifiers();
    modifiers
        &= ~(Qt::AltModifier | Qt::ShiftModifier | Qt::KeypadModifier | Qt::GroupSwitchModifier);
    return modifiers == Qt::NoModifier;
}

bool MenuAccessKeyRouter::isAltMnemonicKeyPress(const QKeyEvent &event)
{
    return isMnemonicKeyPress(event) && (event.modifiers() & Qt::AltModifier);
}

QQuickItem *MenuAccessKeyRouter::itemAt(QObject *menu, int index)
{
    QQuickItem *item = nullptr;
    QMetaObject::invokeMethod(
        menu, "itemAt", Qt::DirectConnection, Q_RETURN_ARG(QQuickItem *, item), Q_ARG(int, index));
    return item;
}

QObject *MenuAccessKeyRouter::subMenuForItem(QObject *item)
{
    if (item == nullptr) {
        return nullptr;
    }

    return item->property("subMenu").value<QObject *>();
}

QObject *MenuAccessKeyRouter::deepestOpenMenu(QObject *menu)
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

bool MenuAccessKeyRouter::openSubMenu(QObject *subMenu, QObject *item)
{
    QQuickItem *menuItem = qobject_cast<QQuickItem *>(item);
    if (menuItem == nullptr) {
        return false;
    }

    if (QMetaObject::invokeMethod(subMenu, "popup", Qt::DirectConnection,
            Q_ARG(QQuickItem *, menuItem), Q_ARG(QQuickItem *, menuItem))) {
        return true;
    }

    return QMetaObject::invokeMethod(
        subMenu, "popup", Qt::DirectConnection, Q_ARG(QQuickItem *, menuItem));
}

void MenuAccessKeyRouter::setMenuAccessKeysActive(QObject *menu, bool active)
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

bool MenuAccessKeyRouter::clickMenuItem(QObject *item)
{
    return QMetaObject::invokeMethod(item, "click", Qt::DirectConnection);
}
