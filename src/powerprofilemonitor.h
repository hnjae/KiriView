// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_POWERPROFILEMONITOR_H
#define KIRIVIEW_POWERPROFILEMONITOR_H

#include "powerprofilemonitorstate.h"
#include "powersaverprovider.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace KiriView {
class PowerProfileMonitor final : public QObject, public PowerSaverStateMonitor
{
    Q_OBJECT

public:
    explicit PowerProfileMonitor(
        QObject *parent = nullptr, PowerSaverChangedCallback callback = {});

    bool powerSaverEnabled() const override;

private Q_SLOTS:
    void handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties,
        const QStringList &invalidatedProperties);

private:
    void refreshPowerSaverEnabled();
    void applyTransition(PowerProfileMonitorTransition transition);

    PowerSaverChangedCallback m_callback;
    PowerProfileMonitorState m_state;
};
}

#endif
