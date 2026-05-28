// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "predecodecachebudget.h"
#include "predecodelogging.h"
#include "predecodewindowplan.h"

#include <QDebug>
#include <QThread>
#include <cstddef>
#include <utility>
#include <vector>

namespace KiriView {
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
    qCDebug(kiriviewPredecodeLog) << "image predecode schedule"
                                  << "url" << context.currentLocation.imageUrl()
                                  << "displayedImages" << context.displayedImages.size();
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
        PredecodePolicyInput {
            predecodeSourceProfileForOpenedCollectionScope(
                schedule.context.currentLocation.openedCollectionScope(),
                QThread::idealThreadCount()),
            m_scheduleRuntime.momentumMode(),
            m_scheduleRuntime.powerSaverEnabled(),
        },
    });
    qCDebug(kiriviewPredecodeLog) << "image predecode start plan"
                                  << "generation" << schedule.generation << "url"
                                  << schedule.context.currentLocation.imageUrl() << "loadCandidates"
                                  << plan.shouldLoadCandidates() << "fallbackUrls"
                                  << plan.fallbackWindow.urls.size() << "parallelLimit"
                                  << plan.fallbackWindow.parallelLimit;
    if (!plan.shouldLoadCandidates()) {
        startPredecodeImageLoads(plan.fallbackWindow, schedule);
        return;
    }

    m_listerJob = m_candidateRepository.loadImages(
        this, plan.candidateList->context,
        [this, schedule, plan](const std::vector<ImageNavigationCandidate> &candidates) {
            qCDebug(kiriviewPredecodeLog)
                << "image predecode candidates loaded"
                << "generation" << schedule.generation << "count" << candidates.size();
            startPredecodeImageLoads(predecodeWindowPlanForCandidates(plan, candidates), schedule);
        },
        [this, schedule, plan](const QString &errorString) {
            qCDebug(kiriviewPredecodeLog)
                << "image predecode candidates failed"
                << "generation" << schedule.generation << "error" << errorString;
            startPredecodeImageLoads(plan.fallbackWindow, schedule);
        });
}

void ImagePredecodeCoordinator::startPredecodeImageLoads(
    const PredecodeWindowPlan &plan, const PredecodePendingSchedule &schedule)
{
    if (!m_scheduleRuntime.accepts(schedule.generation)) {
        qCDebug(kiriviewPredecodeLog) << "image predecode window ignored"
                                      << "reason"
                                      << "stale-generation"
                                      << "generation" << schedule.generation;
        return;
    }

    qCDebug(kiriviewPredecodeLog) << "image predecode window start"
                                  << "generation" << schedule.generation << "primaryUrl"
                                  << schedule.context.currentLocation.imageUrl() << "urls"
                                  << plan.urls.size() << "parallelLimit" << plan.parallelLimit;
    m_loadController.startWindowLoads(PredecodeLoadWindow {
        schedule.context.currentLocation.imageUrl(),
        plan.openedCollectionScope,
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
