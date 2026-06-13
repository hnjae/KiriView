// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_POWERPROFILEMONITOR_H
#define KIRIVIEW_POWERPROFILEMONITOR_H

#include "powerprofilemonitorstate.h"
#include "powersaverprovider.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <functional>

namespace kiriview {
using PowerProfilePortalReader = std::function<QVariantList()>;
using PowerProfilePortalSubscription = std::function<void(QObject *)>;

struct PowerProfileMonitorRuntime {
    PowerProfilePortalReader readPowerSaverEnabled;
    PowerProfilePortalSubscription subscribePropertiesChanged;
};

class PowerProfileMonitor final : public QObject, public PowerSaverStateMonitor
{
    Q_OBJECT

public:
    explicit PowerProfileMonitor(
        QObject *parent = nullptr, PowerSaverChangedCallback callback = {});
    PowerProfileMonitor(
        QObject *parent, PowerSaverChangedCallback callback, PowerProfileMonitorRuntime runtime);

    bool powerSaverEnabled() const override;

private Q_SLOTS:
    void handlePropertiesChanged(const QString &interfaceName, const QVariantMap &changedProperties,
        const QStringList &invalidatedProperties);

private:
    void refreshPowerSaverEnabled();
    void applyPlan(PowerProfileMonitorPlan plan);

    PowerSaverChangedCallback m_callback;
    PowerProfileMonitorRuntime m_runtime;
    PowerProfileMonitorState m_state;
};

PowerProfileMonitorRuntime defaultPowerProfileMonitorRuntime();
PowerProfileMonitorRuntime powerProfileMonitorRuntimeWithDefaults(
    PowerProfileMonitorRuntime runtime);
}

#endif
