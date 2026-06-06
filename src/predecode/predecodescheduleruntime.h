// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODESCHEDULERUNTIME_H
#define KIRIVIEW_PREDECODESCHEDULERUNTIME_H

#include "async/timerscheduler.h"
#include "predecodeloadcontroller.h"
#include "predecodeschedulestate.h"
#include "system/powersaverprovider.h"

#include <QObject>
#include <functional>
#include <memory>

namespace KiriView {
class PredecodeScheduleRuntime final
{
public:
    using StartAdjacentPredecodeCallback = std::function<void(const PredecodePendingSchedule &)>;
    using CancelDomainBackgroundCallback = std::function<void()>;

    PredecodeScheduleRuntime(QObject *owner, PredecodeLoadController &loadController,
        StartAdjacentPredecodeCallback startAdjacentPredecode,
        PowerSaverProvider powerSaverProvider = {});
    PredecodeScheduleRuntime(QObject *owner, PredecodeLoadController &loadController,
        StartAdjacentPredecodeCallback startAdjacentPredecode,
        CancelDomainBackgroundCallback cancelDomainBackground,
        PowerSaverProvider powerSaverProvider = {}, TimerScheduler timerScheduler = {});

    void schedule(PredecodeScheduleContext context);
    void setPowerSaverEnabled(bool enabled);
    bool powerSaverEnabled() const;
    PredecodeMomentumMode momentumMode() const;
    bool accepts(quint64 generation) const;
    void cancel();

private:
    void dispatchSchedulePlan(const PredecodeScheduleRuntimePlan &plan);
    void dispatchScheduleOperation(const PredecodeScheduleOperation &operation);
    void startDebouncedPredecode();
    void scheduleSettledNeutralPredecode();
    void cancelBackgroundRuntime();
    qint64 currentMonotonicMsec() const;

    PredecodeLoadController &m_loadController;
    StartAdjacentPredecodeCallback m_startAdjacentPredecode;
    CancelDomainBackgroundCallback m_cancelDomainBackground;
    TimerScheduler m_timerScheduler;
    std::unique_ptr<PowerSaverStateMonitor> m_powerSaverMonitor;
    PredecodeScheduleState m_scheduleState;
    std::unique_ptr<RuntimeTimerHandle> m_debounceTimer;
    std::unique_ptr<RuntimeTimerHandle> m_neutralTimer;
};
}

#endif
