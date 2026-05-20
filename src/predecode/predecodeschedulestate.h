// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODESCHEDULESTATE_H
#define KIRIVIEW_PREDECODESCHEDULESTATE_H

#include "imageasyncticket.h"
#include "predecodedimage.h"
#include "predecodepolicy.h"

#include <QtGlobal>
#include <optional>

namespace KiriView {
struct PredecodeScheduleContext {
    DisplayedPredecodeImage primaryImage;
    std::optional<DisplayedPredecodeImage> secondaryImage;
    ImageFirstDisplayDecodeContext firstDisplayContext;
    int pageIndex = -1;
};

struct PredecodePendingSchedule {
    PredecodeScheduleContext context;
    quint64 generation = 0;
};

struct PredecodeScheduleEffectPlan {
    std::optional<PredecodeScheduleContext> cacheDisplayedContext;
    std::optional<PredecodePendingSchedule> pendingSchedule;
    bool cancelBackgroundEffects = false;
    bool clearWindowUrls = false;
    bool startDebounceTimers = false;
};

class PredecodeScheduleState final
{
public:
    PredecodeScheduleEffectPlan schedule(PredecodeScheduleContext context, qint64 monotonicMsec);
    PredecodeScheduleEffectPlan setPowerSaverEnabled(bool enabled, qint64 monotonicMsec);
    bool powerSaverEnabled() const;
    PredecodeMomentumMode momentumMode() const;

    std::optional<PredecodeScheduleContext> displayedContext() const;
    std::optional<PredecodePendingSchedule> pendingDebouncedSchedule() const;
    PredecodeScheduleEffectPlan settlePendingScheduleToNeutral();
    bool accepts(quint64 generation) const;

    void cancelBackgroundWork();
    void cancel();

private:
    void updateNavigationMomentum(int pageIndex, qint64 monotonicMsec);

    std::optional<PredecodeScheduleContext> m_displayedContext;
    std::optional<PredecodePendingSchedule> m_pendingSchedule;
    ImageAsyncTicket m_generation;
    PredecodeMomentumState m_momentumState;
    bool m_powerSaverEnabled = false;
};
}

#endif
