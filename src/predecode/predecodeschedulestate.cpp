// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodeschedulestate.h"

namespace {
bool validPredecodeScheduleContext(const KiriView::PredecodeScheduleContext &context)
{
    return !context.currentLocation.isEmpty();
}
}

namespace KiriView {
PredecodeScheduleRuntimePlan PredecodeScheduleState::schedule(
    PredecodeScheduleContext context, qint64 monotonicMsec)
{
    cancelBackgroundWork();
    PredecodeScheduleRuntimePlan plan { CancelBackgroundPredecodeOperation {} };

    if (!validPredecodeScheduleContext(context)) {
        return plan;
    }

    updateNavigationMomentum(context.pageIndex, monotonicMsec);
    const quint64 generation = m_generation.next();
    m_currentContext = context;
    plan.push_back(CacheDisplayedPredecodeContextOperation { context.displayedImages });

    if (!m_powerSaverEnabled) {
        m_pendingSchedule = PredecodePendingSchedule { context, generation };
        plan.push_back(StartPredecodeDebounceOperation { *m_pendingSchedule });
    } else {
        plan.push_back(ClearPredecodeWindowUrlsOperation {});
    }

    return plan;
}

PredecodeScheduleRuntimePlan PredecodeScheduleState::setPowerSaverEnabled(
    bool enabled, qint64 monotonicMsec)
{
    if (m_powerSaverEnabled == enabled) {
        return {};
    }

    m_powerSaverEnabled = enabled;
    if (enabled) {
        cancelBackgroundWork();
        return {
            CancelBackgroundPredecodeOperation {},
            ClearPredecodeWindowUrlsOperation {},
        };
    }

    const std::optional<PredecodeScheduleContext> current = m_currentContext;
    if (!current.has_value()) {
        return {};
    }

    return schedule(*current, monotonicMsec);
}

bool PredecodeScheduleState::powerSaverEnabled() const { return m_powerSaverEnabled; }

PredecodeMomentumMode PredecodeScheduleState::momentumMode() const { return m_momentumState.mode; }

std::optional<PredecodeScheduleContext> PredecodeScheduleState::currentContext() const
{
    return m_currentContext;
}

std::optional<PredecodePendingSchedule> PredecodeScheduleState::pendingDebouncedSchedule() const
{
    if (!m_pendingSchedule.has_value() || !m_generation.accepts(m_pendingSchedule->generation)) {
        return std::nullopt;
    }

    return m_pendingSchedule;
}

PredecodeScheduleRuntimePlan PredecodeScheduleState::settlePendingScheduleToNeutral()
{
    if (!m_pendingSchedule.has_value() || m_momentumState.mode == PredecodeMomentumMode::Neutral) {
        return {};
    }

    m_momentumState.mode = PredecodeMomentumMode::Neutral;
    m_pendingSchedule->generation = m_generation.next();
    return {
        CancelBackgroundPredecodeOperation {},
        StartAdjacentPredecodeOperation { *m_pendingSchedule },
    };
}

bool PredecodeScheduleState::accepts(quint64 generation) const
{
    return m_generation.accepts(generation);
}

void PredecodeScheduleState::cancelBackgroundWork()
{
    m_generation.invalidate();
    m_pendingSchedule.reset();
}

void PredecodeScheduleState::cancel()
{
    cancelBackgroundWork();
    m_currentContext.reset();
    m_momentumState = {};
}

void PredecodeScheduleState::updateNavigationMomentum(int pageIndex, qint64 monotonicMsec)
{
    m_momentumState = predecodeUpdatedMomentumState(m_momentumState, pageIndex, monotonicMsec);
}
}
