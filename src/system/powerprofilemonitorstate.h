// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_POWERPROFILEMONITORSTATE_H
#define KIRIVIEW_POWERPROFILEMONITORSTATE_H

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <optional>

namespace KiriView {
struct PowerProfileMonitorTransition {
    bool powerSaverChanged = false;
    bool refreshRequested = false;
};

class PowerProfileMonitorState final
{
public:
    bool powerSaverEnabled() const;

    PowerProfileMonitorTransition applyPowerSaverValue(bool enabled);
    PowerProfileMonitorTransition applyRefreshReplyArguments(const QVariantList &arguments);
    PowerProfileMonitorTransition applyPropertiesChanged(const QString &interfaceName,
        const QVariantMap &changedProperties, const QStringList &invalidatedProperties);

private:
    bool m_powerSaverEnabled = false;
};

std::optional<bool> powerSaverEnabledFromPortalValue(QVariant value);
}

#endif
