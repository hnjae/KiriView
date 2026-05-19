// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodeschedulestate.h"

namespace {
bool validPredecodeScheduleContext(const KiriView::PredecodeScheduleContext &context)
{
    return context.primaryImage.hasLocation() && context.primaryImage.hasStaticImage();
}
}

namespace KiriView {
PredecodeScheduleEffectPlan PredecodeScheduleState::schedule(
    PredecodeScheduleContext context, qint64 monotonicMsec)
{
    cancelBackgroundWork();
    PredecodeScheduleEffectPlan plan;
    plan.cancelBackgroundEffects = true;

    if (!validPredecodeScheduleContext(context)) {
        return plan;
    }

    updateNavigationMomentum(context.pageIndex, monotonicMsec);
    const quint64 generation = m_generation.next();
    m_displayedContext = context;
    plan.cacheDisplayedContext = context;

    if (!m_powerSaverEnabled) {
        plan.pendingSchedule = PredecodePendingSchedule { context, generation };
        m_pendingSchedule = plan.pendingSchedule;
        plan.startDebounceTimers = true;
    } else {
        plan.clearWindowUrls = true;
    }

    return plan;
}

PredecodeScheduleEffectPlan PredecodeScheduleState::setPowerSaverEnabled(
    bool enabled, qint64 monotonicMsec)
{
    if (m_powerSaverEnabled == enabled) {
        return {};
    }

    m_powerSaverEnabled = enabled;
    if (enabled) {
        cancelBackgroundWork();
        PredecodeScheduleEffectPlan plan;
        plan.cancelBackgroundEffects = true;
        plan.clearWindowUrls = true;
        return plan;
    }

    const std::optional<PredecodeScheduleContext> displayed = m_displayedContext;
    if (!displayed.has_value()) {
        return {};
    }

    return schedule(*displayed, monotonicMsec);
}

bool PredecodeScheduleState::powerSaverEnabled() const { return m_powerSaverEnabled; }

PredecodeMomentumMode PredecodeScheduleState::momentumMode() const { return m_momentumState.mode; }

std::optional<PredecodeScheduleContext> PredecodeScheduleState::displayedContext() const
{
    return m_displayedContext;
}

std::optional<PredecodePendingSchedule> PredecodeScheduleState::pendingDebouncedSchedule() const
{
    if (!m_pendingSchedule.has_value() || !m_generation.accepts(m_pendingSchedule->generation)) {
        return std::nullopt;
    }

    return m_pendingSchedule;
}

PredecodeScheduleEffectPlan PredecodeScheduleState::settlePendingScheduleToNeutral()
{
    if (!m_pendingSchedule.has_value() || m_momentumState.mode == PredecodeMomentumMode::Neutral) {
        return {};
    }

    m_momentumState.mode = PredecodeMomentumMode::Neutral;
    m_pendingSchedule->generation = m_generation.next();
    PredecodeScheduleEffectPlan plan;
    plan.pendingSchedule = m_pendingSchedule;
    plan.cancelBackgroundEffects = true;
    return plan;
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
    m_displayedContext.reset();
    m_momentumState = {};
}

void PredecodeScheduleState::updateNavigationMomentum(int pageIndex, qint64 monotonicMsec)
{
    m_momentumState = predecodeUpdatedMomentumState(m_momentumState, pageIndex, monotonicMsec);
}
}
