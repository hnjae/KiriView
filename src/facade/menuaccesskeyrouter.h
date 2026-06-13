// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MENUACCESSKEYROUTER_H
#define KIRIVIEW_MENUACCESSKEYROUTER_H

#include "application/menuaccesskeyrouterruntime.h"

#include <QObject>
#include <QtQml/qqmlregistration.h>

class QEvent;

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
    kiriview::MenuAccessKeyRouterRuntime m_runtime;
};

#endif
