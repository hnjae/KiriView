// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepagenavigationmodel.h"

#include "imagecontainer.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <cstddef>
#include <utility>

namespace {
bool samePageNavigationState(
    const KiriView::PageNavigationState &left, const KiriView::PageNavigationState &right)
{
    return left.urls == right.urls && left.currentIndex == right.currentIndex;
}

bool sameArchiveDocumentLocation(
    const KiriView::ArchiveDocumentLocation &left, const KiriView::ArchiveDocumentLocation &right)
{
    return KiriView::sameNormalizedUrl(left.fileUrl(), right.fileUrl())
        && KiriView::sameNormalizedUrl(left.rootUrl(), right.rootUrl())
        && left.kind() == right.kind();
}

bool sameImageCandidateListSourcePayload(const KiriView::ImageCandidateListSource::Directory &left,
    const KiriView::ImageCandidateListSource::Directory &right)
{
    return KiriView::sameNormalizedUrl(left.directoryUrl, right.directoryUrl);
}

bool sameImageCandidateListSourcePayload(
    const KiriView::ImageCandidateListSource::ArchiveDocument &left,
    const KiriView::ImageCandidateListSource::ArchiveDocument &right)
{
    return sameArchiveDocumentLocation(left.archiveDocument, right.archiveDocument);
}

template <typename Left, typename Right>
bool sameImageCandidateListSourcePayload(const Left &, const Right &)
{
    return false;
}

bool sameImageCandidateListSource(
    const KiriView::ImageCandidateListSource &left, const KiriView::ImageCandidateListSource &right)
{
    return left.visit([&right](const auto &leftSource) {
        return right.visit([&leftSource](const auto &rightSource) {
            return sameImageCandidateListSourcePayload(leftSource, rightSource);
        });
    });
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
    return m_source.has_value() && sameImageCandidateListSource(*m_source, context.source());
}

bool ImagePageNavigationModel::isCurrentRefreshSource(const ImageCandidateListSource &source) const
{
    return m_refreshContext.has_value()
        && sameImageCandidateListSource(m_refreshContext->source(), source);
}

std::optional<ImageCandidateListContext> ImagePageNavigationModel::refreshContext() const
{
    return m_refreshContext;
}

bool ImagePageNavigationModel::previewRefresh(const ImageCandidateListContext &context)
{
    const ImageCandidateListSource source = context.source();
    const bool canReuseKnownState = m_source.has_value()
        && sameImageCandidateListSource(*m_source, source) && !m_state.urls.empty();

    bool changed = false;
    if (canReuseKnownState) {
        changed
            = replaceState(pageNavigationStateForCurrentUrl(m_state, context.currentUrl()), true);
    } else {
        m_source = std::nullopt;
        changed = replaceState(PageNavigationState {});
    }

    m_refreshContext = context;
    return changed;
}

bool ImagePageNavigationModel::completeRefresh(
    std::vector<QUrl> urls, const QUrl &currentUrl, ImageCandidateListSource source)
{
    m_source = std::move(source);
    return replaceState(pageNavigationStateForUrls(std::move(urls), currentUrl));
}

bool ImagePageNavigationModel::clear()
{
    m_source = std::nullopt;
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
