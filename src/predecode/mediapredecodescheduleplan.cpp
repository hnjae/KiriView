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
    MediaPredecodeSchedulePayload(std::vector<KiriView::MediaNavigationCandidate> candidates,
        KiriView::MediaPredecodeEligibilitySnapshot eligibility)
        : mediaCandidates(std::move(candidates))
        , eligibleImages(std::move(eligibility))
    {
    }

    std::vector<KiriView::MediaNavigationCandidate> mediaCandidates;
    KiriView::MediaPredecodeEligibilitySnapshot eligibleImages;
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

    MediaPredecodeEligibilitySnapshot eligibility
        = mediaPredecodeEligibilitySnapshot(request.candidates, *cursorUrl);
    PredecodeScheduleContext context {
        DisplayedImageLocation::fromUrl(*cursorUrl),
        std::move(request.displayedImages),
        request.firstDisplayContext,
        eligibility.currentMediaIndex.has_value() ? static_cast<int>(*eligibility.currentMediaIndex)
                                                  : -1,
        std::make_shared<MediaPredecodeSchedulePayload>(
            std::move(request.candidates), std::move(eligibility)),
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

const MediaPredecodeEligibilitySnapshot *mediaPredecodeScheduleEligibility(
    const PredecodePendingSchedule &schedule)
{
    const auto *payload
        = dynamic_cast<const MediaPredecodeSchedulePayload *>(schedule.context.payload.get());
    return payload != nullptr ? &payload->eligibleImages : nullptr;
}
}
