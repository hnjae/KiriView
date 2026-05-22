// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeyrouter.h"

#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMetaObject>

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

QObject *MenuAccessKeyRouter::menu() const { return m_menuRuntime.menu(); }

void MenuAccessKeyRouter::setMenu(QObject *menu)
{
    if (!m_menuRuntime.setMenu(menu)) {
        return;
    }

    QObject::disconnect(m_menuClosedConnection);
    m_menuClosedConnection = {};
    resetAltTracking();
    if (m_menuRuntime.menu() != nullptr) {
        m_menuClosedConnection = QObject::connect(
            m_menuRuntime.menu(), SIGNAL(closed()), this, SLOT(clearMenuAccessKeys()));
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
        m_menuRuntime.setAccessKeysActive(false);
    }
    m_enabled = enabled;
    resetAltTracking();
    Q_EMIT enabledChanged();
}

bool MenuAccessKeyRouter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)

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

bool MenuAccessKeyRouter::handleKeyPress(QKeyEvent *event)
{
    return routeOpenMenuKey(event, KiriView::MenuAccessKeyRoutingPhase::KeyPress);
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
    return routeOpenMenuKey(event, KiriView::MenuAccessKeyRoutingPhase::ShortcutOverride);
}

bool MenuAccessKeyRouter::routeOpenMenuKey(
    QKeyEvent *event, KiriView::MenuAccessKeyRoutingPhase phase)
{
    if (openMenuOrClearAccessKeys() == nullptr) {
        return false;
    }

    return executeRoutePlan(event, m_accessKeySession.routeOpenMenuKey(inputKind(*event), phase));
}

KiriView::MenuAccessKeyInputKind MenuAccessKeyRouter::inputKind(const QKeyEvent &event) const
{
    if (event.key() == Qt::Key_Alt) {
        return KiriView::MenuAccessKeyInputKind::AltKey;
    }
    if (KiriView::MenuAccessKeyMenuRuntime::isAltMnemonicKeyPress(event)) {
        return KiriView::MenuAccessKeyInputKind::AltMnemonic;
    }
    if (KiriView::MenuAccessKeyMenuRuntime::isMnemonicKeyPress(event)) {
        return KiriView::MenuAccessKeyInputKind::Mnemonic;
    }
    return KiriView::MenuAccessKeyInputKind::Other;
}

QObject *MenuAccessKeyRouter::openMenuOrClearAccessKeys()
{
    QObject *menu = m_menuRuntime.openMenu();
    if (menu != nullptr) {
        return menu;
    }

    applySessionTransition(m_accessKeySession.menuUnavailable());
    return nullptr;
}

bool MenuAccessKeyRouter::executeRoutePlan(QKeyEvent *event, KiriView::MenuAccessKeyRoutePlan plan)
{
    applySessionTransition(KiriView::MenuAccessKeySessionTransition {
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

void MenuAccessKeyRouter::applySessionTransition(
    KiriView::MenuAccessKeySessionTransition transition)
{
    switch (transition.visualEffect) {
    case KiriView::MenuAccessKeyVisualEffect::None:
        return;
    case KiriView::MenuAccessKeyVisualEffect::Activate:
        m_menuRuntime.setAccessKeysActive(true);
        return;
    case KiriView::MenuAccessKeyVisualEffect::Clear:
        m_menuRuntime.setAccessKeysActive(false);
        return;
    }
}

void MenuAccessKeyRouter::clearAccessKeySessionVisuals()
{
    applySessionTransition(m_accessKeySession.clearVisuals());
}

void MenuAccessKeyRouter::clearMenuAccessKeys() { clearAccessKeySessionVisuals(); }

void MenuAccessKeyRouter::resetAltTracking() { m_accessKeySession.reset(); }
