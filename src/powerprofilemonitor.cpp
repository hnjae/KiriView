// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "powerprofilemonitor.h"

#include "imagecallback.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <QVariant>
#include <optional>
#include <utility>

namespace {
constexpr auto portalService = "org.freedesktop.portal.Desktop";
constexpr auto portalPath = "/org/freedesktop/portal/desktop";
constexpr auto dbusPropertiesInterface = "org.freedesktop.DBus.Properties";
constexpr auto powerProfileMonitorInterface = "org.freedesktop.portal.PowerProfileMonitor";
constexpr auto powerSaverEnabledProperty = "power-saver-enabled";

std::optional<bool> boolFromDBusVariant(QVariant value)
{
    if (value.canConvert<QDBusVariant>()) {
        value = value.value<QDBusVariant>().variant();
    }
    if (!value.canConvert<bool>()) {
        return std::nullopt;
    }

    return value.toBool();
}
}

namespace KiriView {
PowerProfileMonitor::PowerProfileMonitor(QObject *parent, PowerSaverChangedCallback callback)
    : QObject(parent)
    , m_callback(std::move(callback))
{
    refreshPowerSaverEnabled();
    QDBusConnection::sessionBus().connect(portalService, portalPath, dbusPropertiesInterface,
        QStringLiteral("PropertiesChanged"), this,
        SLOT(handlePropertiesChanged(QString, QVariantMap, QStringList)));
}

bool PowerProfileMonitor::powerSaverEnabled() const { return m_powerSaverEnabled; }

void PowerProfileMonitor::handlePropertiesChanged(const QString &interfaceName,
    const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (interfaceName != QLatin1String(powerProfileMonitorInterface)) {
        return;
    }

    const auto changedProperty = changedProperties.constFind(powerSaverEnabledProperty);
    if (changedProperty != changedProperties.cend()) {
        setPowerSaverEnabled(boolFromDBusVariant(*changedProperty).value_or(false));
        return;
    }

    if (invalidatedProperties.contains(QLatin1String(powerSaverEnabledProperty))) {
        refreshPowerSaverEnabled();
    }
}

void PowerProfileMonitor::refreshPowerSaverEnabled()
{
    QDBusMessage message = QDBusMessage::createMethodCall(
        portalService, portalPath, dbusPropertiesInterface, QStringLiteral("Get"));
    message << QString::fromLatin1(powerProfileMonitorInterface)
            << QString::fromLatin1(powerSaverEnabledProperty);

    const QDBusMessage reply = QDBusConnection::sessionBus().call(message);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().isEmpty()) {
        setPowerSaverEnabled(false);
        return;
    }

    setPowerSaverEnabled(boolFromDBusVariant(reply.arguments().first()).value_or(false));
}

void PowerProfileMonitor::setPowerSaverEnabled(bool enabled)
{
    if (m_powerSaverEnabled == enabled) {
        return;
    }

    m_powerSaverEnabled = enabled;
    invokeIfSet(m_callback, enabled);
}
}
