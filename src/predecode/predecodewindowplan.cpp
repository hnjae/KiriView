// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodewindowplan.h"

#include "navigation/imagedocumentpagenavigationpolicy.h"

#include <optional>
#include <utility>

namespace {
std::vector<QUrl> predecodeWindowImageUrls(
    const std::vector<KiriView::ImageDocumentPageCandidate> &candidates,
    const std::vector<std::size_t> &indices)
{
    std::vector<QUrl> urls;
    urls.reserve(indices.size());
    for (std::size_t index : indices) {
        if (index < candidates.size() && imageDocumentPageCandidateIsImage(candidates.at(index))) {
            urls.push_back(candidates.at(index).url);
        }
    }
    return urls;
}

}

namespace KiriView {
bool PredecodeWindowStartPlan::shouldLoadCandidates() const { return candidateList.has_value(); }

PredecodeWindowStartPlan predecodeWindowStartPlan(const PredecodeWindowPlanRequest &request)
{
    const OpenedCollectionScopeLocation openedCollectionScope
        = request.displayedLocation.openedCollectionScope();
    const PredecodeSchedulePlan initialSchedule
        = predecodeSchedulePlan(0, std::nullopt, request.policyInput);
    const std::optional<ImageDocumentPageCandidateListContext> candidateContext
        = imageDocumentPageCandidateListContextForDisplayedImage(request.displayedLocation);

    PredecodeWindowStartPlan plan {
        PredecodeWindowPlan {
            openedCollectionScope,
            {},
            initialSchedule.parallelLimit,
        },
        std::nullopt,
    };
    if (initialSchedule.parallelLimit > 0 && candidateContext.has_value()) {
        plan.candidateList
            = PredecodeCandidateListLoadPlan { *candidateContext, request.policyInput };
    }

    return plan;
}

PredecodeWindowPlan predecodeWindowPlanForCandidates(
    const PredecodeWindowStartPlan &plan, const std::vector<ImageDocumentPageCandidate> &candidates)
{
    if (!plan.candidateList.has_value()) {
        return plan.fallbackWindow;
    }

    const PredecodeSchedulePlan schedule = predecodeSchedulePlan(candidates.size(),
        imageDocumentPageCandidateIndex(candidates, plan.candidateList->context.currentUrl()),
        plan.candidateList->policyInput);
    return PredecodeWindowPlan {
        plan.fallbackWindow.openedCollectionScope,
        predecodeWindowImageUrls(candidates, schedule.targetIndices),
        schedule.parallelLimit,
    };
}

}
