// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAPREDECODESCHEDULEPLAN_H
#define KIRIVIEW_MEDIAPREDECODESCHEDULEPLAN_H

#include "navigation/medianavigationmodel.h"
#include "predecode/mediapredecodeeligibility.h"
#include "predecodeschedulestate.h"

#include <QUrl>
#include <vector>

namespace KiriView {
struct MediaPredecodeScheduleRequest {
    QUrl currentUrl;
    std::vector<MediaNavigationCandidate> candidates;
    std::vector<DisplayedPredecodeImage> displayedImages;
    ImageFirstDisplayDecodeContext firstDisplayContext;
};

struct MediaPredecodeSchedulePlan {
    PredecodeScheduleContext context;

    bool shouldSchedule() const;
};

MediaPredecodeSchedulePlan mediaPredecodeSchedulePlan(MediaPredecodeScheduleRequest request);
const std::vector<MediaNavigationCandidate> *mediaPredecodeScheduleCandidates(
    const PredecodePendingSchedule &schedule);
const MediaPredecodeEligibilitySnapshot *mediaPredecodeScheduleEligibility(
    const PredecodePendingSchedule &schedule);
}

#endif
