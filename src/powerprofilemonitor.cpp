// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "powerprofilemonitor.h"

#include "imagecallback.h"

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

bool PowerProfileMonitor::powerSaverEnabled() const { return m_state.powerSaverEnabled(); }

void PowerProfileMonitor::handlePropertiesChanged(const QString &interfaceName,
    const QVariantMap &changedProperties, const QStringList &invalidatedProperties)
{
    const PowerProfileMonitorTransition transition
        = m_state.applyPropertiesChanged(interfaceName, changedProperties, invalidatedProperties);
    applyTransition(transition);
    if (transition.refreshRequested) {
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
        applyTransition(m_state.applyRefreshReplyArguments({}));
        return;
    }

    applyTransition(m_state.applyRefreshReplyArguments(reply.arguments()));
}

void PowerProfileMonitor::applyTransition(PowerProfileMonitorTransition transition)
{
    if (!transition.powerSaverChanged) {
        return;
    }

    invokeIfSet(m_callback, m_state.powerSaverEnabled());
}
}
