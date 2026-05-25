// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAPREDECODEWINDOWPLAN_H
#define KIRIVIEW_MEDIAPREDECODEWINDOWPLAN_H

#include "navigation/medianavigationmodel.h"
#include "predecode/predecodepolicy.h"
#include "predecode/predecodewindowplan.h"

#include <QUrl>
#include <vector>

namespace KiriView {
PredecodeWindowPlan mediaPredecodeWindowPlan(const QUrl &currentUrl,
    const std::vector<MediaNavigationCandidate> &candidates, PredecodePolicyInput policyInput);
}

#endif
