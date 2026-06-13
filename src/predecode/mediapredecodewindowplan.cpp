// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodewindowplan.h"

namespace kiriview {
PredecodeWindowPlan mediaPredecodeWindowPlan(
    const MediaPredecodeEligibilitySnapshot &snapshot, PredecodePolicyInput policyInput)
{
    const PredecodeSchedulePlan schedule = predecodeSchedulePlan(
        snapshot.directMediaNavigationCandidateCount, snapshot.currentMediaIndex, policyInput);
    return PredecodeWindowPlan {
        OpenedCollectionScopeLocation {},
        mediaPredecodeEligibleUrlsForTargetIndices(snapshot, schedule.targetIndices),
        schedule.parallelLimit,
    };
}
}
