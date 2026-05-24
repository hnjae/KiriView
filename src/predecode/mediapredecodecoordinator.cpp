// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodecoordinator.h"

#include "location/imageurl.h"
#include "predecodewindowplan.h"

#include <QThread>
#include <memory>
#include <optional>
#include <utility>

namespace {
class MediaPredecodeSchedulePayload final : public KiriView::PredecodeSchedulePayload
{
public:
    explicit MediaPredecodeSchedulePayload(
        std::vector<KiriView::MediaNavigationCandidate> candidates)
        : mediaCandidates(std::move(candidates))
    {
    }

    std::vector<KiriView::MediaNavigationCandidate> mediaCandidates;
};

bool validMediaPredecodeContext(const KiriView::MediaPredecodeCoordinator::Context &context)
{
    return KiriView::normalizedValidImageUrl(context.currentUrl).has_value();
}

const MediaPredecodeSchedulePayload *mediaPredecodeSchedulePayload(
    const KiriView::PredecodePendingSchedule &schedule)
{
    return dynamic_cast<const MediaPredecodeSchedulePayload *>(schedule.context.payload.get());
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
    , m_scheduleRuntime(
          this, m_loadController,
          [this](const PredecodePendingSchedule &schedule) { startPredecodeWindow(schedule); },
          std::move(powerSaverProvider))
{
}

void MediaPredecodeCoordinator::schedule(Context context)
{
    if (!validMediaPredecodeContext(context)) {
        m_scheduleRuntime.schedule(PredecodeScheduleContext {});
        return;
    }

    m_scheduleRuntime.schedule(scheduleContext(context));
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

    const MediaPredecodeSchedulePayload *payload = mediaPredecodeSchedulePayload(schedule);
    if (payload == nullptr) {
        return;
    }

    const PredecodeWindowPlan plan = predecodeWindowPlanForMediaCandidates(
        schedule.context.currentLocation.imageUrl(), payload->mediaCandidates, policyInput());
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

PredecodeScheduleContext MediaPredecodeCoordinator::scheduleContext(const Context &context) const
{
    const std::optional<std::size_t> currentIndex
        = mediaNavigationCandidateIndex(context.candidates, context.currentUrl);
    return PredecodeScheduleContext {
        DisplayedImageLocation::fromUrl(context.currentUrl),
        context.displayedImages,
        context.firstDisplayContext,
        currentIndex.has_value() ? static_cast<int>(*currentIndex) : -1,
        std::make_shared<MediaPredecodeSchedulePayload>(context.candidates),
    };
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
