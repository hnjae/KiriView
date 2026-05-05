// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationmodel.h"

#include "imageformatregistry.h"
#include "imageurl.h"
#include "kiriview/src/imagenavigationmodel.cxx.h"

#include <QCollator>
#include <QLocale>
#include <Qt>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>

namespace {
KiriView::RustNavigationDirection rustNavigationDirection(KiriView::NavigationDirection direction)
{
    switch (direction) {
    case KiriView::NavigationDirection::Previous:
        return KiriView::RustNavigationDirection::Previous;
    case KiriView::NavigationDirection::Next:
        return KiriView::RustNavigationDirection::Next;
    }

    return KiriView::RustNavigationDirection::Next;
}

KiriView::RustNavigationIndex rustNavigationIndex(std::optional<std::size_t> index)
{
    if (!index.has_value()) {
        return KiriView::RustNavigationIndex { false, 0 };
    }

    return KiriView::RustNavigationIndex { true, *index };
}

template <typename Candidate>
std::optional<std::size_t> currentCandidateIndex(
    const std::vector<Candidate> &candidates, const QUrl &currentUrl)
{
    const auto current = std::find_if(
        candidates.cbegin(), candidates.cend(), [&currentUrl](const Candidate &candidate) {
            return KiriView::sameNormalizedUrl(candidate.url, currentUrl);
        });
    if (current == candidates.cend()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(candidates.cbegin(), current));
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
    const rust::Vec<std::size_t> indices = rustPredecodeWindowImageIndices(
        candidates.size(), rustNavigationIndex(currentCandidateIndex(candidates, currentUrl)));
    urls.reserve(indices.size());
    for (std::size_t index : indices) {
        urls.push_back(candidates.at(index).url);
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
    const RustNavigationIndex targetIndex = rustAdjacentNavigationCandidateIndex(candidates.size(),
        rustNavigationIndex(currentCandidateIndex(candidates, currentUrl)),
        rustNavigationDirection(direction));
    if (!targetIndex.found) {
        return std::nullopt;
    }

    return candidates.at(targetIndex.index).url;
}

std::optional<ContainerNavigationCandidate> adjacentContainerNavigationCandidate(
    const std::vector<ContainerNavigationCandidate> &candidates, const QUrl &currentContainerUrl,
    NavigationDirection direction)
{
    const RustNavigationIndex targetIndex = rustAdjacentNavigationCandidateIndex(candidates.size(),
        rustNavigationIndex(currentCandidateIndex(candidates, currentContainerUrl)),
        rustNavigationDirection(direction));
    if (!targetIndex.found) {
        return std::nullopt;
    }

    return candidates.at(targetIndex.index);
}

PageNavigationState pageNavigationStateForUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    PageNavigationState state { std::move(urls), -1 };
    const auto matchesCurrentUrl
        = [&currentUrl](const QUrl &url) { return sameNormalizedUrl(url, currentUrl); };
    const auto current = std::find_if(state.urls.cbegin(), state.urls.cend(), matchesCurrentUrl);

    std::optional<std::size_t> currentIndex;
    if (current != state.urls.cend()) {
        currentIndex = static_cast<std::size_t>(std::distance(state.urls.cbegin(), current));
    }
    const RustPageNavigationUpdate update = rustPageNavigationStateUpdate(
        rustNavigationIndex(currentIndex), currentUrl.isValid() && !currentUrl.isEmpty());
    if (update.insert_current_url) {
        state.urls.insert(state.urls.begin(), normalizedImageUrl(currentUrl));
    }
    state.currentIndex = update.current_index;

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
