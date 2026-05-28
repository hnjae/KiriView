// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodewindowplan.h"

namespace KiriView {
PredecodeWindowPlan mediaPredecodeWindowPlan(
    const MediaPredecodeEligibilitySnapshot &snapshot, PredecodePolicyInput policyInput)
{
    const PredecodeSchedulePlan schedule = predecodeSchedulePlan(
        snapshot.mediaCandidateCount, snapshot.currentMediaIndex, policyInput);
    return PredecodeWindowPlan {
        OpenedCollectionScopeLocation {},
        mediaPredecodeEligibleUrlsForTargetIndices(snapshot, schedule.targetIndices),
        schedule.parallelLimit,
    };
}
}
