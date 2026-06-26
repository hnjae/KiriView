// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_POWERPROFILEMONITORSTATE_H
#define KIRIVIEW_POWERPROFILEMONITORSTATE_H

namespace kiriview {
enum class PowerProfileMonitorEventKind {
    Ignore,
    PowerSaverValue,
    PowerSaverInvalidated,
};

struct PowerProfileMonitorEvent
{
    PowerProfileMonitorEventKind kind = PowerProfileMonitorEventKind::Ignore;
    bool powerSaverEnabled = false;

    static PowerProfileMonitorEvent ignore();
    static PowerProfileMonitorEvent powerSaverValue(bool enabled);
    static PowerProfileMonitorEvent powerSaverInvalidated();
};

struct PowerProfileMonitorPlan
{
    bool powerSaverChanged = false;
    bool refreshPowerSaverEnabled = false;
};

class PowerProfileMonitorState final
{
public:
    bool powerSaverEnabled() const;

    PowerProfileMonitorPlan applyEvent(PowerProfileMonitorEvent event);

private:
    PowerProfileMonitorPlan applyPowerSaverValue(bool enabled);

    bool m_powerSaverEnabled = false;
};
}

#endif
