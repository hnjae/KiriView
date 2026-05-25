// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "predecodecache.h"
#include "predecodewindowplan.h"
#include "system/systemmemory.h"

#include <QThread>
#include <cstddef>
#include <utility>
#include <vector>

namespace KiriView {
namespace {
    qsizetype defaultPredecodeCacheByteBudget()
    {
        return PredecodeCache::byteBudgetForSystemMemory(systemMemorySnapshot().physicalByteSize);
    }
}

ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent)
    : ImagePredecodeCoordinator(parent, ImageNavigationCandidateProvider {},
          ImageDecodeDependencies {}, PowerSaverProvider {}, defaultPredecodeCacheByteBudget())
{
}

ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent,
    ImageNavigationCandidateProvider candidateProvider, ImageDecodeDependencies decodeDependencies,
    PowerSaverProvider powerSaverProvider, qsizetype cacheByteBudget)
    : QObject(parent)
    , m_candidateRepository(std::move(candidateProvider))
    , m_loadController(this, std::move(decodeDependencies), cacheByteBudget)
    , m_scheduleRuntime(
          this, m_loadController,
          [this](const PredecodePendingSchedule &schedule) {
              scheduleAdjacentImagePredecode(schedule);
          },
          [this]() { m_listerJob.cancel(); }, std::move(powerSaverProvider))
{
}

void ImagePredecodeCoordinator::schedule(Context context)
{
    m_scheduleRuntime.schedule(std::move(context));
}

void ImagePredecodeCoordinator::setPowerSaverEnabled(bool enabled)
{
    m_scheduleRuntime.setPowerSaverEnabled(enabled);
}

bool ImagePredecodeCoordinator::powerSaverEnabled() const
{
    return m_scheduleRuntime.powerSaverEnabled();
}

void ImagePredecodeCoordinator::scheduleAdjacentImagePredecode(
    const PredecodePendingSchedule &schedule)
{
    const PredecodeWindowStartPlan plan = predecodeWindowStartPlan(PredecodeWindowPlanRequest {
        schedule.context.currentLocation,
        m_scheduleRuntime.momentumMode(),
        m_scheduleRuntime.powerSaverEnabled(),
        QThread::idealThreadCount(),
    });
    if (!plan.shouldLoadCandidates()) {
        startPredecodeImageLoads(plan.fallbackWindow, schedule);
        return;
    }

    m_listerJob = m_candidateRepository.loadImages(
        this, plan.candidateList->context,
        [this, schedule, plan](const std::vector<ImageNavigationCandidate> &candidates) {
            startPredecodeImageLoads(predecodeWindowPlanForCandidates(plan, candidates), schedule);
        },
        [this, schedule, plan](
            const QString &) { startPredecodeImageLoads(plan.fallbackWindow, schedule); });
}

void ImagePredecodeCoordinator::startPredecodeImageLoads(
    const PredecodeWindowPlan &plan, const PredecodePendingSchedule &schedule)
{
    if (!m_scheduleRuntime.accepts(schedule.generation)) {
        return;
    }

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

void ImagePredecodeCoordinator::cancel() { m_scheduleRuntime.cancel(); }

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
