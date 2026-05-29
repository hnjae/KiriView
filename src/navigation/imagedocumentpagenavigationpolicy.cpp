// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagenavigationpolicy.h"

#include "kiriview/src/policy/imagedocumentpagenavigation.cxx.h"
#include "location/imageurl.h"
#include "navigationcandidateordering.h"

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

std::optional<std::size_t> currentUrlIndex(const std::vector<QUrl> &urls, const QUrl &currentUrl)
{
    const auto current = std::find_if(urls.cbegin(), urls.cend(),
        [&currentUrl](const QUrl &url) { return KiriView::sameNormalizedUrl(url, currentUrl); });
    if (current == urls.cend()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(urls.cbegin(), current));
}

std::optional<std::size_t> pageNavigationIndexValue(KiriView::RustNavigationIndex index)
{
    if (!index.found) {
        return std::nullopt;
    }

    return index.index;
}

KiriView::RustNavigationIndex pageRustNavigationIndex(std::optional<std::size_t> index)
{
    return KiriView::RustNavigationIndex { index.has_value(), index.value_or(0) };
}
}

namespace KiriView {
PageNavigationState::PageNavigationState(std::vector<QUrl> urls, int currentIndex)
    : targets()
    , currentIndex(currentIndex)
{
    targets.reserve(urls.size());
    for (QUrl &url : urls) {
        targets.push_back(ImageDocumentPageTarget {
            url, ImageDocumentPageKind::Image, url.fileName(QUrl::PrettyDecoded) });
    }
}

int ImageDocumentPageNavigationSnapshot::currentPageNumber() const
{
    return pageNavigationCurrentPageNumber(state);
}

int ImageDocumentPageNavigationSnapshot::pageCount() const
{
    return pageNavigationPageCount(state);
}

std::optional<QUrl> ImageDocumentPageNavigationSnapshot::urlAtPage(int pageNumber) const
{
    return pageNavigationUrlAtPage(state, pageNumber);
}

std::vector<QUrl> imageDocumentPageCandidateUrls(
    const std::vector<ImageDocumentPageCandidate> &candidates)
{
    return imageDocumentPageTargetUrls(imageDocumentPageCandidateTargets(candidates));
}

std::vector<ImageDocumentPageTarget> imageDocumentPageCandidateTargets(
    const std::vector<ImageDocumentPageCandidate> &candidates)
{
    std::vector<ImageDocumentPageTarget> targets;
    targets.reserve(candidates.size());
    for (const ImageDocumentPageCandidate &candidate : candidates) {
        targets.push_back(
            ImageDocumentPageTarget { candidate.url, candidate.kind, candidate.name });
    }

    return targets;
}

std::vector<QUrl> imageDocumentPageTargetUrls(const std::vector<ImageDocumentPageTarget> &targets)
{
    std::vector<QUrl> urls;
    urls.reserve(targets.size());
    for (const ImageDocumentPageTarget &target : targets) {
        urls.push_back(target.url);
    }

    return urls;
}

std::vector<QUrl> stillImageDocumentPageCandidateUrls(
    const std::vector<ImageDocumentPageCandidate> &candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());
    for (const ImageDocumentPageCandidate &candidate : candidates) {
        if (imageDocumentPageCandidateIsImage(candidate)) {
            urls.push_back(candidate.url);
        }
    }

    return urls;
}

bool imageDocumentPageCandidateIsImage(const ImageDocumentPageCandidate &candidate)
{
    return candidate.kind == ImageDocumentPageKind::Image;
}

bool imageDocumentPageCandidateIsVideo(const ImageDocumentPageCandidate &candidate)
{
    return candidate.kind == ImageDocumentPageKind::Video;
}

std::optional<std::size_t> imageDocumentPageCandidateIndex(
    const std::vector<ImageDocumentPageCandidate> &candidates, const QUrl &currentUrl)
{
    return navigationCandidateIndex(candidates, currentUrl);
}

bool imageDocumentPageCandidatesContainUrl(
    const std::vector<ImageDocumentPageCandidate> &candidates, const QUrl &url)
{
    return imageDocumentPageCandidateIndex(candidates, url).has_value();
}

std::optional<QUrl> adjacentImageDocumentPageUrl(
    const std::vector<ImageDocumentPageCandidate> &candidates, const QUrl &currentUrl,
    NavigationDirection direction)
{
    return adjacentNavigationCandidateUrl(candidates, currentUrl, direction);
}

std::optional<ImageDocumentPageCandidate> adjacentImageDocumentPageCandidate(
    const std::vector<ImageDocumentPageCandidate> &candidates, const QUrl &currentUrl,
    NavigationDirection direction)
{
    return adjacentNavigationCandidate(candidates, currentUrl, direction);
}

