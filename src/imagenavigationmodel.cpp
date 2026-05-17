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
    return state.currentIndex < 0 ? 0 : state.currentIndex + 1;
}

int ImagePageNavigationSnapshot::imageCount() const { return static_cast<int>(state.urls.size()); }

std::optional<QUrl> ImagePageNavigationSnapshot::urlAtPage(int pageNumber) const
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
