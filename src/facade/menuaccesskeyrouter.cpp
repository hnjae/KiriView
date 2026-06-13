// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "menuaccesskeyrouter.h"

#include <QCoreApplication>
#include <QEvent>

MenuAccessKeyRouter::MenuAccessKeyRouter(QObject *parent)
    : QObject(parent)
    , m_runtime(this, [this](kiriview::MenuAccessKeyRouterChange change) {
        switch (change) {
        case kiriview::MenuAccessKeyRouterChange::Menu:
            Q_EMIT menuChanged();
            return;
        case kiriview::MenuAccessKeyRouterChange::Enabled:
            Q_EMIT enabledChanged();
            return;
        }
    })
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

QObject *MenuAccessKeyRouter::menu() const { return m_runtime.menu(); }

void MenuAccessKeyRouter::setMenu(QObject *menu) { m_runtime.setMenu(menu); }

bool MenuAccessKeyRouter::isEnabled() const { return m_runtime.isEnabled(); }

void MenuAccessKeyRouter::setEnabled(bool enabled) { m_runtime.setEnabled(enabled); }

bool MenuAccessKeyRouter::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)

    return m_runtime.handleEvent(event);
}

void MenuAccessKeyRouter::clearMenuAccessKeys() { m_runtime.clearMenuAccessKeys(); }
