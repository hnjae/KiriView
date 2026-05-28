// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodecoordinator.h"

#include "mediapredecodescheduleplan.h"
#include "mediapredecodewindowplan.h"
#include "predecodecachebudget.h"
#include "predecodelogging.h"

#include <QDebug>
#include <QThread>
#include <memory>
#include <optional>
#include <utility>

namespace KiriView {
MediaPredecodeCoordinator::MediaPredecodeCoordinator(QObject *parent)
    : MediaPredecodeCoordinator(parent, ImageDecodeDependencies {}, PowerSaverProvider {},
          defaultPredecodeCacheByteBudget())
{
}

MediaPredecodeCoordinator::MediaPredecodeCoordinator(QObject *parent,
    ImageDecodeDependencies decodeDependencies, PowerSaverProvider powerSaverProvider,
    qsizetype cacheByteBudget)
    : QObject(parent)
    , m_loadController(this, std::move(decodeDependencies), cacheByteBudget)
    , m_scheduleRuntime(
          this, m_loadController,
          [this](const PredecodePendingSchedule &schedule) { startPredecodeWindow(schedule); },
          std::move(powerSaverProvider))
{
}

void MediaPredecodeCoordinator::schedule(Context context)
{
    qCDebug(kiriviewPredecodeLog) << "media predecode schedule"
                                  << "url" << context.currentUrl << "candidates"
                                  << context.candidates.size() << "displayedImages"
                                  << context.displayedImages.size();
    MediaPredecodeSchedulePlan plan = mediaPredecodeSchedulePlan(MediaPredecodeScheduleRequest {
        context.currentUrl,
        std::move(context.candidates),
        std::move(context.displayedImages),
        context.firstDisplayContext,
    });
    if (!plan.shouldSchedule()) {
        qCDebug(kiriviewPredecodeLog) << "media predecode schedule ignored"
                                      << "reason"
                                      << "invalid-current-url";
    }
    m_scheduleRuntime.schedule(std::move(plan.context));
}

void MediaPredecodeCoordinator::setPowerSaverEnabled(bool enabled)
{
    m_scheduleRuntime.setPowerSaverEnabled(enabled);
}

bool MediaPredecodeCoordinator::powerSaverEnabled() const
{
    return m_scheduleRuntime.powerSaverEnabled();
}

void MediaPredecodeCoordinator::startPredecodeWindow(const PredecodePendingSchedule &schedule)
{
    if (!m_scheduleRuntime.accepts(schedule.generation)) {
        qCDebug(kiriviewPredecodeLog) << "media predecode window ignored"
                                      << "reason"
                                      << "stale-generation"
                                      << "generation" << schedule.generation;
        return;
    }

    const std::vector<MediaNavigationCandidate> *candidates
        = mediaPredecodeScheduleCandidates(schedule);
    const MediaPredecodeEligibilitySnapshot *eligibility
        = mediaPredecodeScheduleEligibility(schedule);
    if (candidates == nullptr || eligibility == nullptr) {
        qCDebug(kiriviewPredecodeLog) << "media predecode window ignored"
                                      << "reason"
                                      << "missing-payload"
                                      << "generation" << schedule.generation;
        return;
    }

    const PredecodeWindowPlan plan = mediaPredecodeWindowPlan(*eligibility, policyInput());
    qCDebug(kiriviewPredecodeLog) << "media predecode window start"
                                  << "generation" << schedule.generation << "primaryUrl"
                                  << schedule.context.currentLocation.imageUrl() << "candidates"
                                  << candidates->size() << "stillUrls" << plan.urls.size()
                                  << "parallelLimit" << plan.parallelLimit;
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

PredecodePolicyInput MediaPredecodeCoordinator::policyInput() const
{
    return PredecodePolicyInput {
        PredecodeScopeKind::DirectMedia,
        m_scheduleRuntime.momentumMode(),
        m_scheduleRuntime.powerSaverEnabled(),
        QThread::idealThreadCount(),
    };
}

void MediaPredecodeCoordinator::cancel() { m_scheduleRuntime.cancel(); }

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
