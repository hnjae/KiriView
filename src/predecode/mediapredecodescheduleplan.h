// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAPREDECODESCHEDULEPLAN_H
#define KIRIVIEW_MEDIAPREDECODESCHEDULEPLAN_H

#include "navigation/directmedianavigationmodel.h"
#include "predecode/mediapredecodeeligibility.h"
#include "predecodeschedulestate.h"

#include <QUrl>
#include <QtGlobal>
#include <vector>

namespace kiriview {
struct MediaPredecodeSchedulePayload final : PredecodeSchedulePayload
{
public:
    MediaPredecodeSchedulePayload() = default;
    std::vector<DirectMediaNavigationCandidate> directMediaNavigationCandidates;
    MediaPredecodeEligibilitySnapshot eligibleImages;
    Q_DISABLE_COPY(MediaPredecodeSchedulePayload)
};

struct MediaPredecodeScheduleRequest
{
    QUrl currentUrl;
    std::vector<DirectMediaNavigationCandidate> candidates;
    std::vector<DisplayedPredecodeImage> displayedImages;
    ImageFirstDisplayDecodeContext firstDisplayContext;
    bool immediate = false;
};

struct MediaPredecodeSchedulePlan
{
    PredecodeScheduleContext context;

    bool shouldSchedule() const;
};

MediaPredecodeSchedulePlan mediaPredecodeSchedulePlan(MediaPredecodeScheduleRequest request);
const std::vector<DirectMediaNavigationCandidate>* mediaPredecodeScheduleCandidates(
    const PredecodePendingSchedule& schedule);
const MediaPredecodeEligibilitySnapshot* mediaPredecodeScheduleEligibility(
    const PredecodePendingSchedule& schedule);
}

#endif
