// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepagenavigationrefreshstate.h"

#include <utility>

namespace KiriView {
ImagePageNavigationRefreshPreview ImagePageNavigationRefreshState::beginRefresh(
    const ImageCandidateListContext &context, bool knownUrlsAvailable)
{
    const quint64 refreshId = m_pendingRefresh.start();
    const bool keepKnownUrls = m_knownContext.has_value() && knownUrlsAvailable
        && sameImageCandidateListSource(m_knownContext->source(), context.source());
    if (!keepKnownUrls) {
        m_knownContext = std::nullopt;
    }

    m_refreshContext = context;
    return ImagePageNavigationRefreshPreview { keepKnownUrls, refreshId };
}

bool ImagePageNavigationRefreshState::shouldKeepExistingWatcherFor(
    const ImageCandidateListContext &context) const
{
    return m_knownContext.has_value()
        && sameImageCandidateListSource(m_knownContext->source(), context.source());
}

std::optional<ImageCandidateListContext>
ImagePageNavigationRefreshState::acceptedPendingRefreshContext(
    quint64 refreshId, ImageCandidateListSource source) const
{
    if (!m_pendingRefresh.accepts(refreshId)) {
        return std::nullopt;
    }

    return acceptedWatchedRefreshContext(std::move(source));
}

std::optional<ImageCandidateListContext>
ImagePageNavigationRefreshState::acceptedWatchedRefreshContext(
    ImageCandidateListSource source) const
{
    if (!m_refreshContext.has_value()
        || !sameImageCandidateListSource(m_refreshContext->source(), source)) {
        return std::nullopt;
    }

    return ImageCandidateListContext::forSource(m_refreshContext->currentUrl(), std::move(source));
}

bool ImagePageNavigationRefreshState::finishPendingRefresh(quint64 refreshId)
{
    return m_pendingRefresh.finish(refreshId);
}

void ImagePageNavigationRefreshState::finishRefresh(ImageCandidateListContext context)
{
    m_knownContext = std::move(context);
}

void ImagePageNavigationRefreshState::clear()
{
    m_pendingRefresh.cancel();
    m_knownContext = std::nullopt;
    m_refreshContext = std::nullopt;
}
}
