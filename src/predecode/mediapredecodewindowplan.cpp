// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodewindowplan.h"

#include "decoding/imageformatregistry.h"

#include <cstddef>
#include <vector>

namespace {
std::vector<QUrl> mediaPredecodeWindowImageUrls(
    const std::vector<KiriView::MediaNavigationCandidate> &candidates,
    const std::vector<std::size_t> &indices)
{
    std::vector<QUrl> urls;
    urls.reserve(indices.size());
    for (std::size_t index : indices) {
        if (index < candidates.size()
            && KiriView::mediaPredecodeStillImageEligible(candidates.at(index))) {
            urls.push_back(candidates.at(index).url);
        }
    }
    return urls;
}
}

namespace KiriView {
bool mediaPredecodeStillImageEligible(const MediaNavigationCandidate &candidate)
{
    return isSupportedImageFileName(candidate.name)
        || isSupportedImageFileName(candidate.url.fileName(QUrl::PrettyDecoded))
        || isSupportedImageFileName(candidate.url.toString(QUrl::PrettyDecoded));
}

PredecodeWindowPlan mediaPredecodeWindowPlan(const QUrl &currentUrl,
    const std::vector<MediaNavigationCandidate> &candidates, PredecodePolicyInput policyInput)
{
    const PredecodeSchedulePlan schedule = predecodeSchedulePlan(
        candidates.size(), mediaNavigationCandidateIndex(candidates, currentUrl), policyInput);
    return PredecodeWindowPlan {
        ArchiveDocumentLocation {},
        mediaPredecodeWindowImageUrls(candidates, schedule.targetIndices),
        schedule.parallelLimit,
    };
}
}
