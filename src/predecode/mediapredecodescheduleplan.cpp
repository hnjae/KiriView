// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodescheduleplan.h"

#include "location/imageurl.h"

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

}

namespace KiriView {
bool MediaPredecodeSchedulePlan::shouldSchedule() const
{
    return !context.currentLocation.isEmpty();
}

MediaPredecodeSchedulePlan mediaPredecodeSchedulePlan(MediaPredecodeScheduleRequest request)
{
    const std::optional<QUrl> cursorUrl = normalizedValidUrlForIdentity(request.currentUrl);
    if (!cursorUrl.has_value()) {
        return {};
    }

    const std::optional<std::size_t> currentIndex
        = mediaNavigationCandidateIndex(request.candidates, *cursorUrl);
    PredecodeScheduleContext context {
        DisplayedImageLocation::fromUrl(*cursorUrl),
        std::move(request.displayedImages),
        request.firstDisplayContext,
        currentIndex.has_value() ? static_cast<int>(*currentIndex) : -1,
        std::make_shared<MediaPredecodeSchedulePayload>(std::move(request.candidates)),
    };
    return MediaPredecodeSchedulePlan { std::move(context) };
}

const std::vector<MediaNavigationCandidate> *mediaPredecodeScheduleCandidates(
    const PredecodePendingSchedule &schedule)
{
    const auto *payload
        = dynamic_cast<const MediaPredecodeSchedulePayload *>(schedule.context.payload.get());
    return payload != nullptr ? &payload->mediaCandidates : nullptr;
}
}
