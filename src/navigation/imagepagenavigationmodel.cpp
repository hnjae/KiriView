// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepagenavigationmodel.h"

#include "imagenavigationmodel.h"

#include <cstddef>
#include <utility>

namespace KiriView {
int ImagePageNavigationModel::currentPageNumber() const
{
    return pageNavigationCurrentPageNumber(m_state);
}

int ImagePageNavigationModel::imageCount() const { return pageNavigationImageCount(m_state); }

ImagePageNavigationSnapshot ImagePageNavigationModel::snapshot() const
{
    return ImagePageNavigationSnapshot { m_state };
}

bool ImagePageNavigationModel::hasKnownSelection() const
{
    return pageNavigationHasKnownSelection(m_state);
}

std::optional<QUrl> ImagePageNavigationModel::urlAtPage(int pageNumber) const
{
    return pageNavigationUrlAtPage(m_state, pageNumber);
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
    return m_refreshState.shouldKeepExistingWatcherFor(context);
}

bool ImagePageNavigationModel::previewRefresh(const ImageCandidateListContext &context)
{
    const ImagePageNavigationRefreshPreview preview
        = m_refreshState.beginRefresh(context, !m_state.urls.empty());
    if (preview.keepKnownUrls) {
        return replaceState(pageNavigationStateForCurrentUrl(m_state, context.currentUrl()), true);
    }

    return replaceState(PageNavigationState {});
}

bool ImagePageNavigationModel::completeRefresh(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl,
    ImageCandidateListSource source)
{
    return completeRefresh(imageNavigationCandidateUrls(candidates),
        ImageCandidateListContext::forSource(currentUrl, std::move(source)));
}

ImagePageNavigationRefreshResult ImagePageNavigationModel::completeRefreshFromCurrentContext(
    const std::vector<ImageNavigationCandidate> &candidates, ImageCandidateListSource source)
{
    std::optional<ImageCandidateListContext> context
        = m_refreshState.acceptedPendingRefreshContext(std::move(source));
    if (!context.has_value()) {
        return {};
    }

    const bool currentImageRemoved
        = !imageNavigationCandidatesContainUrl(candidates, context->currentUrl());
    const bool changed = completeRefresh(imageNavigationCandidateUrls(candidates), *context);
    return ImagePageNavigationRefreshResult { true, changed, currentImageRemoved, context };
}

bool ImagePageNavigationModel::completeRefresh(
    std::vector<QUrl> urls, ImageCandidateListContext context)
{
    const QUrl currentUrl = context.currentUrl();
    m_refreshState.finishRefresh(std::move(context));
    return replaceState(pageNavigationStateForUrls(std::move(urls), currentUrl));
}

bool ImagePageNavigationModel::clear()
{
    m_refreshState.clear();
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
