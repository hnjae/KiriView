// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "powerprofilemonitorstate.h"

#include <QDBusVariant>
#include <QLatin1String>
#include <QVariant>

namespace {
constexpr auto powerProfileMonitorInterface = "org.freedesktop.portal.PowerProfileMonitor";
constexpr auto powerSaverEnabledProperty = "power-saver-enabled";
}

namespace KiriView {
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

bool PowerProfileMonitorState::powerSaverEnabled() const { return m_powerSaverEnabled; }

PowerProfileMonitorTransition PowerProfileMonitorState::applyPowerSaverValue(bool enabled)
{
    if (m_powerSaverEnabled == enabled) {
        return {};
    }

    m_powerSaverEnabled = enabled;
    return PowerProfileMonitorTransition { true, false };
}

PowerProfileMonitorTransition PowerProfileMonitorState::applyRefreshReplyArguments(
    const QVariantList &arguments)
{
    if (arguments.isEmpty()) {
        return applyPowerSaverValue(false);
    }

    return applyPowerSaverValue(
        powerSaverEnabledFromPortalValue(arguments.first()).value_or(false));
}

PowerProfileMonitorTransition PowerProfileMonitorState::applyPropertiesChanged(
    const QString &interfaceName, const QVariantMap &changedProperties,
    const QStringList &invalidatedProperties)
{
    if (interfaceName != QLatin1String(powerProfileMonitorInterface)) {
        return {};
    }

    const auto changedProperty = changedProperties.constFind(powerSaverEnabledProperty);
    if (changedProperty != changedProperties.cend()) {
        return applyPowerSaverValue(
            powerSaverEnabledFromPortalValue(*changedProperty).value_or(false));
    }

    if (invalidatedProperties.contains(QLatin1String(powerSaverEnabledProperty))) {
        return PowerProfileMonitorTransition { false, true };
    }

    return {};
}
}
