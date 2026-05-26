// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodescheduleplan.h"

#include "location/imageurl.h"

#include <memory>
#include <optional>
#include <utility>
#include <variant>

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
        std::make_shared<PredecodeSchedulePayload>(MediaPredecodeSchedulePayload {
            std::move(request.candidates), std::move(eligibility) }),
    };
    return MediaPredecodeSchedulePlan { std::move(context) };
}

const std::vector<MediaNavigationCandidate> *mediaPredecodeScheduleCandidates(
    const PredecodePendingSchedule &schedule)
{
    const auto *payload = schedule.context.payload == nullptr
        ? nullptr
        : std::get_if<MediaPredecodeSchedulePayload>(schedule.context.payload.get());
    return payload != nullptr ? &payload->mediaCandidates : nullptr;
}

const MediaPredecodeEligibilitySnapshot *mediaPredecodeScheduleEligibility(
    const PredecodePendingSchedule &schedule)
{
    const auto *payload = schedule.context.payload == nullptr
        ? nullptr
        : std::get_if<MediaPredecodeSchedulePayload>(schedule.context.payload.get());
    return payload != nullptr ? &payload->eligibleImages : nullptr;
}
}
