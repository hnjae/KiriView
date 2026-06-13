// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "powerprofilemonitorstate.h"

namespace kiriview {
PowerProfileMonitorEvent PowerProfileMonitorEvent::ignore() { return {}; }

PowerProfileMonitorEvent PowerProfileMonitorEvent::powerSaverValue(bool enabled)
{
    return PowerProfileMonitorEvent { PowerProfileMonitorEventKind::PowerSaverValue, enabled };
}

PowerProfileMonitorEvent PowerProfileMonitorEvent::powerSaverInvalidated()
{
    return PowerProfileMonitorEvent { PowerProfileMonitorEventKind::PowerSaverInvalidated, false };
}

bool PowerProfileMonitorState::powerSaverEnabled() const { return m_powerSaverEnabled; }

PowerProfileMonitorPlan PowerProfileMonitorState::applyEvent(PowerProfileMonitorEvent event)
{
    switch (event.kind) {
    case PowerProfileMonitorEventKind::Ignore:
        return {};
    case PowerProfileMonitorEventKind::PowerSaverValue:
        return applyPowerSaverValue(event.powerSaverEnabled);
    case PowerProfileMonitorEventKind::PowerSaverInvalidated:
        return PowerProfileMonitorPlan { false, true };
    }

    return {};
}

PowerProfileMonitorPlan PowerProfileMonitorState::applyPowerSaverValue(bool enabled)
{
    if (m_powerSaverEnabled == enabled) {
        return {};
    }

    m_powerSaverEnabled = enabled;
    return PowerProfileMonitorPlan { true, false };
}
}
