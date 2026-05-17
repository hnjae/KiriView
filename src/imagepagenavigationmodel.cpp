// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepagenavigationmodel.h"

#include "imagenavigationmodel.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace {
std::vector<QUrl> imageNavigationCandidateUrls(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates)
{
    std::vector<QUrl> urls;
    urls.reserve(candidates.size());
    for (const KiriView::ImageNavigationCandidate &candidate : candidates) {
        urls.push_back(candidate.url);
    }
    return urls;
}

bool imageNavigationCandidatesContainUrl(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &url)
{
    return std::any_of(candidates.cbegin(), candidates.cend(),
        [&url](const KiriView::ImageNavigationCandidate &candidate) {
            return KiriView::sameNormalizedUrl(candidate.url, url);
        });
}

bool samePageNavigationState(
    const KiriView::PageNavigationState &left, const KiriView::PageNavigationState &right)
{
    return left.urls == right.urls && left.currentIndex == right.currentIndex;
}
}

namespace KiriView {
int ImagePageNavigationModel::currentPageNumber() const
{
    return m_state.currentIndex < 0 ? 0 : m_state.currentIndex + 1;
}

int ImagePageNavigationModel::imageCount() const { return static_cast<int>(m_state.urls.size()); }

bool ImagePageNavigationModel::hasKnownSelection() const
{
    if (m_state.currentIndex < 0) {
        return false;
    }

    return static_cast<std::size_t>(m_state.currentIndex) < m_state.urls.size();
}

std::optional<QUrl> ImagePageNavigationModel::urlAtPage(int pageNumber) const
{
    if (pageNumber < 1) {
        return std::nullopt;
    }

    const std::size_t pageIndex = static_cast<std::size_t>(pageNumber - 1);
    if (pageIndex >= m_state.urls.size()) {
        return std::nullopt;
    }

    return m_state.urls.at(pageIndex);
}

std::optional<QUrl> ImagePageNavigationModel::selectPage(int pageNumber)
{
    const std::optional<std::size_t> targetIndex = pageNavigationTargetIndex(m_state, pageNumber);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    PageNavigationState state = m_state;
    state.currentIndex = static_cast<int>(*targetIndex);
    const QUrl targetUrl = state.urls.at(*targetIndex);
    replaceState(std::move(state));
    return targetUrl;
}

std::optional<QUrl> ImagePageNavigationModel::selectAdjacentPage(NavigationDirection direction)
{
    const std::optional<std::size_t> targetIndex
        = pageNavigationAdjacentTargetIndex(m_state, direction);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    PageNavigationState state = m_state;
    state.currentIndex = static_cast<int>(*targetIndex);
    const QUrl targetUrl = state.urls.at(*targetIndex);
    replaceState(std::move(state));
    return targetUrl;
}

bool ImagePageNavigationModel::shouldKeepExistingWatcherFor(
    const ImageCandidateListContext &context) const
{
    return m_knownContext.has_value()
        && sameImageCandidateListSource(m_knownContext->source(), context.source());
}

bool ImagePageNavigationModel::previewRefresh(const ImageCandidateListContext &context)
{
    const bool canReuseKnownState = m_knownContext.has_value()
        && sameImageCandidateListSource(m_knownContext->source(), context.source())
        && !m_state.urls.empty();

    bool changed = false;
    if (canReuseKnownState) {
        changed
            = replaceState(pageNavigationStateForCurrentUrl(m_state, context.currentUrl()), true);
    } else {
        m_knownContext = std::nullopt;
        changed = replaceState(PageNavigationState {});
    }

    m_refreshContext = context;
    return changed;
}

bool ImagePageNavigationModel::completeRefresh(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl,
    ImageCandidateListSource source)
{
    return completeRefresh(imageNavigationCandidateUrls(candidates), currentUrl, std::move(source));
}

ImagePageNavigationRefreshResult ImagePageNavigationModel::completeRefreshFromCurrentContext(
    const std::vector<ImageNavigationCandidate> &candidates, ImageCandidateListSource source)
{
    if (!m_refreshContext.has_value()
        || !sameImageCandidateListSource(m_refreshContext->source(), source)) {
        return {};
    }

    const ImageCandidateListContext context = *m_refreshContext;
    const bool currentImageRemoved
        = !imageNavigationCandidatesContainUrl(candidates, context.currentUrl());
    const bool changed = completeRefresh(
        imageNavigationCandidateUrls(candidates), context.currentUrl(), std::move(source));
    return ImagePageNavigationRefreshResult { true, changed, currentImageRemoved, context };
}

bool ImagePageNavigationModel::completeRefresh(
    std::vector<QUrl> urls, const QUrl &currentUrl, ImageCandidateListSource source)
{
    m_knownContext = ImageCandidateListContext::forSource(currentUrl, std::move(source));
    return replaceState(pageNavigationStateForUrls(std::move(urls), currentUrl));
}

bool ImagePageNavigationModel::clear()
{
    m_knownContext = std::nullopt;
    m_refreshContext = std::nullopt;
    return replaceState(PageNavigationState {});
}

bool ImagePageNavigationModel::replaceState(PageNavigationState state, bool forceChanged)
{
    if (!forceChanged && samePageNavigationState(m_state, state)) {
        return false;
    }

    m_state = std::move(state);
    return true;
}
}
