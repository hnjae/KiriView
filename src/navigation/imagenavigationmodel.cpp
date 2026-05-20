// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationmodel.h"

#include "kiriview/src/policy/imagenavigationmodel.cxx.h"
#include "location/imageurl.h"

#include <QCollator>
#include <QLocale>
#include <Qt>
#include <algorithm>
#include <cstddef>
#include <cstdint>
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

template <typename Candidate>
std::optional<std::size_t> currentCandidateIndex(
    const std::vector<Candidate> &candidates, const QUrl &currentUrl)
{
    const auto currentCandidate = std::find_if(
        candidates.cbegin(), candidates.cend(), [&currentUrl](const Candidate &candidate) {
            return KiriView::sameNormalizedUrl(candidate.url, currentUrl);
        });
    if (currentCandidate == candidates.cend()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(candidates.cbegin(), currentCandidate));
}

std::optional<std::size_t> currentUrlIndex(const std::vector<QUrl> &urls, const QUrl &currentUrl)
{
    const auto current = std::find_if(urls.cbegin(), urls.cend(),
        [&currentUrl](const QUrl &url) { return KiriView::sameNormalizedUrl(url, currentUrl); });
    if (current == urls.cend()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(urls.cbegin(), current));
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

std::optional<std::size_t> navigationIndexValue(KiriView::RustNavigationIndex index)
{
    if (!index.found) {
        return std::nullopt;
    }

    return index.index;
}

KiriView::RustNavigationIndex rustNavigationIndex(std::optional<std::size_t> index)
{
    return KiriView::RustNavigationIndex { index.has_value(), index.value_or(0) };
}

template <typename Candidate>
std::optional<std::size_t> adjacentCandidateIndex(const std::vector<Candidate> &candidates,
    const QUrl &currentUrl, KiriView::NavigationDirection direction)
{
    return navigationIndexValue(KiriView::rustAdjacentNavigationCandidateIndex(candidates.size(),
        rustNavigationIndex(currentCandidateIndex(candidates, currentUrl)),
        rustNavigationDirection(direction)));
}
}

namespace KiriView {
int ImagePageNavigationSnapshot::currentPageNumber() const
{
    return pageNavigationCurrentPageNumber(state);
}

int ImagePageNavigationSnapshot::imageCount() const { return pageNavigationImageCount(state); }

std::optional<QUrl> ImagePageNavigationSnapshot::urlAtPage(int pageNumber) const
{
    return pageNavigationUrlAtPage(state, pageNumber);
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

std::optional<std::size_t> imageNavigationCandidateIndex(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    return currentCandidateIndex(candidates, currentUrl);
}

bool imageNavigationCandidatesContainUrl(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &url)
{
    return imageNavigationCandidateIndex(candidates, url).has_value();
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

int pageNavigationCurrentPageNumber(const PageNavigationState &state)
{
    return state.currentIndex < 0 ? 0 : state.currentIndex + 1;
}

int pageNavigationImageCount(const PageNavigationState &state)
{
    return static_cast<int>(state.urls.size());
}

bool pageNavigationHasKnownSelection(const PageNavigationState &state)
{
    if (state.currentIndex < 0) {
        return false;
    }

    return static_cast<std::size_t>(state.currentIndex) < state.urls.size();
}

std::optional<QUrl> pageNavigationUrlAtPage(const PageNavigationState &state, int pageNumber)
{
    if (pageNumber < 1) {
        return std::nullopt;
    }

    const std::size_t pageIndex = static_cast<std::size_t>(pageNumber - 1);
    if (pageIndex >= state.urls.size()) {
        return std::nullopt;
    }

    return state.urls.at(pageIndex);
}

std::optional<std::size_t> pageNavigationTargetIndex(
    const PageNavigationState &state, int pageNumber)
{
    return navigationIndexValue(
        rustPageNavigationTargetIndex(state.urls.size(), state.currentIndex, pageNumber));
}

std::optional<std::size_t> pageNavigationAdjacentTargetIndex(
    const PageNavigationState &state, NavigationDirection direction)
{
    return navigationIndexValue(rustPageNavigationAdjacentTargetIndex(
        state.urls.size(), state.currentIndex, rustNavigationDirection(direction)));
}

PageNavigationState pageNavigationStateForCurrentUrl(
    const PageNavigationState &knownState, const QUrl &currentUrl)
{
    const RustPageNavigationPreviewState preview = rustPageNavigationPreviewState(
        rustNavigationIndex(currentUrlIndex(knownState.urls, currentUrl)),
        currentUrl.isValid() && !currentUrl.isEmpty(), knownState.urls.size());

    if (!preview.keep_known_urls) {
        return {};
    }

    return PageNavigationState { knownState.urls, preview.current_index };
}

PageNavigationState pageNavigationStateForUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    PageNavigationState state { std::move(urls), -1 };
    const RustPageNavigationUpdate update = rustPageNavigationStateUpdate(
        rustNavigationIndex(currentUrlIndex(state.urls, currentUrl)),
        currentUrl.isValid() && !currentUrl.isEmpty(), state.urls.size());
    if (update.insert_current_url) {
        state.urls.insert(state.urls.begin(), normalizedImageUrl(currentUrl));
    }
    state.currentIndex = update.current_index;

    return state;
}

bool samePageNavigationState(const PageNavigationState &left, const PageNavigationState &right)
{
    return left.urls == right.urls && left.currentIndex == right.currentIndex;
}

void sortImageNavigationCandidates(std::vector<ImageNavigationCandidate> *candidates)
{
    sortNavigationCandidates(candidates);
}

void sortContainerNavigationCandidates(std::vector<ContainerNavigationCandidate> *candidates)
{
    sortNavigationCandidates(candidates);
}

}
