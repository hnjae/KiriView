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

std::optional<ImageNavigationTarget> ImagePageNavigationModel::targetAtPage(int pageNumber) const
{
    return pageNavigationTargetAtPage(m_state, pageNumber);
}

std::optional<ImageNavigationTarget> ImagePageNavigationModel::selectPage(int pageNumber)
{
    const std::optional<std::size_t> targetIndex = pageNavigationTargetIndex(m_state, pageNumber);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    PageNavigationState state = m_state;
    state.currentIndex = static_cast<int>(*targetIndex);
    const ImageNavigationTarget target = state.targets.at(*targetIndex);
    replaceState(std::move(state));
    return target;
}

std::optional<ImageNavigationTarget> ImagePageNavigationModel::selectAdjacentPage(
    NavigationDirection direction)
{
    const std::optional<std::size_t> targetIndex
        = pageNavigationAdjacentTargetIndex(m_state, direction);
    if (!targetIndex.has_value()) {
        return std::nullopt;
    }

    PageNavigationState state = m_state;
    state.currentIndex = static_cast<int>(*targetIndex);
    const ImageNavigationTarget target = state.targets.at(*targetIndex);
    replaceState(std::move(state));
    return target;
}

bool ImagePageNavigationModel::shouldKeepExistingWatcherFor(
    const ImageCandidateListContext &context) const
{
    return m_knownRefreshContext.has_value()
        && sameImageCandidateListSource(m_knownRefreshContext->source(), context.source());
}

ImagePageNavigationRefreshPlan ImagePageNavigationModel::beginRefresh(
    const ImageCandidateListContext &context)
{
    const quint64 refreshId = m_pendingRefresh.start();
    const bool keepKnownUrls = m_knownRefreshContext.has_value() && !m_state.targets.empty()
        && sameImageCandidateListSource(m_knownRefreshContext->source(), context.source());
    if (!keepKnownUrls) {
        m_knownRefreshContext = std::nullopt;
    }

    m_pendingRefreshContext = context;

    bool changed = false;
    if (keepKnownUrls) {
        changed
            = replaceState(pageNavigationStateForCurrentUrl(m_state, context.currentUrl()), true);
    } else {
        changed = replaceState(PageNavigationState {});
    }

    return ImagePageNavigationRefreshPlan { changed, refreshId };
}

bool ImagePageNavigationModel::completeRefresh(
    const std::vector<ImageNavigationCandidate> &candidates, const QUrl &currentUrl,
    ImageCandidateListSource source)
{
    return completeRefresh(imageNavigationCandidateTargets(candidates),
        ImageCandidateListContext::forSource(currentUrl, std::move(source)));
}

ImagePageNavigationRefreshResult ImagePageNavigationModel::completePendingRefresh(
    const std::vector<ImageNavigationCandidate> &candidates, quint64 refreshId,
    ImageCandidateListSource source)
{
    std::optional<ImageCandidateListContext> context
        = acceptedPendingRefreshContext(refreshId, std::move(source));
    if (!context.has_value()) {
        return {};
    }

    m_pendingRefresh.finish(refreshId);
    return completeRefreshFromCurrentContext(candidates, *context);
}

ImagePageNavigationRefreshResult ImagePageNavigationModel::completeWatchedRefreshFromCurrentContext(
    const std::vector<ImageNavigationCandidate> &candidates, ImageCandidateListSource source)
{
    std::optional<ImageCandidateListContext> context
        = acceptedWatchedRefreshContext(std::move(source));
    if (!context.has_value()) {
        return {};
    }

    return completeRefreshFromCurrentContext(candidates, *context);
}

ImagePageNavigationRefreshResult ImagePageNavigationModel::completeRefreshFromCurrentContext(
    const std::vector<ImageNavigationCandidate> &candidates, ImageCandidateListContext context)
{
    const bool currentImageRemoved
        = !imageNavigationCandidatesContainUrl(candidates, context.currentUrl());
    const bool changed = completeRefresh(imageNavigationCandidateTargets(candidates), context);
    return ImagePageNavigationRefreshResult { true, changed, currentImageRemoved, context };
}

std::optional<ImageCandidateListContext> ImagePageNavigationModel::acceptedPendingRefreshContext(
    quint64 refreshId, ImageCandidateListSource source) const
{
    if (!m_pendingRefresh.accepts(refreshId)) {
        return std::nullopt;
    }

    return acceptedWatchedRefreshContext(std::move(source));
}

std::optional<ImageCandidateListContext> ImagePageNavigationModel::acceptedWatchedRefreshContext(
    ImageCandidateListSource source) const
{
    if (!m_pendingRefreshContext.has_value()
        || !sameImageCandidateListSource(m_pendingRefreshContext->source(), source)) {
        return std::nullopt;
    }

    return ImageCandidateListContext::forSource(
        m_pendingRefreshContext->currentUrl(), std::move(source));
}

bool ImagePageNavigationModel::completeRefresh(
    std::vector<ImageNavigationTarget> targets, ImageCandidateListContext context)
{
    const QUrl currentUrl = context.currentUrl();
    finishRefresh(std::move(context));
    return replaceState(pageNavigationStateForTargets(std::move(targets), currentUrl));
}

void ImagePageNavigationModel::finishRefresh(ImageCandidateListContext context)
{
    m_knownRefreshContext = std::move(context);
}

bool ImagePageNavigationModel::clear()
{
    m_pendingRefresh.cancel();
    m_knownRefreshContext = std::nullopt;
    m_pendingRefreshContext = std::nullopt;
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
