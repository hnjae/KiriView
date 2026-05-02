// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationmodel.h"

#include "imageformatregistry.h"
#include "imageurl.h"

#include <QCollator>
#include <QLocale>
#include <Qt>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>

namespace {
constexpr std::ptrdiff_t predecodePreviousImageCount = 2;
constexpr std::ptrdiff_t predecodeNextImageCount = 4;

template <typename Candidate>
std::optional<std::size_t> adjacentCandidateIndex(const std::vector<Candidate> &candidates,
    const QUrl &currentUrl, KiriView::NavigationDirection direction)
{
    const auto current = std::find_if(
        candidates.cbegin(), candidates.cend(), [&currentUrl](const Candidate &candidate) {
            return KiriView::sameNormalizedUrl(candidate.url, currentUrl);
        });
    if (current == candidates.cend()) {
        return std::nullopt;
    }

    const auto currentIndex = std::distance(candidates.cbegin(), current);
    if (direction == KiriView::NavigationDirection::Previous) {
        if (currentIndex == 0) {
            return std::nullopt;
        }
        return static_cast<std::size_t>(currentIndex - 1);
    }

    const auto nextIndex = currentIndex + 1;
    if (nextIndex == static_cast<std::ptrdiff_t>(candidates.size())) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(nextIndex);
}

template <typename Candidate> void sortNavigationCandidates(std::vector<Candidate> *candidates)
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
            return KiriView::sameNormalizedUrl(left.url, right.url);
        });
    candidates->erase(duplicateStart, candidates->end());
}
}

namespace KiriView {
std::vector<QUrl> predecodeWindowImageUrls(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    std::vector<QUrl> urls;
    const auto current = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentUrl](const ImageNavigationCandidate &candidate) {
            return sameNormalizedUrl(candidate.url, currentUrl);
        });
    if (current == candidates.cend()) {
        return urls;
    }

    const auto currentIndex = std::distance(candidates.cbegin(), current);
    auto appendOffset = [&urls, &candidates, currentIndex](std::ptrdiff_t offset) {
        const std::ptrdiff_t targetIndex = currentIndex + offset;
        if (targetIndex < 0 || targetIndex >= static_cast<std::ptrdiff_t>(candidates.size())) {
            return;
        }

        urls.push_back(candidates.at(static_cast<std::size_t>(targetIndex)).url);
    };

    appendOffset(0);
    for (std::ptrdiff_t offset = 1; offset <= predecodeNextImageCount; ++offset) {
        appendOffset(offset);
        if (offset <= predecodePreviousImageCount) {
            appendOffset(-offset);
        }
    }
    return urls;
}

std::vector<QUrl> imageNavigationCandidateUrls(
    const std::vector<ImageNavigationCandidate> &candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());

    for (const ImageNavigationCandidate &candidate : candidates) {
        urls.push_back(candidate.url);
    }
    return urls;
}

std::optional<QUrl> adjacentImageNavigationUrl(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl,
    NavigationDirection direction)
{
    const std::optional<std::size_t> targetIndex
        = adjacentCandidateIndex(candidates, currentUrl, direction);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    return candidates.at(*targetIndex).url;
}

std::optional<ContainerNavigationCandidate> adjacentContainerNavigationCandidate(
    const std::vector<ContainerNavigationCandidate> &candidates, const QUrl &currentContainerUrl,
    NavigationDirection direction)
{
    const std::optional<std::size_t> targetIndex
        = adjacentCandidateIndex(candidates, currentContainerUrl, direction);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    return candidates.at(*targetIndex);
}

PageNavigationState pageNavigationStateForUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    PageNavigationState state { std::move(urls), -1 };
    const auto matchesCurrentUrl
        = [&currentUrl](const QUrl &url) { return sameNormalizedUrl(url, currentUrl); };
    const auto current = std::find_if(state.urls.cbegin(), state.urls.cend(), matchesCurrentUrl);

    if (current == state.urls.cend()) {
        if (currentUrl.isValid() && !currentUrl.isEmpty()) {
            state.urls.insert(state.urls.begin(), normalizedImageUrl(currentUrl));
            state.currentIndex = 0;
        }
    } else {
        state.currentIndex = static_cast<int>(std::distance(state.urls.cbegin(), current));
    }

    return state;
}

void sortImageNavigationCandidates(std::vector<ImageNavigationCandidate> *candidates)
{
    sortNavigationCandidates(candidates);
}

void sortContainerNavigationCandidates(std::vector<ContainerNavigationCandidate> *candidates)
{
    sortNavigationCandidates(candidates);
}

std::vector<ImageNavigationCandidate> imageNavigationCandidates(const KFileItemList &items)
{
    std::vector<ImageNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (!item.isFile() || !KiriView::isSupportedImageFileName(name)) {
            continue;
        }

        candidates.push_back(ImageNavigationCandidate { item.url(), name });
    }

    sortImageNavigationCandidates(&candidates);
    return candidates;
}
}
