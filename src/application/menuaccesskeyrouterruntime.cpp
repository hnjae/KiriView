// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeyrouterruntime.h"

#include <QEvent>
#include <QKeyEvent>
#include <QObject>
#include <utility>

namespace KiriView {
MenuAccessKeyRouterRuntime::MenuAccessKeyRouterRuntime(
    QObject *owner, ChangeCallback changeCallback)
    : m_owner(owner)
    , m_changeCallback(std::move(changeCallback))
{
}

QObject *MenuAccessKeyRouterRuntime::menu() const { return m_menuRuntime.menu(); }

void MenuAccessKeyRouterRuntime::setMenu(QObject *menu)
{
    if (m_menuRuntime.menu() == menu) {
        return;
    }

    QObject::disconnect(m_menuClosedConnection);
    m_menuClosedConnection = {};
    applySessionTransition(m_accessKeySession.clearSession());
    m_menuRuntime.setMenu(menu);
    if (m_menuRuntime.menu() != nullptr && m_owner != nullptr) {
        m_menuClosedConnection = QObject::connect(
            m_menuRuntime.menu(), SIGNAL(closed()), m_owner, SLOT(clearMenuAccessKeys()));
    }
    notify(MenuAccessKeyRouterChange::Menu);
}

bool MenuAccessKeyRouterRuntime::isEnabled() const { return m_enabled; }

void MenuAccessKeyRouterRuntime::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    if (!enabled) {
        applySessionTransition(m_accessKeySession.clearSession());
    }
    m_enabled = enabled;
    notify(MenuAccessKeyRouterChange::Enabled);
}

bool MenuAccessKeyRouterRuntime::handleEvent(QEvent *event)
{
    if (!m_enabled || m_menuRuntime.menu() == nullptr) {
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

bool MenuAccessKeyRouterRuntime::handleKeyPress(QKeyEvent *event)
{
    return routeOpenMenuKey(event, MenuAccessKeyRoutingPhase::KeyPress);
}

bool MenuAccessKeyRouterRuntime::handleKeyRelease(QKeyEvent *event)
{
    if (event->key() != Qt::Key_Alt) {
        return false;
    }

    const MenuAccessKeySessionTransition transition = m_accessKeySession.releaseAltKey();
    applySessionTransition(transition);
    if (transition.consumeEvent) {
        event->accept();
    }
    return transition.consumeEvent;
}

bool MenuAccessKeyRouterRuntime::handleShortcutOverride(QKeyEvent *event)
{
    return routeOpenMenuKey(event, MenuAccessKeyRoutingPhase::ShortcutOverride);
}

bool MenuAccessKeyRouterRuntime::routeOpenMenuKey(QKeyEvent *event, MenuAccessKeyRoutingPhase phase)
{
    if (openMenuOrClearAccessKeys() == nullptr) {
        return false;
    }

    return executeRoutePlan(event, m_accessKeySession.routeOpenMenuKey(inputKind(*event), phase));
}

MenuAccessKeyInputKind MenuAccessKeyRouterRuntime::inputKind(const QKeyEvent &event) const
{
    if (event.key() == Qt::Key_Alt) {
        return MenuAccessKeyInputKind::AltKey;
    }
    if (MenuAccessKeyMenuRuntime::isAltMnemonicKeyPress(event)) {
        return MenuAccessKeyInputKind::AltMnemonic;
    }
    if (MenuAccessKeyMenuRuntime::isMnemonicKeyPress(event)) {
        return MenuAccessKeyInputKind::Mnemonic;
    }
    return MenuAccessKeyInputKind::Other;
}

QObject *MenuAccessKeyRouterRuntime::openMenuOrClearAccessKeys()
{
    QObject *menu = m_menuRuntime.openMenu();
    if (menu != nullptr) {
        return menu;
    }

    applySessionTransition(m_accessKeySession.menuUnavailable());
    return nullptr;
}

bool MenuAccessKeyRouterRuntime::executeRoutePlan(QKeyEvent *event, MenuAccessKeyRoutePlan plan)
{
    applySessionTransition(MenuAccessKeySessionTransition {
        plan.visualEffect,
        false,
    });

    bool handled = plan.consumeEvent;
    if (plan.triggerMnemonic) {
        handled = m_menuRuntime.triggerMnemonic(event, plan.accessKeySessionActive);
    }
    if (handled) {
        event->accept();
    }
    return handled;
}

void MenuAccessKeyRouterRuntime::applySessionTransition(MenuAccessKeySessionTransition transition)
{
    switch (transition.visualEffect) {
    case MenuAccessKeyVisualEffect::None:
        return;
    case MenuAccessKeyVisualEffect::Activate:
        m_menuRuntime.setAccessKeysActive(true);
        return;
    case MenuAccessKeyVisualEffect::Clear:
        m_menuRuntime.setAccessKeysActive(false);
        return;
    }
}

void MenuAccessKeyRouterRuntime::clearMenuAccessKeys()
{
    applySessionTransition(m_accessKeySession.clearSession());
}

void MenuAccessKeyRouterRuntime::notify(MenuAccessKeyRouterChange change) const
{
    if (m_changeCallback) {
        m_changeCallback(change);
    }
}
}
