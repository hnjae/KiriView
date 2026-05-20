// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "powersaverprovider.h"

#include "powerprofilemonitor.h"

#include <utility>

namespace KiriView {
PowerSaverProvider defaultPowerSaverProvider()
{
    return PowerSaverProvider {
        [](QObject *parent, PowerSaverChangedCallback callback) {
            return std::make_unique<PowerProfileMonitor>(parent, std::move(callback));
        },
    };
}

PowerSaverProvider powerSaverProviderWithDefault(PowerSaverProvider provider)
{
    if (!provider.monitor) {
        provider = defaultPowerSaverProvider();
    }

    return provider;
}
}
