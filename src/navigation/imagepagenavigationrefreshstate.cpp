// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepagenavigationrefreshstate.h"

#include <utility>

namespace KiriView {
ImagePageNavigationRefreshPreview ImagePageNavigationRefreshState::beginRefresh(
    const ImageCandidateListContext &context, bool knownUrlsAvailable)
{
    const bool keepKnownUrls = m_knownContext.has_value() && knownUrlsAvailable
        && sameImageCandidateListSource(m_knownContext->source(), context.source());
    if (!keepKnownUrls) {
        m_knownContext = std::nullopt;
    }

    m_refreshContext = context;
    return ImagePageNavigationRefreshPreview { keepKnownUrls };
}

bool ImagePageNavigationRefreshState::shouldKeepExistingWatcherFor(
    const ImageCandidateListContext &context) const
{
    return m_knownContext.has_value()
        && sameImageCandidateListSource(m_knownContext->source(), context.source());
}

std::optional<ImageCandidateListContext>
ImagePageNavigationRefreshState::acceptedPendingRefreshContext(
    ImageCandidateListSource source) const
{
    if (!m_refreshContext.has_value()
        || !sameImageCandidateListSource(m_refreshContext->source(), source)) {
        return std::nullopt;
    }

    return ImageCandidateListContext::forSource(m_refreshContext->currentUrl(), std::move(source));
}

void ImagePageNavigationRefreshState::finishRefresh(ImageCandidateListContext context)
{
    m_knownContext = std::move(context);
}

void ImagePageNavigationRefreshState::clear()
{
    m_knownContext = std::nullopt;
    m_refreshContext = std::nullopt;
}
}
