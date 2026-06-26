// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MENUACCESSKEYMENURUNTIME_H
#define KIRIVIEW_MENUACCESSKEYMENURUNTIME_H

#include <QObject>
#include <QPointer>

class QKeyEvent;
class QQuickItem;

namespace kiriview {
class MenuAccessKeyMenuRuntime final
{
public:
    QObject* menu() const;
    bool setMenu(QObject* menu);

    QObject* openMenu() const;
    bool triggerMnemonic(QKeyEvent* event, bool accessKeySessionActive);
    void setAccessKeysActive(bool active);

private:
    static bool isMenu(QObject* object);
    static bool isOpenMenu(QObject* object);
    static bool isEnabledMenuItem(QObject* object);
    static QQuickItem* itemAt(QObject* menu, int index);
    static QObject* subMenuForItem(QObject* item);
    static QObject* deepestOpenMenu(QObject* menu);
    static QObject* menuItemForMnemonic(QObject* menu, const QKeyEvent& event);
    static bool itemMatchesMnemonic(QObject* item, const QKeyEvent& event);
    static bool openSubMenu(QObject* subMenu, QObject* item);
    static void setMenuAccessKeysActive(QObject* menu, bool active);
    static bool clickMenuItem(QObject* item);

    QPointer<QObject> m_menu;
};
}

#endif
