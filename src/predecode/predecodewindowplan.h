// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_PREDECODEWINDOWPLAN_H
#define KIRIVIEW_PREDECODEWINDOWPLAN_H

#include "location/imagelocation.h"
#include "navigation/imagedocumentpagecandidatelistsource.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "predecodepolicy.h"

#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace kiriview {
struct PredecodeWindowPlanRequest {
    DisplayedImageLocation displayedLocation;
    PredecodePolicyInput policyInput;
};

struct PredecodeCandidateListLoadPlan {
    ImageDocumentPageCandidateListContext context;
    PredecodePolicyInput policyInput;
};

struct PredecodeWindowPlan {
    OpenedCollectionScopeLocation openedCollectionScope;
    std::vector<QUrl> urls;
    std::size_t parallelLimit = 0;
};

struct PredecodeWindowStartPlan {
    PredecodeWindowPlan fallbackWindow;
    std::optional<PredecodeCandidateListLoadPlan> candidateList;

    bool shouldLoadCandidates() const;
};

PredecodeWindowStartPlan predecodeWindowStartPlan(const PredecodeWindowPlanRequest &request);
PredecodeWindowPlan predecodeWindowPlanForCandidates(const PredecodeWindowStartPlan &plan,
    const std::vector<ImageDocumentPageCandidate> &candidates);
}

#endif
