// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_NAVIGATIONCANDIDATEORDERING_H
#define KIRIVIEW_NAVIGATIONCANDIDATEORDERING_H

#include "location/imageurl.h"
#include "navigation/imagenavigationtypes.h"

#include <QCollator>
#include <QLocale>
#include <QUrl>
#include <Qt>
#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
inline std::optional<std::size_t> adjacentNavigationIndex(std::size_t candidateCount,
    std::optional<std::size_t> currentIndex, NavigationDirection direction)
{
    if (!currentIndex.has_value() || *currentIndex >= candidateCount) {
        return std::nullopt;
    }

    switch (direction) {
    case NavigationDirection::Previous:
        if (*currentIndex == 0) {
            return std::nullopt;
        }
        return *currentIndex - 1;
    case NavigationDirection::Next: {
        const std::size_t nextIndex = *currentIndex + 1;
        if (nextIndex >= candidateCount) {
            return std::nullopt;
        }
        return nextIndex;
    }
    }

    return std::nullopt;
}

template <typename Candidate>
std::optional<std::size_t> navigationCandidateIndex(
    const std::vector<Candidate> &candidates, const QUrl &currentUrl)
{
    const auto currentCandidate = std::find_if(
        candidates.cbegin(), candidates.cend(), [&currentUrl](const Candidate &candidate) {
            return sameNormalizedUrl(candidate.url, currentUrl);
        });
    if (currentCandidate == candidates.cend()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(candidates.cbegin(), currentCandidate));
}

template <typename Candidate>
std::optional<std::size_t> adjacentNavigationCandidateIndex(
    const std::vector<Candidate> &candidates, const QUrl &currentUrl, NavigationDirection direction)
{
    return adjacentNavigationIndex(
        candidates.size(), navigationCandidateIndex(candidates, currentUrl), direction);
}

template <typename Candidate>
std::optional<Candidate> adjacentNavigationCandidate(
    const std::vector<Candidate> &candidates, const QUrl &currentUrl, NavigationDirection direction)
{
    const std::optional<std::size_t> targetIndex
        = adjacentNavigationCandidateIndex(candidates, currentUrl, direction);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    return candidates.at(*targetIndex);
}

template <typename Candidate>
std::optional<QUrl> adjacentNavigationCandidateUrl(
    const std::vector<Candidate> &candidates, const QUrl &currentUrl, NavigationDirection direction)
{
    const std::optional<std::size_t> targetIndex
        = adjacentNavigationCandidateIndex(candidates, currentUrl, direction);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    return candidates.at(*targetIndex).url;
}

template <typename Candidate>
void sortNavigationCandidatesByNameAndUrl(std::vector<Candidate> *candidates)
{
    QCollator collator(QLocale::system());
    collator.setCaseSensitivity(Qt::CaseSensitive);
    collator.setNumericMode(false);
    collator.setIgnorePunctuation(false);

    std::stable_sort(candidates->begin(), candidates->end(),
        [&collator](const Candidate &left, const Candidate &right) {
            const int nameComparison = collator.compare(left.name, right.name);
            if (nameComparison != 0) {
                return nameComparison < 0;
            }

            return left.url.toEncoded() < right.url.toEncoded();
        });

    const auto duplicateStart = std::unique(
        candidates->begin(), candidates->end(), [](const Candidate &left, const Candidate &right) {
            return sameNormalizedUrl(left.url, right.url);
        });
    candidates->erase(duplicateStart, candidates->end());
}
}

#endif