std::optional<ContainerNavigationCandidate> adjacentContainerNavigationCandidate(
    const std::vector<ContainerNavigationCandidate> &candidates, const QUrl &currentContainerUrl,
    NavigationDirection direction)
{
    return adjacentNavigationCandidate(candidates, currentContainerUrl, direction);
}

std::optional<std::size_t> containerNavigationCandidateIndex(
    const std::vector<ContainerNavigationCandidate> &candidates, const QUrl &currentContainerUrl)
{
    return navigationCandidateIndex(candidates, currentContainerUrl);
}

int pageNavigationCurrentPageNumber(const PageNavigationState &state)
{
    return state.currentIndex < 0 ? 0 : state.currentIndex + 1;
}

int pageNavigationPageCount(const PageNavigationState &state)
{
    return static_cast<int>(state.targets.size());
}

bool pageNavigationHasKnownSelection(const PageNavigationState &state)
{
    if (state.currentIndex < 0) {
        return false;
    }

    return static_cast<std::size_t>(state.currentIndex) < state.targets.size();
}

std::optional<QUrl> pageNavigationUrlAtPage(const PageNavigationState &state, int pageNumber)
{
    const std::optional<ImageDocumentPageTarget> target
        = pageNavigationTargetAtPage(state, pageNumber);
    if (!target.has_value()) {
        return std::nullopt;
    }

    return target->url;
}

std::optional<ImageDocumentPageTarget> pageNavigationTargetAtPage(
    const PageNavigationState &state, int pageNumber)
{
    if (pageNumber < 1) {
        return std::nullopt;
    }

    const std::size_t pageIndex = static_cast<std::size_t>(pageNumber - 1);
    if (pageIndex >= state.targets.size()) {
        return std::nullopt;
    }

    return state.targets.at(pageIndex);
}

std::optional<std::size_t> pageNavigationTargetIndex(
    const PageNavigationState &state, int pageNumber)
{
    return pageNavigationIndexValue(
        rustPageNavigationTargetIndex(state.targets.size(), state.currentIndex, pageNumber));
}

std::optional<std::size_t> pageNavigationAdjacentTargetIndex(
    const PageNavigationState &state, NavigationDirection direction)
{
    return pageNavigationIndexValue(rustPageNavigationAdjacentTargetIndex(
        state.targets.size(), state.currentIndex, rustNavigationDirection(direction)));
}

PageNavigationState pageNavigationStateForCurrentUrl(
    const PageNavigationState &knownState, const QUrl &currentUrl)
{
    const std::vector<QUrl> knownUrls = imageDocumentPageTargetUrls(knownState.targets);
    const RustPageNavigationPreviewState preview = rustPageNavigationPreviewState(
        pageRustNavigationIndex(currentUrlIndex(knownUrls, currentUrl)),
        currentUrl.isValid() && !currentUrl.isEmpty(), knownState.targets.size());

    if (!preview.keep_known_urls) {
        return {};
    }

    return PageNavigationState { knownState.targets, preview.current_index };
}

PageNavigationState pageNavigationStateForTargets(
    std::vector<ImageDocumentPageTarget> targets, const QUrl &currentUrl)
{
    PageNavigationState state { std::move(targets), -1 };
    std::vector<QUrl> urls = imageDocumentPageTargetUrls(state.targets);
    const RustPageNavigationUpdate update
        = rustPageNavigationStateUpdate(pageRustNavigationIndex(currentUrlIndex(urls, currentUrl)),
            currentUrl.isValid() && !currentUrl.isEmpty(), state.targets.size());
    if (update.insert_current_url) {
        const QUrl normalizedUrl = normalizedImageUrl(currentUrl);
        state.targets.insert(state.targets.begin(),
            ImageDocumentPageTarget { normalizedUrl, ImageDocumentPageKind::Image,
                normalizedUrl.fileName(QUrl::PrettyDecoded) });
    }
    state.currentIndex = update.current_index;

    return state;
}

PageNavigationState pageNavigationStateForUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    return pageNavigationStateForTargets(PageNavigationState(std::move(urls)).targets, currentUrl);
}

bool samePageNavigationState(const PageNavigationState &left, const PageNavigationState &right)
{
    return left.targets == right.targets && left.currentIndex == right.currentIndex;
}

void sortImageDocumentPageCandidates(std::vector<ImageDocumentPageCandidate> *candidates)
{
    sortNavigationCandidatesByNameAndUrl(candidates);
}

void sortContainerNavigationCandidates(std::vector<ContainerNavigationCandidate> *candidates)
{
    sortNavigationCandidatesByNameAndUrl(candidates);
}

}
