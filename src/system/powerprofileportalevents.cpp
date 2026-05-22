// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "powerprofileportalevents.h"

#include <QDBusVariant>
#include <QLatin1String>

namespace {
constexpr auto powerProfileMonitorInterface = "org.freedesktop.portal.PowerProfileMonitor";
constexpr auto powerSaverEnabledProperty = "power-saver-enabled";
}

namespace KiriView {
PowerProfileMonitorEvent powerProfileMonitorEventFromRefreshReply(const QVariantList &arguments)
{
    if (arguments.isEmpty()) {
        return PowerProfileMonitorEvent::powerSaverValue(false);
    }

    return PowerProfileMonitorEvent::powerSaverValue(
        powerSaverEnabledFromPortalValue(arguments.first()).value_or(false));
}

PowerProfileMonitorEvent powerProfileMonitorEventFromPropertiesChanged(const QString &interfaceName,
    const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    if (interfaceName != QLatin1String(powerProfileMonitorInterface)) {
        return PowerProfileMonitorEvent::ignore();
    }

    const auto changedProperty = changedProperties.constFind(powerSaverEnabledProperty);
    if (changedProperty != changedProperties.cend()) {
        return PowerProfileMonitorEvent::powerSaverValue(
            powerSaverEnabledFromPortalValue(*changedProperty).value_or(false));
    }

    if (invalidatedProperties.contains(QLatin1String(powerSaverEnabledProperty))) {
        return PowerProfileMonitorEvent::powerSaverInvalidated();
    }

    return PowerProfileMonitorEvent::ignore();
}

std::optional<bool> powerSaverEnabledFromPortalValue(QVariant value)
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
