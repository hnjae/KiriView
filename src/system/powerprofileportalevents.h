// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_POWERPROFILEPORTALEVENTS_H
#define KIRIVIEW_POWERPROFILEPORTALEVENTS_H

#include "powerprofilemonitorstate.h"

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <optional>

namespace KiriView {
PowerProfileMonitorEvent powerProfileMonitorEventFromRefreshReply(const QVariantList &arguments);
PowerProfileMonitorEvent powerProfileMonitorEventFromPropertiesChanged(const QString &interfaceName,
    const QVariantMap &changedProperties, const QStringList &invalidatedProperties);
std::optional<bool> powerSaverEnabledFromPortalValue(QVariant value);
}

#endif
