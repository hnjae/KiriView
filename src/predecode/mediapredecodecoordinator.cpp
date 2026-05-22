// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodecoordinator.h"

#include "location/imageurl.h"
#include "predecodewindowplan.h"

#include <QThread>
#include <optional>
#include <type_traits>
#include <utility>

namespace {
template <typename> inline constexpr bool alwaysFalse = false;

bool validMediaPredecodeContext(const KiriView::MediaPredecodeCoordinator::Context &context)
{
    return KiriView::normalizedValidImageUrl(context.currentUrl).has_value();
}
}

namespace KiriView {
MediaPredecodeCoordinator::MediaPredecodeCoordinator(QObject *parent)
    : MediaPredecodeCoordinator(parent, ImageDecodeDependencies {})
{
}

MediaPredecodeCoordinator::MediaPredecodeCoordinator(QObject *parent,
    ImageDecodeDependencies decodeDependencies, PowerSaverProvider powerSaverProvider)
    : QObject(parent)
    , m_loadController(this, std::move(decodeDependencies))
{
    m_monotonicClock.start();
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(predecodeDebounceMsec());
    m_neutralTimer.setSingleShot(true);
    m_neutralTimer.setInterval(predecodeNeutralRefreshMsec());

    QObject::connect(
        &m_debounceTimer, &QTimer::timeout, this, [this]() { startDebouncedPredecode(); });
    QObject::connect(
        &m_neutralTimer, &QTimer::timeout, this, [this]() { scheduleSettledNeutralPredecode(); });

    powerSaverProvider = powerSaverProviderWithDefault(std::move(powerSaverProvider));
    if (powerSaverProvider.monitor) {
        m_powerSaverMonitor = powerSaverProvider.monitor(
            this, [this](bool enabled) { setPowerSaverEnabled(enabled); });
    }
    if (m_powerSaverMonitor != nullptr) {
        setPowerSaverEnabled(m_powerSaverMonitor->powerSaverEnabled());
    }
}

void MediaPredecodeCoordinator::schedule(Context context)
{
    if (!validMediaPredecodeContext(context)) {
        dispatchSchedulePlan(
            m_scheduleState.schedule(PredecodeScheduleContext {}, currentMonotonicMsec()));
        return;
    }

    m_currentCandidates = context.candidates;
    dispatchSchedulePlan(
        m_scheduleState.schedule(scheduleContext(context), currentMonotonicMsec()));
}

void MediaPredecodeCoordinator::setPowerSaverEnabled(bool enabled)
{
    dispatchSchedulePlan(m_scheduleState.setPowerSaverEnabled(enabled, currentMonotonicMsec()));
}

bool MediaPredecodeCoordinator::powerSaverEnabled() const
{
    return m_scheduleState.powerSaverEnabled();
}

void MediaPredecodeCoordinator::dispatchSchedulePlan(const PredecodeScheduleRuntimePlan &plan)
{
    for (const PredecodeScheduleOperation &operation : plan) {
        dispatchScheduleOperation(operation);
    }
}

void MediaPredecodeCoordinator::dispatchScheduleOperation(
    const PredecodeScheduleOperation &operation)
{
    std::visit(
        [this](const auto &payload) {
            using Operation = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Operation, CancelBackgroundPredecodeOperation>) {
                cancelBackgroundRuntime();
            } else if constexpr (std::is_same_v<Operation,
                                     CacheDisplayedPredecodeContextOperation>) {
                m_loadController.cacheDisplayedImages(payload.images);
            } else if constexpr (std::is_same_v<Operation, ClearPredecodeWindowUrlsOperation>) {
                m_loadController.clearWindowUrls();
            } else if constexpr (std::is_same_v<Operation, StartPredecodeDebounceOperation>) {
                m_debounceTimer.start();
                m_neutralTimer.start();
            } else if constexpr (std::is_same_v<Operation, StartAdjacentPredecodeOperation>) {
                startPredecodeWindow(payload.schedule);
            } else {
                static_assert(alwaysFalse<Operation>, "Unhandled predecode schedule operation");
            }
        },
        operation);
}

void MediaPredecodeCoordinator::startDebouncedPredecode()
{
    const std::optional<PredecodePendingSchedule> pendingSchedule
        = m_scheduleState.pendingDebouncedSchedule();
    if (!pendingSchedule.has_value()) {
        return;
    }

    startPredecodeWindow(*pendingSchedule);
}

void MediaPredecodeCoordinator::scheduleSettledNeutralPredecode()
{
    dispatchSchedulePlan(m_scheduleState.settlePendingScheduleToNeutral());
}

void MediaPredecodeCoordinator::startPredecodeWindow(const PredecodePendingSchedule &schedule)
{
    if (!m_scheduleState.accepts(schedule.generation)) {
        return;
    }

    const PredecodeWindowPlan plan = predecodeWindowPlanForMediaCandidates(
        schedule.context.currentLocation.imageUrl(), m_currentCandidates, policyInput());
    m_loadController.startWindowLoads(PredecodeLoadWindow {
        schedule.context.currentLocation.imageUrl(),
        plan.archiveDocument,
        plan.urls,
        schedule.context.displayedImages,
        schedule.context.firstDisplayContext,
        schedule.generation,
        plan.parallelLimit,
    });
}

void MediaPredecodeCoordinator::cancelBackgroundRuntime()
{
    m_debounceTimer.stop();
    m_neutralTimer.stop();
    m_loadController.cancelBackgroundWork();
}

PredecodeScheduleContext MediaPredecodeCoordinator::scheduleContext(const Context &context) const
{
    const std::optional<std::size_t> currentIndex
        = mediaNavigationCandidateIndex(context.candidates, context.currentUrl);
    return PredecodeScheduleContext {
        DisplayedImageLocation::fromUrl(context.currentUrl),
        context.displayedImages,
        context.firstDisplayContext,
        currentIndex.has_value() ? static_cast<int>(*currentIndex) : -1,
    };
}

PredecodePolicyInput MediaPredecodeCoordinator::policyInput() const
{
    return PredecodePolicyInput {
        PredecodeDocumentKind::Regular,
        m_scheduleState.momentumMode(),
        m_scheduleState.powerSaverEnabled(),
        QThread::idealThreadCount(),
    };
}

qint64 MediaPredecodeCoordinator::currentMonotonicMsec() const
{
    return m_monotonicClock.isValid() ? m_monotonicClock.elapsed() : 0;
}

void MediaPredecodeCoordinator::cancel()
{
    cancelBackgroundRuntime();
    m_scheduleState.cancel();
    m_currentCandidates.clear();
}

void MediaPredecodeCoordinator::clear()
{
    cancel();
    m_loadController.clear();
}

std::optional<PredecodedImage> MediaPredecodeCoordinator::findPredecodedImage(const QUrl &url) const
{
    return m_loadController.findPredecodedImage(url);
}
}
