// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "predecodewindowplan.h"

#include "location/imageurl.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <utility>

namespace {
std::optional<std::size_t> currentCandidateIndex(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    const auto currentCandidate = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentUrl](const KiriView::ImageNavigationCandidate &candidate) {
            return KiriView::sameNormalizedUrl(candidate.url, currentUrl);
        });
    if (currentCandidate == candidates.cend()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(candidates.cbegin(), currentCandidate));
}

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
        currentCandidateIndex(candidates, plan.candidateContext->currentUrl()), plan.policyInput);
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
