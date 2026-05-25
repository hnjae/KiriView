// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodescheduleruntime.h"

#include "predecodelogging.h"

#include <QDebug>
#include <type_traits>
#include <utility>

namespace {
template <typename> inline constexpr bool alwaysFalse = false;
}

namespace KiriView {
PredecodeScheduleRuntime::PredecodeScheduleRuntime(QObject *owner,
    PredecodeLoadController &loadController, StartAdjacentPredecodeCallback startAdjacentPredecode,
    PowerSaverProvider powerSaverProvider)
    : PredecodeScheduleRuntime(owner, loadController, std::move(startAdjacentPredecode), {},
          std::move(powerSaverProvider))
{
}

PredecodeScheduleRuntime::PredecodeScheduleRuntime(QObject *owner,
    PredecodeLoadController &loadController, StartAdjacentPredecodeCallback startAdjacentPredecode,
    CancelDomainBackgroundCallback cancelDomainBackground, PowerSaverProvider powerSaverProvider)
    : m_loadController(loadController)
    , m_startAdjacentPredecode(std::move(startAdjacentPredecode))
    , m_cancelDomainBackground(std::move(cancelDomainBackground))
{
    m_monotonicClock.start();
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(predecodeDebounceMsec());
    m_neutralTimer.setSingleShot(true);
    m_neutralTimer.setInterval(predecodeNeutralRefreshMsec());

    QObject::connect(
        &m_debounceTimer, &QTimer::timeout, owner, [this]() { startDebouncedPredecode(); });
    QObject::connect(
        &m_neutralTimer, &QTimer::timeout, owner, [this]() { scheduleSettledNeutralPredecode(); });

    powerSaverProvider = powerSaverProviderWithDefault(std::move(powerSaverProvider));
    if (powerSaverProvider.monitor) {
        m_powerSaverMonitor = powerSaverProvider.monitor(
            owner, [this](bool enabled) { setPowerSaverEnabled(enabled); });
    }
    if (m_powerSaverMonitor != nullptr) {
        setPowerSaverEnabled(m_powerSaverMonitor->powerSaverEnabled());
    }
}

void PredecodeScheduleRuntime::schedule(PredecodeScheduleContext context)
{
    qCDebug(kiriviewPredecodeLog) << "schedule requested"
                                  << "url" << context.currentLocation.imageUrl() << "pageIndex"
                                  << context.pageIndex << "displayedImages"
                                  << context.displayedImages.size() << "powerSaver"
                                  << m_scheduleState.powerSaverEnabled();
    dispatchSchedulePlan(m_scheduleState.schedule(std::move(context), currentMonotonicMsec()));
}

void PredecodeScheduleRuntime::setPowerSaverEnabled(bool enabled)
{
    qCDebug(kiriviewPredecodeLog) << "power saver changed"
                                  << "enabled" << enabled;
    dispatchSchedulePlan(m_scheduleState.setPowerSaverEnabled(enabled, currentMonotonicMsec()));
}

bool PredecodeScheduleRuntime::powerSaverEnabled() const
{
    return m_scheduleState.powerSaverEnabled();
}

PredecodeMomentumMode PredecodeScheduleRuntime::momentumMode() const
{
    return m_scheduleState.momentumMode();
}

bool PredecodeScheduleRuntime::accepts(quint64 generation) const
{
    return m_scheduleState.accepts(generation);
}

void PredecodeScheduleRuntime::dispatchSchedulePlan(const PredecodeScheduleRuntimePlan &plan)
{
    for (const PredecodeScheduleOperation &operation : plan) {
        dispatchScheduleOperation(operation);
    }
}

void PredecodeScheduleRuntime::dispatchScheduleOperation(
    const PredecodeScheduleOperation &operation)
{
    std::visit(
        [this](const auto &payload) {
            using Operation = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Operation, CancelBackgroundPredecodeOperation>) {
                qCDebug(kiriviewPredecodeLog) << "cancel background predecode";
                cancelBackgroundRuntime();
            } else if constexpr (std::is_same_v<Operation,
                                     CacheDisplayedPredecodeContextOperation>) {
                qCDebug(kiriviewPredecodeLog) << "cache displayed predecode context"
                                              << "images" << payload.images.size();
                m_loadController.cacheDisplayedImages(payload.images);
            } else if constexpr (std::is_same_v<Operation, ClearPredecodeWindowUrlsOperation>) {
                qCDebug(kiriviewPredecodeLog) << "clear predecode window urls";
                m_loadController.clearWindowUrls();
            } else if constexpr (std::is_same_v<Operation, StartPredecodeDebounceOperation>) {
                qCDebug(kiriviewPredecodeLog)
                    << "start predecode debounce"
                    << "generation" << payload.schedule.generation << "url"
                    << payload.schedule.context.currentLocation.imageUrl();
                m_debounceTimer.start();
                m_neutralTimer.start();
            } else if constexpr (std::is_same_v<Operation, StartAdjacentPredecodeOperation>) {
                qCDebug(kiriviewPredecodeLog)
                    << "start adjacent predecode"
                    << "generation" << payload.schedule.generation << "url"
                    << payload.schedule.context.currentLocation.imageUrl();
                m_startAdjacentPredecode(payload.schedule);
            } else {
                static_assert(alwaysFalse<Operation>, "Unhandled predecode schedule operation");
            }
        },
        operation);
}

void PredecodeScheduleRuntime::startDebouncedPredecode()
{
    const std::optional<PredecodePendingSchedule> pendingSchedule
        = m_scheduleState.pendingDebouncedSchedule();
    if (!pendingSchedule.has_value()) {
        qCDebug(kiriviewPredecodeLog) << "debounced predecode skipped"
                                      << "reason"
                                      << "no-pending-schedule";
        return;
    }

    qCDebug(kiriviewPredecodeLog) << "debounced predecode fired"
                                  << "generation" << pendingSchedule->generation << "url"
                                  << pendingSchedule->context.currentLocation.imageUrl();
    m_startAdjacentPredecode(*pendingSchedule);
}

void PredecodeScheduleRuntime::scheduleSettledNeutralPredecode()
{
    qCDebug(kiriviewPredecodeLog) << "settle predecode momentum";
    dispatchSchedulePlan(m_scheduleState.settlePendingScheduleToNeutral());
}

void PredecodeScheduleRuntime::cancelBackgroundRuntime()
{
    m_debounceTimer.stop();
    m_neutralTimer.stop();
    if (m_cancelDomainBackground) {
        m_cancelDomainBackground();
    }
    m_loadController.cancelBackgroundWork();
}

qint64 PredecodeScheduleRuntime::currentMonotonicMsec() const
{
    return m_monotonicClock.isValid() ? m_monotonicClock.elapsed() : 0;
}

void PredecodeScheduleRuntime::cancel()
{
    qCDebug(kiriviewPredecodeLog) << "predecode runtime cancel";
    cancelBackgroundRuntime();
    m_scheduleState.cancel();
}
}
