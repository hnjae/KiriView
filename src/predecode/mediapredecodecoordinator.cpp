// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodecoordinator.h"

#include "mediapredecodescheduleplan.h"
#include "mediapredecodewindowplan.h"

#include <QThread>
#include <memory>
#include <optional>
#include <utility>

namespace KiriView {
MediaPredecodeCoordinator::MediaPredecodeCoordinator(QObject *parent)
    : MediaPredecodeCoordinator(parent, ImageDecodeDependencies {})
{
}

MediaPredecodeCoordinator::MediaPredecodeCoordinator(QObject *parent,
    ImageDecodeDependencies decodeDependencies, PowerSaverProvider powerSaverProvider)
    : QObject(parent)
    , m_loadController(this, std::move(decodeDependencies))
    , m_scheduleRuntime(
          this, m_loadController,
          [this](const PredecodePendingSchedule &schedule) { startPredecodeWindow(schedule); },
          std::move(powerSaverProvider))
{
}

void MediaPredecodeCoordinator::schedule(Context context)
{
    MediaPredecodeSchedulePlan plan = mediaPredecodeSchedulePlan(MediaPredecodeScheduleRequest {
        context.currentUrl,
        std::move(context.candidates),
        std::move(context.displayedImages),
        context.firstDisplayContext,
    });
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
        return;
    }

    const std::vector<MediaNavigationCandidate> *candidates
        = mediaPredecodeScheduleCandidates(schedule);
    if (candidates == nullptr) {
        return;
    }

    const PredecodeWindowPlan plan = mediaPredecodeWindowPlan(
        schedule.context.currentLocation.imageUrl(), *candidates, policyInput());
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

PredecodePolicyInput MediaPredecodeCoordinator::policyInput() const
{
    return PredecodePolicyInput {
        PredecodeDocumentKind::Regular,
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
