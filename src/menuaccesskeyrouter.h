// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MENUACCESSKEYROUTER_H
#define KIRIVIEW_MENUACCESSKEYROUTER_H

#include "menuaccesskeymenuruntime.h"
#include "menuaccesskeysessionstate.h"

#include <QMetaObject>
#include <QObject>
#include <QtQml/qqmlregistration.h>

class QEvent;
class QKeyEvent;

// Qt Quick Controls does not give KiriView's toolbar popup menu and menubar the
// same post-open access-key behavior out of the box. This shim keeps that
// behavior local: Alt/plain mnemonics route through the canonical Controls.Menu
// instances while MenuActionItem owns the visible underline state.
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

private Q_SLOTS:
    void clearMenuAccessKeys();

private:
    bool handleKeyPress(QKeyEvent *event);
    bool handleKeyRelease(QKeyEvent *event);
    bool handleShortcutOverride(QKeyEvent *event);
    bool routeOpenMenuKey(QKeyEvent *event, KiriView::MenuAccessKeyRoutingPhase phase);
    KiriView::MenuAccessKeyInputKind inputKind(const QKeyEvent &event) const;
    QObject *openMenuOrClearAccessKeys();
    void applySessionTransition(KiriView::MenuAccessKeySessionTransition transition);
    bool executeRoutePlan(QKeyEvent *event, KiriView::MenuAccessKeyRoutePlan plan);
    void clearAccessKeySessionVisuals();
    void resetAltTracking();

    KiriView::MenuAccessKeyMenuRuntime m_menuRuntime;
    QMetaObject::Connection m_menuClosedConnection;
    KiriView::MenuAccessKeySessionState m_accessKeySession;
    bool m_enabled = true;
};

#endif
