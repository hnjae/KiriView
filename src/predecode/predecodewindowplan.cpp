// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodewindowplan.h"

#include "navigation/imagenavigationmodel.h"

#include <optional>
#include <utility>

namespace {
std::vector<QUrl> predecodeWindowImageUrls(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates,
    const std::vector<std::size_t> &indices)
{
    std::vector<QUrl> urls;
    urls.reserve(indices.size());
    for (std::size_t index : indices) {
        if (index < candidates.size()) {
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
    const ArchiveDocumentLocation archiveDocument = request.displayedLocation.archiveDocument();
    const PredecodePolicyInput policyInput = predecodePolicyInputForArchiveDocument(
        archiveDocument, request.momentumMode, request.powerSaverEnabled, request.idealThreadCount);
    const PredecodeSchedulePlan initialSchedule
        = predecodeSchedulePlan(0, std::nullopt, policyInput);
    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateListContextForDisplayedImage(request.displayedLocation);

    PredecodeWindowStartPlan plan {
        PredecodeWindowPlan {
            archiveDocument,
            {},
            initialSchedule.parallelLimit,
        },
        std::nullopt,
    };
    if (initialSchedule.parallelLimit > 0 && candidateContext.has_value()) {
        plan.candidateList = PredecodeCandidateListLoadPlan { *candidateContext, policyInput };
    }

    return plan;
}

PredecodeWindowPlan predecodeWindowPlanForCandidates(
    const PredecodeWindowStartPlan &plan, const std::vector<ImageNavigationCandidate> &candidates)
{
    if (!plan.candidateList.has_value()) {
        return plan.fallbackWindow;
    }

    const PredecodeSchedulePlan schedule = predecodeSchedulePlan(candidates.size(),
        imageNavigationCandidateIndex(candidates, plan.candidateList->context.currentUrl()),
        plan.candidateList->policyInput);
    return PredecodeWindowPlan {
        plan.fallbackWindow.archiveDocument,
        predecodeWindowImageUrls(candidates, schedule.targetIndices),
        schedule.parallelLimit,
    };
}

}
