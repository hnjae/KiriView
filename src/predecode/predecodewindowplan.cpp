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
bool PredecodeCandidateListPlan::shouldLoadCandidates() const
{
    return parallelLimit > 0 && candidateContext.has_value();
}

PredecodeCandidateListPlan predecodeCandidateListPlan(const PredecodeWindowPlanRequest &request)
{
    const ArchiveDocumentLocation archiveDocument = request.displayedLocation.archiveDocument();
    const PredecodePolicyInput policyInput = predecodePolicyInputForArchiveDocument(
        archiveDocument, request.momentumMode, request.powerSaverEnabled, request.idealThreadCount);
    const PredecodeSchedulePlan initialSchedule
        = predecodeSchedulePlan(0, std::nullopt, policyInput);
    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateListContextForDisplayedImage(request.displayedLocation);

    return PredecodeCandidateListPlan {
        archiveDocument,
        candidateContext,
        policyInput,
        initialSchedule.parallelLimit,
    };
}

PredecodeWindowPlan predecodeWindowPlanForCandidates(
    const PredecodeCandidateListPlan &plan, const std::vector<ImageNavigationCandidate> &candidates)
{
    if (!plan.candidateContext.has_value()) {
        return predecodeWindowPlanWithoutCandidates(plan);
    }

    const PredecodeSchedulePlan schedule = predecodeSchedulePlan(candidates.size(),
        imageNavigationCandidateIndex(candidates, plan.candidateContext->currentUrl()),
        plan.policyInput);
    return PredecodeWindowPlan {
        plan.archiveDocument,
        predecodeWindowImageUrls(candidates, schedule.targetIndices),
        schedule.parallelLimit,
    };
}

PredecodeWindowPlan predecodeWindowPlanWithoutCandidates(const PredecodeCandidateListPlan &plan)
{
    return PredecodeWindowPlan {
        plan.archiveDocument,
        {},
        plan.parallelLimit,
    };
}
}
