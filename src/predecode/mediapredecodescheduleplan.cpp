// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodescheduleplan.h"

#include "location/imageurl.h"

#include <memory>
#include <optional>
#include <utility>

namespace kiriview {
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
    auto payload = std::make_shared<MediaPredecodeSchedulePayload>();
    payload->directMediaNavigationCandidates = std::move(request.candidates);
    payload->eligibleImages = std::move(eligibility);
    PredecodeScheduleContext context {
        DisplayedImageLocation::fromUrl(*cursorUrl),
        std::move(request.displayedImages),
        request.firstDisplayContext,
        payload->eligibleImages.currentMediaIndex.has_value()
            ? static_cast<int>(*payload->eligibleImages.currentMediaIndex)
            : -1,
        std::move(payload),
    };
    return MediaPredecodeSchedulePlan { std::move(context) };
}

const std::vector<DirectMediaNavigationCandidate> *mediaPredecodeScheduleCandidates(
    const PredecodePendingSchedule &schedule)
{
    const auto *payload = predecodeSchedulePayload<MediaPredecodeSchedulePayload>(schedule);
    return payload != nullptr ? &payload->directMediaNavigationCandidates : nullptr;
}

const MediaPredecodeEligibilitySnapshot *mediaPredecodeScheduleEligibility(
    const PredecodePendingSchedule &schedule)
{
    const auto *payload = predecodeSchedulePayload<MediaPredecodeSchedulePayload>(schedule);
    return payload != nullptr ? &payload->eligibleImages : nullptr;
}
}
