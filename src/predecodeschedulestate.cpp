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
std::optional<PredecodeScheduleUpdate> PredecodeScheduleState::schedule(
    PredecodeScheduleContext context, qint64 monotonicMsec)
{
    cancelBackgroundWork();
    if (!validPredecodeScheduleContext(context)) {
        return std::nullopt;
    }

    updateNavigationMomentum(context.pageIndex, monotonicMsec);
    const quint64 generation = m_generation.next();
    m_displayedContext = context;

    std::optional<PredecodePendingSchedule> pendingSchedule;
    if (!m_powerSaverEnabled) {
        pendingSchedule = PredecodePendingSchedule { context, generation };
        m_pendingSchedule = pendingSchedule;
    }

    return PredecodeScheduleUpdate { context, std::move(pendingSchedule), m_powerSaverEnabled };
}

bool PredecodeScheduleState::setPowerSaverEnabled(bool enabled)
{
    if (m_powerSaverEnabled == enabled) {
        return false;
    }

    m_powerSaverEnabled = enabled;
    return true;
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

std::optional<PredecodePendingSchedule> PredecodeScheduleState::settlePendingScheduleToNeutral()
{
    if (!m_pendingSchedule.has_value() || m_momentumState.mode == PredecodeMomentumMode::Neutral) {
        return std::nullopt;
    }

    m_momentumState.mode = PredecodeMomentumMode::Neutral;
    m_pendingSchedule->generation = m_generation.next();
    return m_pendingSchedule;
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
