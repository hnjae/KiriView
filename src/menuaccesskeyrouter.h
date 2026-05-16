// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MENUACCESSKEYROUTER_H
#define KIRIVIEW_MENUACCESSKEYROUTER_H

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class QEvent;
class QKeyEvent;
class QQuickItem;

class MenuAccessKeyRouter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject *menu READ menu WRITE setMenu NOTIFY menuChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)

public:
    explicit MenuAccessKeyRouter(QObject *parent = nullptr);
    ~MenuAccessKeyRouter() override;

    QObject *menu() const;
    void setMenu(QObject *menu);
    bool isEnabled() const;
    void setEnabled(bool enabled);

Q_SIGNALS:
    void menuChanged();
    void enabledChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool handleKeyPress(QKeyEvent *event);
    bool handleKeyRelease(QKeyEvent *event);
    QObject *openMenu() const;
    void resetAltTracking();

    static bool isMenu(QObject *object);
    static bool isOpenMenu(QObject *object);
    static bool isEnabledMenuItem(QObject *object);
    static bool isAltMnemonicKeyPress(const QKeyEvent &event);
    static QQuickItem *itemAt(QObject *menu, int index);
    static QObject *menuItemForMnemonic(QObject *menu, const QKeyEvent &event);
    static bool itemMatchesMnemonic(QObject *item, const QKeyEvent &event);
    static bool clickMenuItem(QObject *item);

    QPointer<QObject> m_menu;
    bool m_enabled = true;
    bool m_altPressedInOpenMenu = false;
};

#endif
