// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "predecodewindowplan.h"

#include <QThread>
#include <cstddef>
#include <utility>
#include <vector>

namespace KiriView {
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
    const PredecodeScheduleEffectPlan plan
        = m_scheduleState.schedule(std::move(context), currentMonotonicMsec());
    executeScheduleEffects(plan);
}

void ImagePredecodeCoordinator::setPowerSaverEnabled(bool enabled)
{
    const PredecodeScheduleEffectPlan plan
        = m_scheduleState.setPowerSaverEnabled(enabled, currentMonotonicMsec());
    executeScheduleEffects(plan);
}

bool ImagePredecodeCoordinator::powerSaverEnabled() const
{
    return m_scheduleState.powerSaverEnabled();
}

void ImagePredecodeCoordinator::executeScheduleEffects(const PredecodeScheduleEffectPlan &plan)
{
    if (plan.cancelBackgroundEffects) {
        cancelBackgroundEffects();
    }
    if (plan.cacheDisplayedContext.has_value()) {
        cacheDisplayedImages(*plan.cacheDisplayedContext);
    }
    if (plan.clearWindowUrls) {
        m_loadController.clearWindowUrls();
    }
    if (plan.startDebounceTimers) {
        m_debounceTimer.start();
        m_neutralTimer.start();
    }
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
    const PredecodeScheduleEffectPlan plan = m_scheduleState.settlePendingScheduleToNeutral();
    executeScheduleEffects(plan);
    if (!plan.pendingSchedule.has_value()) {
        return;
    }

    scheduleAdjacentImagePredecode(plan.pendingSchedule->context, plan.pendingSchedule->generation);
}

void ImagePredecodeCoordinator::scheduleAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const PredecodeCandidateListPlan candidateListPlan
        = predecodeCandidateListPlan(PredecodeWindowPlanRequest {
            context.primaryImage.location,
            m_scheduleState.momentumMode(),
            m_scheduleState.powerSaverEnabled(),
            QThread::idealThreadCount(),
        });
    if (!candidateListPlan.shouldLoadCandidates()) {
        const PredecodeWindowPlan windowPlan
            = predecodeWindowPlanWithoutCandidates(candidateListPlan);
        startPredecodeImageLoads(windowPlan.urls, windowPlan.archiveDocument, context, generation,
            windowPlan.parallelLimit);
        return;
    }

    m_listerJob = m_candidateRepository.loadImages(
        this, *candidateListPlan.candidateContext,
        [this, context, generation, candidateListPlan](
            const std::vector<ImageNavigationCandidate> &candidates) {
            const PredecodeWindowPlan windowPlan
                = predecodeWindowPlanForCandidates(candidateListPlan, candidates);
            startPredecodeImageLoads(windowPlan.urls, windowPlan.archiveDocument, context,
                generation, windowPlan.parallelLimit);
        },
        [this, context, generation, candidateListPlan](const QString &) {
            const PredecodeWindowPlan windowPlan
                = predecodeWindowPlanWithoutCandidates(candidateListPlan);
            startPredecodeImageLoads(windowPlan.urls, windowPlan.archiveDocument, context,
                generation, windowPlan.parallelLimit);
        });
}

void ImagePredecodeCoordinator::startPredecodeImageLoads(const std::vector<QUrl> &urls,
    const ArchiveDocumentLocation &archiveDocument, const Context &context, quint64 generation,
    std::size_t parallelLimit)
{
    if (!m_scheduleState.accepts(generation)) {
        return;
    }

    m_loadController.startWindowLoads(PredecodeLoadWindow {
        context.primaryImage.location.imageUrl(),
        archiveDocument,
        urls,
        displayedImages(context),
        context.firstDisplayContext,
        generation,
        parallelLimit,
    });
}

void ImagePredecodeCoordinator::cancelBackgroundWork()
{
    m_scheduleState.cancelBackgroundWork();
    cancelBackgroundEffects();
}

void ImagePredecodeCoordinator::cancelBackgroundEffects()
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
    cancelBackgroundEffects();
    m_scheduleState.cancel();
}

void ImagePredecodeCoordinator::clear()
{
    cancel();
    m_loadController.clear();
}

std::optional<PredecodedImage> ImagePredecodeCoordinator::tryTake(const QUrl &url) const
{
    return m_loadController.tryTake(url);
}
}
