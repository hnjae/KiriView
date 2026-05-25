// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodeeligibility.h"

#include "navigation/mediaformatregistry.h"

#include <algorithm>

namespace KiriView {
MediaPredecodeEligibilitySnapshot mediaPredecodeEligibilitySnapshot(
    const std::vector<MediaNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    MediaPredecodeEligibilitySnapshot snapshot {
        candidates.size(),
        mediaNavigationCandidateIndex(candidates, currentUrl),
        {},
    };
    snapshot.images.reserve(candidates.size());

    for (std::size_t index = 0; index < candidates.size(); ++index) {
        const MediaNavigationCandidate &candidate = candidates.at(index);
        if (isSupportedStillImageMediaCandidate(candidate)) {
            snapshot.images.push_back(MediaPredecodeEligibleImage { candidate.url, index });
        }
    }

    return snapshot;
}

std::vector<QUrl> mediaPredecodeEligibleUrlsForTargetIndices(
    const MediaPredecodeEligibilitySnapshot &snapshot, const std::vector<std::size_t> &indices)
{
    std::vector<QUrl> urls;
    urls.reserve(indices.size());

    for (std::size_t index : indices) {
        const auto eligible = std::find_if(snapshot.images.cbegin(), snapshot.images.cend(),
            [index](
                const MediaPredecodeEligibleImage &image) { return image.mediaIndex == index; });
        if (eligible != snapshot.images.cend()) {
            urls.push_back(eligible->url);
        }
    }

    return urls;
}
}
