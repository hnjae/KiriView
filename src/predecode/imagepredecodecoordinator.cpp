// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "predecodewindowplan.h"

#include <QThread>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace KiriView {
namespace {
    template <typename> inline constexpr bool alwaysFalse = false;
}

struct ImagePredecodeCoordinator::WindowLoadContext {
    Context scheduleContext;
    quint64 generation = 0;
};

ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent)
    : ImagePredecodeCoordinator(
          parent, ImageNavigationCandidateProvider {}, ImageDecodeDependencies {})
{
}

ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent,
    ImageNavigationCandidateProvider candidateProvider, ImageDecodeDependencies decodeDependencies,
    PowerSaverProvider powerSaverProvider)
    : QObject(parent)
    , m_candidateRepository(std::move(candidateProvider))
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

void ImagePredecodeCoordinator::schedule(Context context)
{
    const PredecodeScheduleRuntimePlan plan
        = m_scheduleState.schedule(std::move(context), currentMonotonicMsec());
    dispatchSchedulePlan(plan);
}

void ImagePredecodeCoordinator::setPowerSaverEnabled(bool enabled)
{
    const PredecodeScheduleRuntimePlan plan
        = m_scheduleState.setPowerSaverEnabled(enabled, currentMonotonicMsec());
    dispatchSchedulePlan(plan);
}

bool ImagePredecodeCoordinator::powerSaverEnabled() const
{
    return m_scheduleState.powerSaverEnabled();
}

void ImagePredecodeCoordinator::dispatchSchedulePlan(const PredecodeScheduleRuntimePlan &plan)
{
    for (const PredecodeScheduleOperation &operation : plan) {
        dispatchScheduleOperation(operation);
    }
}

void ImagePredecodeCoordinator::dispatchScheduleOperation(
    const PredecodeScheduleOperation &operation)
{
    std::visit(
        [this](const auto &payload) {
            using Operation = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<Operation, CancelBackgroundPredecodeOperation>) {
                cancelBackgroundRuntime();
            } else if constexpr (std::is_same_v<Operation,
                                     CacheDisplayedPredecodeContextOperation>) {
                cacheDisplayedImages(payload.context);
            } else if constexpr (std::is_same_v<Operation, ClearPredecodeWindowUrlsOperation>) {
                m_loadController.clearWindowUrls();
            } else if constexpr (std::is_same_v<Operation, StartPredecodeDebounceOperation>) {
                m_debounceTimer.start();
                m_neutralTimer.start();
            } else if constexpr (std::is_same_v<Operation, StartAdjacentPredecodeOperation>) {
                scheduleAdjacentImagePredecode(
                    payload.schedule.context, payload.schedule.generation);
            } else {
                static_assert(alwaysFalse<Operation>, "Unhandled predecode schedule operation");
            }
        },
        operation);
}

void ImagePredecodeCoordinator::cacheDisplayedImages(const Context &context)
{
    m_loadController.cacheDisplayedImages(displayedImages(context));
}

std::vector<DisplayedPredecodeImage> ImagePredecodeCoordinator::displayedImages(
    const Context &context) const
{
    std::vector<DisplayedPredecodeImage> images;
    images.push_back(context.primaryImage);
    if (context.secondaryImage.has_value()) {
        images.push_back(*context.secondaryImage);
    }

    return images;
}

void ImagePredecodeCoordinator::startDebouncedPredecode()
{
    const std::optional<PredecodePendingSchedule> pendingSchedule
        = m_scheduleState.pendingDebouncedSchedule();
    if (!pendingSchedule.has_value()) {
        return;
    }

    scheduleAdjacentImagePredecode(pendingSchedule->context, pendingSchedule->generation);
}

void ImagePredecodeCoordinator::scheduleSettledNeutralPredecode()
{
    dispatchSchedulePlan(m_scheduleState.settlePendingScheduleToNeutral());
}

void ImagePredecodeCoordinator::scheduleAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const PredecodeWindowStartPlan plan = predecodeWindowStartPlan(PredecodeWindowPlanRequest {
        context.primaryImage.location,
        m_scheduleState.momentumMode(),
        m_scheduleState.powerSaverEnabled(),
        QThread::idealThreadCount(),
    });
    const WindowLoadContext loadContext { context, generation };
    if (!plan.shouldLoadCandidates()) {
        startPredecodeImageLoads(plan.fallbackWindow, loadContext);
        return;
    }

    m_listerJob = m_candidateRepository.loadImages(
        this, plan.candidateList->context,
        [this, loadContext, plan](const std::vector<ImageNavigationCandidate> &candidates) {
            startPredecodeImageLoads(
                predecodeWindowPlanForCandidates(plan, candidates), loadContext);
        },
        [this, loadContext, plan](
            const QString &) { startPredecodeImageLoads(plan.fallbackWindow, loadContext); });
}

void ImagePredecodeCoordinator::startPredecodeImageLoads(
    const PredecodeWindowPlan &plan, const WindowLoadContext &context)
{
    if (!m_scheduleState.accepts(context.generation)) {
        return;
    }

    m_loadController.startWindowLoads(PredecodeLoadWindow {
        context.scheduleContext.primaryImage.location.imageUrl(),
        plan.archiveDocument,
        plan.urls,
        displayedImages(context.scheduleContext),
        context.scheduleContext.firstDisplayContext,
        context.generation,
        plan.parallelLimit,
    });
}

void ImagePredecodeCoordinator::cancelBackgroundWork()
{
    m_scheduleState.cancelBackgroundWork();
    cancelBackgroundRuntime();
}

void ImagePredecodeCoordinator::cancelBackgroundRuntime()
{
    m_debounceTimer.stop();
    m_neutralTimer.stop();
    m_listerJob.cancel();
    m_loadController.cancelBackgroundWork();
}

qint64 ImagePredecodeCoordinator::currentMonotonicMsec() const
{
    return m_monotonicClock.isValid() ? m_monotonicClock.elapsed() : 0;
}

void ImagePredecodeCoordinator::cancel()
{
    cancelBackgroundRuntime();
    m_scheduleState.cancel();
}

void ImagePredecodeCoordinator::clear()
{
    cancel();
    m_loadController.clear();
}

std::optional<PredecodedImage> ImagePredecodeCoordinator::findPredecodedImage(const QUrl &url) const
{
    return m_loadController.findPredecodedImage(url);
}
}
