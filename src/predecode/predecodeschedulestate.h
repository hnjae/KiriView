// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODESCHEDULESTATE_H
#define KIRIVIEW_PREDECODESCHEDULESTATE_H

#include "async/imageasyncticket.h"
#include "predecodedimage.h"
#include "predecodepolicy.h"

#include <QtGlobal>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace KiriView {
class PredecodeSchedulePayload
{
public:
    virtual ~PredecodeSchedulePayload() = default;
};

struct PredecodeScheduleContext {
    DisplayedImageLocation currentLocation;
    std::vector<DisplayedPredecodeImage> displayedImages;
    ImageFirstDisplayDecodeContext firstDisplayContext;
    int pageIndex = -1;
    std::shared_ptr<const PredecodeSchedulePayload> payload;
};

struct PredecodePendingSchedule {
    PredecodeScheduleContext context;
    quint64 generation = 0;
};

template <typename Payload>
const Payload *predecodeSchedulePayload(const PredecodeScheduleContext &context)
{
    return context.payload == nullptr ? nullptr
                                      : dynamic_cast<const Payload *>(context.payload.get());
}

template <typename Payload>
const Payload *predecodeSchedulePayload(const PredecodePendingSchedule &schedule)
{
    return predecodeSchedulePayload<Payload>(schedule.context);
}

struct CancelBackgroundPredecodeOperation {
};
struct CacheDisplayedPredecodeContextOperation {
    std::vector<DisplayedPredecodeImage> images;
};
struct ClearPredecodeWindowUrlsOperation {
};
struct StartPredecodeDebounceOperation {
    PredecodePendingSchedule schedule;
};
struct StartAdjacentPredecodeOperation {
    PredecodePendingSchedule schedule;
};

using PredecodeScheduleOperation = std::variant<CancelBackgroundPredecodeOperation,
    CacheDisplayedPredecodeContextOperation, ClearPredecodeWindowUrlsOperation,
    StartPredecodeDebounceOperation, StartAdjacentPredecodeOperation>;
using PredecodeScheduleRuntimePlan = std::vector<PredecodeScheduleOperation>;

class PredecodeScheduleState final
{
public:
    PredecodeScheduleRuntimePlan schedule(PredecodeScheduleContext context, qint64 monotonicMsec);
    PredecodeScheduleRuntimePlan setPowerSaverEnabled(bool enabled, qint64 monotonicMsec);
    bool powerSaverEnabled() const;
    PredecodeMomentumMode momentumMode() const;

    std::optional<PredecodeScheduleContext> currentContext() const;
    std::optional<PredecodePendingSchedule> pendingDebouncedSchedule() const;
    PredecodeScheduleRuntimePlan settlePendingScheduleToNeutral();
    bool accepts(quint64 generation) const;

    void cancelBackgroundWork();
    void cancel();

private:
    void updateNavigationMomentum(int pageIndex, qint64 monotonicMsec);

    std::optional<PredecodeScheduleContext> m_currentContext;
    std::optional<PredecodePendingSchedule> m_pendingSchedule;
    ImageAsyncTicket m_generation;
    PredecodeMomentumState m_momentumState;
    bool m_powerSaverEnabled = false;
};
}

#endif
