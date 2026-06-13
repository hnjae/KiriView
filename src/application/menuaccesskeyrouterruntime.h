// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MENUACCESSKEYROUTERRUNTIME_H
#define KIRIVIEW_MENUACCESSKEYROUTERRUNTIME_H

#include "application/menuaccesskeymenuruntime.h"
#include "application/menuaccesskeysessionstate.h"

#include <QMetaObject>
#include <functional>

class QEvent;
class QKeyEvent;
class QObject;

namespace kiriview {
enum class MenuAccessKeyRouterChange {
    Menu,
    Enabled,
};

class MenuAccessKeyRouterRuntime final
{
public:
    using ChangeCallback = std::function<void(MenuAccessKeyRouterChange)>;

    explicit MenuAccessKeyRouterRuntime(QObject *owner, ChangeCallback changeCallback = {});

    QObject *menu() const;
    void setMenu(QObject *menu);
    bool isEnabled() const;
    void setEnabled(bool enabled);

    bool handleEvent(QEvent *event);
    void clearMenuAccessKeys();

private:
    bool handleKeyPress(QKeyEvent *event);
    bool handleKeyRelease(QKeyEvent *event);
    bool handleShortcutOverride(QKeyEvent *event);
    bool routeOpenMenuKey(QKeyEvent *event, MenuAccessKeyRoutingPhase phase);
    QObject *openMenuOrClearAccessKeys();
    void applySessionPlan(MenuAccessKeySessionPlan plan);
    bool executeSessionPlan(QKeyEvent *event, MenuAccessKeySessionPlan plan);
    void notify(MenuAccessKeyRouterChange change) const;

    QObject *m_owner = nullptr;
    ChangeCallback m_changeCallback;
    MenuAccessKeyMenuRuntime m_menuRuntime;
    QMetaObject::Connection m_menuClosedConnection;
    MenuAccessKeySessionState m_accessKeySession;
    bool m_enabled = true;
};
}

#endif
