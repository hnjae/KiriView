// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "powerprofilemonitor.h"

#include "async/imagecallback.h"
#include "powerprofileportalevents.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QVariant>
#include <utility>

namespace {
constexpr auto portalService = "org.freedesktop.portal.Desktop";
constexpr auto portalPath = "/org/freedesktop/portal/desktop";
constexpr auto dbusPropertiesInterface = "org.freedesktop.DBus.Properties";
constexpr auto powerProfileMonitorInterface = "org.freedesktop.portal.PowerProfileMonitor";
constexpr auto powerSaverEnabledProperty = "power-saver-enabled";

QVariantList readPortalPowerSaverEnabled()
{
    QDBusMessage message = QDBusMessage::createMethodCall(
        portalService, portalPath, dbusPropertiesInterface, QStringLiteral("Get"));
    message << QString::fromLatin1(powerProfileMonitorInterface)
            << QString::fromLatin1(powerSaverEnabledProperty);

    const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() != QDBusMessage::ReplyMessage) {
        return {};
    }

    return reply.arguments();
}

void subscribePortalPowerSaverChanges(QObject* receiver)
{
    // clang-format off
    QDBusConnection::sessionBus().connect(portalService, portalPath, dbusPropertiesInterface,
        QStringLiteral("PropertiesChanged"), receiver,
        SLOT(handlePropertiesChanged(QString,QVariantMap,QStringList)));
    // clang-format on
}
}

namespace kiriview {
PowerProfileMonitor::PowerProfileMonitor(QObject* parent, PowerSaverChangedCallback callback)
    : PowerProfileMonitor(parent, std::move(callback), {})
{
}

PowerProfileMonitor::PowerProfileMonitor(
    QObject* parent, PowerSaverChangedCallback callback, PowerProfileMonitorRuntime runtime)
    : QObject(parent)
    , m_callback(std::move(callback))
    , m_runtime(powerProfileMonitorRuntimeWithDefaults(std::move(runtime)))
{
    refreshPowerSaverEnabled();
    m_runtime.subscribePropertiesChanged(this);
}

bool PowerProfileMonitor::powerSaverEnabled() const { return m_state.powerSaverEnabled(); }

void PowerProfileMonitor::handlePropertiesChanged(const QString& interfaceName,
    const QVariantMap& changedProperties, const QStringList& invalidatedProperties)
{
    const PowerProfileMonitorPlan plan
        = m_state.applyEvent(powerProfileMonitorEventFromPropertiesChanged(
            interfaceName, changedProperties, invalidatedProperties));
    applyPlan(plan);
    if (plan.refreshPowerSaverEnabled) {
        refreshPowerSaverEnabled();
    }
}

void PowerProfileMonitor::refreshPowerSaverEnabled()
{
    applyPlan(m_state.applyEvent(
        powerProfileMonitorEventFromRefreshReply(m_runtime.readPowerSaverEnabled())));
}

void PowerProfileMonitor::applyPlan(PowerProfileMonitorPlan plan)
{
    if (!plan.powerSaverChanged) {
        return;
    }

    invokeIfSet(m_callback, m_state.powerSaverEnabled());
}

PowerProfileMonitorRuntime defaultPowerProfileMonitorRuntime()
{
    return PowerProfileMonitorRuntime {
        readPortalPowerSaverEnabled,
        subscribePortalPowerSaverChanges,
    };
}

PowerProfileMonitorRuntime powerProfileMonitorRuntimeWithDefaults(
    PowerProfileMonitorRuntime runtime)
{
    PowerProfileMonitorRuntime defaults = defaultPowerProfileMonitorRuntime();
    if (!runtime.readPowerSaverEnabled) {
        runtime.readPowerSaverEnabled = std::move(defaults.readPowerSaverEnabled);
    }
    if (!runtime.subscribePropertiesChanged) {
        runtime.subscribePropertiesChanged = std::move(defaults.subscribePropertiesChanged);
    }

    return runtime;
}
}
