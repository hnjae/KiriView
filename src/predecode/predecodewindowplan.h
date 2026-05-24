// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODEWINDOWPLAN_H
#define KIRIVIEW_PREDECODEWINDOWPLAN_H

#include "location/imagelocation.h"
#include "navigation/imagecandidatelistsource.h"
#include "navigation/imagenavigationtypes.h"
#include "predecodepolicy.h"

#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
struct PredecodeWindowPlanRequest {
    DisplayedImageLocation displayedLocation;
    PredecodeMomentumMode momentumMode = PredecodeMomentumMode::Neutral;
    bool powerSaverEnabled = false;
    int idealThreadCount = 1;
};

struct PredecodeCandidateListLoadPlan {
    ImageCandidateListContext context;
    PredecodePolicyInput policyInput;
};

struct PredecodeWindowPlan {
    ArchiveDocumentLocation archiveDocument;
    std::vector<QUrl> urls;
    std::size_t parallelLimit = 0;
};

struct PredecodeWindowStartPlan {
    PredecodeWindowPlan fallbackWindow;
    std::optional<PredecodeCandidateListLoadPlan> candidateList;

    bool shouldLoadCandidates() const;
};

PredecodeWindowStartPlan predecodeWindowStartPlan(const PredecodeWindowPlanRequest &request);
PredecodeWindowPlan predecodeWindowPlanForCandidates(
    const PredecodeWindowStartPlan &plan, const std::vector<ImageNavigationCandidate> &candidates);
}

#endif
