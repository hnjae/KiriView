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

std::uint8_t rustFlag(bool value) { return value ? 1 : 0; }

template <typename Candidate>
rust::Vec<std::uint8_t> currentCandidateMatches(
    const std::vector<Candidate> &candidates, const QUrl &currentUrl)
{
    rust::Vec<std::uint8_t> matches;
    matches.reserve(candidates.size());
    for (const Candidate &candidate : candidates) {
        matches.push_back(rustFlag(KiriView::sameNormalizedUrl(candidate.url, currentUrl)));
    }

    return matches;
}

rust::Vec<std::uint8_t> currentUrlMatches(const std::vector<QUrl> &urls, const QUrl &currentUrl)
{
    rust::Vec<std::uint8_t> matches;
    matches.reserve(urls.size());
    for (const QUrl &url : urls) {
        matches.push_back(rustFlag(KiriView::sameNormalizedUrl(url, currentUrl)));
    }

    return matches;
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

template <typename Candidate>
std::optional<std::size_t> adjacentCandidateIndex(const std::vector<Candidate> &candidates,
    const QUrl &currentUrl, KiriView::NavigationDirection direction)
{
    return navigationIndexValue(KiriView::rustAdjacentNavigationCandidateIndex(candidates.size(),
        KiriView::rustCurrentNavigationIndex(currentCandidateMatches(candidates, currentUrl)),
        rustNavigationDirection(direction)));
}
}

namespace KiriView {
std::vector<QUrl> predecodeWindowImageUrls(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    std::vector<QUrl> urls;
    const rust::Vec<std::size_t> indices = rustPredecodeWindowImageIndices(candidates.size(),
        rustCurrentNavigationIndex(currentCandidateMatches(candidates, currentUrl)));
    urls.reserve(indices.size());
    for (std::size_t index : indices) {
        urls.push_back(candidates.at(index).url);
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

std::optional<std::size_t> pageNavigationTargetIndex(
    const PageNavigationState &state, int pageNumber)
{
    return navigationIndexValue(
        rustPageNavigationTargetIndex(state.urls.size(), state.currentIndex, pageNumber));
}

PageNavigationState pageNavigationStateForCurrentUrl(
    const PageNavigationState &knownState, const QUrl &currentUrl)
{
    const RustPageNavigationPreviewState preview = rustPageNavigationPreviewState(
        rustCurrentNavigationIndex(currentUrlMatches(knownState.urls, currentUrl)),
        currentUrl.isValid() && !currentUrl.isEmpty(), knownState.urls.size());

    switch (preview.urls) {
    case RustPageNavigationUrlsTarget::Empty:
        return {};
    case RustPageNavigationUrlsTarget::Known:
        return PageNavigationState { knownState.urls, preview.current_index };
    case RustPageNavigationUrlsTarget::Current:
        return PageNavigationState { { normalizedImageUrl(currentUrl) }, preview.current_index };
    }

    return {};
}

PageNavigationState pageNavigationStateForUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    PageNavigationState state { std::move(urls), -1 };
    const RustPageNavigationUpdate update = rustPageNavigationStateUpdate(
        rustCurrentNavigationIndex(currentUrlMatches(state.urls, currentUrl)),
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
