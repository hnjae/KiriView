// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_POWERSAVERPROVIDER_H
#define KIRIVIEW_POWERSAVERPROVIDER_H

#include <functional>
#include <memory>

class QObject;

namespace KiriView {
class PowerSaverStateMonitor
{
public:
    virtual ~PowerSaverStateMonitor() = default;
    virtual bool powerSaverEnabled() const = 0;
};

using PowerSaverChangedCallback = std::function<void(bool)>;
using PowerSaverMonitorFactory
    = std::function<std::unique_ptr<PowerSaverStateMonitor>(QObject *, PowerSaverChangedCallback)>;

struct PowerSaverProvider {
    PowerSaverMonitorFactory monitor;
};

PowerSaverProvider defaultPowerSaverProvider();
PowerSaverProvider powerSaverProviderWithDefault(PowerSaverProvider provider);
}

#endif
