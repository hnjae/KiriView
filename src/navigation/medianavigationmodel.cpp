// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "medianavigationmodel.h"

#include "location/imageurl.h"
#include "navigationcandidateordering.h"

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

std::optional<std::size_t> adjacentCandidateIndex(std::size_t candidateCount,
    std::optional<std::size_t> currentIndex, KiriView::NavigationDirection direction)
{
    if (!currentIndex.has_value() || *currentIndex >= candidateCount) {
        return std::nullopt;
    }

    switch (direction) {
    case KiriView::NavigationDirection::Previous:
        if (*currentIndex == 0) {
            return std::nullopt;
        }
        return *currentIndex - 1;
    case KiriView::NavigationDirection::Next: {
        const std::size_t nextIndex = *currentIndex + 1;
        if (nextIndex >= candidateCount) {
            return std::nullopt;
        }
        return nextIndex;
    }
    }

    return std::nullopt;
}

void appendRemovedCandidate(
    std::vector<KiriView::MediaNavigationCandidate> *candidates, const QUrl &currentUrl)
{
    candidates->push_back(KiriView::MediaNavigationCandidate { currentUrl, currentUrl.fileName() });
}

std::optional<std::size_t> clampedMediaNumberIndex(int mediaNumber, std::size_t candidateCount)
{
    if (candidateCount == 0) {
        return std::nullopt;
    }

    if (mediaNumber <= 1) {
        return 0;
    }

    return std::min(static_cast<std::size_t>(mediaNumber - 1), candidateCount - 1);
}

std::optional<QUrl> mediaNavigationTargetUrlForRequest(
    const std::vector<KiriView::MediaNavigationCandidate> &candidates, const QUrl &currentUrl,
    KiriView::MediaNavigationOpenRequest request)
{
    switch (request.kind) {
    case KiriView::MediaNavigationOpenKind::Previous:
        return KiriView::adjacentMediaNavigationUrl(
            candidates, currentUrl, KiriView::NavigationDirection::Previous);
    case KiriView::MediaNavigationOpenKind::Next:
        return KiriView::adjacentMediaNavigationUrl(
            candidates, currentUrl, KiriView::NavigationDirection::Next);
    case KiriView::MediaNavigationOpenKind::Number: {
        const std::optional<std::size_t> targetIndex
            = clampedMediaNumberIndex(request.mediaNumber, candidates.size());
        if (!targetIndex.has_value()) {
            return std::nullopt;
        }

        return candidates.at(*targetIndex).url;
    }
    }

    return std::nullopt;
}
}

namespace KiriView {
MediaNavigationOpenRequest previousMediaNavigationOpenRequest()
{
    return MediaNavigationOpenRequest { MediaNavigationOpenKind::Previous, 0 };
}

MediaNavigationOpenRequest nextMediaNavigationOpenRequest()
{
    return MediaNavigationOpenRequest { MediaNavigationOpenKind::Next, 0 };
}

MediaNavigationOpenRequest numberedMediaNavigationOpenRequest(int mediaNumber)
{
    return MediaNavigationOpenRequest { MediaNavigationOpenKind::Number, mediaNumber };
}

QUrl mediaNavigationSourceUrl(const QUrl &url)
{
    return directoryNavigationLocationForFileUrl(url).fileUrl;
}

QUrl mediaNavigationParentUrl(const QUrl &url)
{
    return directoryNavigationLocationForFileUrl(url).directoryUrl;
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
        = adjacentCandidateIndex(candidates.size(), currentIndex, direction);
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

MediaNavigationOpenPlan mediaNavigationOpenPlan(
    const std::vector<MediaNavigationCandidate> &candidates, const QUrl &currentUrl,
    MediaNavigationOpenRequest request)
{
    return MediaNavigationOpenPlan {
        mediaNavigationBoundaryState(candidates, currentUrl),
        mediaNavigationTargetUrlForRequest(candidates, currentUrl, request),
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
