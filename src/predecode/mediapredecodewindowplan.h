// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAPREDECODEWINDOWPLAN_H
#define KIRIVIEW_MEDIAPREDECODEWINDOWPLAN_H

#include "predecode/mediapredecodeeligibility.h"
#include "predecode/predecodepolicy.h"
#include "predecode/predecodewindowplan.h"

namespace KiriView {
PredecodeWindowPlan mediaPredecodeWindowPlan(
    const MediaPredecodeEligibilitySnapshot &snapshot, PredecodePolicyInput policyInput);
}

#endif
