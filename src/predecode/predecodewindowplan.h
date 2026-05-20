// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODEWINDOWPLAN_H
#define KIRIVIEW_PREDECODEWINDOWPLAN_H

#include "imagelocation.h"
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

struct PredecodeCandidateListPlan {
    ArchiveDocumentLocation archiveDocument;
    std::optional<ImageCandidateListContext> candidateContext;
    PredecodePolicyInput policyInput;
    std::size_t parallelLimit = 0;

    bool shouldLoadCandidates() const;
};

struct PredecodeWindowPlan {
    ArchiveDocumentLocation archiveDocument;
    std::vector<QUrl> urls;
    std::size_t parallelLimit = 0;
};

PredecodeCandidateListPlan predecodeCandidateListPlan(const PredecodeWindowPlanRequest &request);
PredecodeWindowPlan predecodeWindowPlanForCandidates(const PredecodeCandidateListPlan &plan,
    const std::vector<ImageNavigationCandidate> &candidates);
PredecodeWindowPlan predecodeWindowPlanWithoutCandidates(const PredecodeCandidateListPlan &plan);
}

#endif
