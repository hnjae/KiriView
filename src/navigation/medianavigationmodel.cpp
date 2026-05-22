// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "medianavigationmodel.h"

#include "location/imageurl.h"
#include "navigationcandidateordering.h"
#include "navigationindexpolicy.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace {
std::optional<std::size_t> candidateIndex(
    const std::vector<KiriView::MediaNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    const auto current = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentUrl](const KiriView::MediaNavigationCandidate &candidate) {
            return KiriView::sameNormalizedUrl(candidate.url, currentUrl);
        });
    if (current == candidates.cend()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(candidates.cbegin(), current));
}

void appendRemovedCandidate(
    std::vector<KiriView::MediaNavigationCandidate> *candidates, const QUrl &currentUrl)
{
    candidates->push_back(KiriView::MediaNavigationCandidate { currentUrl, currentUrl.fileName() });
}
}

namespace KiriView {
QUrl mediaNavigationSourceUrl(const QUrl &url) { return navigationSourceUrl(url); }

QUrl mediaNavigationParentUrl(const QUrl &url)
{
    const QUrl currentUrl = mediaNavigationSourceUrl(url);
    return currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
}

std::optional<std::size_t> mediaNavigationCandidateIndex(
    const std::vector<MediaNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    return candidateIndex(candidates, mediaNavigationSourceUrl(currentUrl));
}

std::optional<QUrl> adjacentMediaNavigationUrl(
    const std::vector<MediaNavigationCandidate> &candidates, const QUrl &currentUrl,
    NavigationDirection direction)
{
    const std::optional<std::size_t> currentIndex
        = mediaNavigationCandidateIndex(candidates, currentUrl);
    const std::optional<std::size_t> targetIndex
        = adjacentNavigationCandidateIndex(candidates.size(), currentIndex, direction);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    return candidates.at(*targetIndex).url;
}

MediaNavigationBoundaryState mediaNavigationBoundaryState(
    const std::vector<MediaNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    const std::optional<std::size_t> currentIndex
        = mediaNavigationCandidateIndex(candidates, currentUrl);
    if (!currentIndex.has_value() || candidates.empty()) {
        return {};
    }

    return MediaNavigationBoundaryState {
        *currentIndex > 0,
        *currentIndex + 1 < candidates.size(),
        *currentIndex == 0,
        *currentIndex + 1 == candidates.size(),
        static_cast<int>(*currentIndex + 1),
        static_cast<int>(candidates.size()),
    };
}

MediaDeletionFallbackPlan mediaDeletionFallbackPlan(
    std::vector<MediaNavigationCandidate> candidates, const QUrl &currentUrl)
{
    const QUrl identityUrl = mediaNavigationSourceUrl(currentUrl);
    if (identityUrl.isEmpty()) {
        return {};
    }

    if (!mediaNavigationCandidateIndex(candidates, identityUrl).has_value()) {
        appendRemovedCandidate(&candidates, identityUrl);
        sortMediaNavigationCandidates(&candidates);
    }

    return MediaDeletionFallbackPlan {
        identityUrl,
        adjacentMediaNavigationUrl(candidates, identityUrl, NavigationDirection::Next),
        adjacentMediaNavigationUrl(candidates, identityUrl, NavigationDirection::Previous),
    };
}

void sortMediaNavigationCandidates(std::vector<MediaNavigationCandidate> *candidates)
{
    sortNavigationCandidatesByNameAndUrl(candidates);
}
}
