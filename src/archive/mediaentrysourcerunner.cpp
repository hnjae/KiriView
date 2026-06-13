// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourcerunner.h"

#include "mediaentrysourcebackend_p.h"

#include <utility>
#include <variant>

namespace {
namespace Backend = kiriview::MediaEntrySourceBackendDetail;

kiriview::MediaEntrySourceFactory defaultSourceFactory(
    kiriview::MediaEntrySourceFactory sourceFactory)
{
    return sourceFactory ? std::move(sourceFactory) : kiriview::openMediaEntrySource;
}
}

namespace kiriview {
MediaEntrySourceRunner::MediaEntrySourceRunner(
    OpenedCollectionScopeLocation openedCollectionScope, MediaEntrySourceFactory sourceFactory)
    : m_openedCollectionScope(std::move(openedCollectionScope))
    , m_sourceFactory(defaultSourceFactory(std::move(sourceFactory)))
{
}

const OpenedCollectionScopeLocation &MediaEntrySourceRunner::openedCollectionScope() const
{
    return m_openedCollectionScope;
}

MediaEntrySourceCandidatesResult MediaEntrySourceRunner::loadImageDocumentPageCandidates()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_cachedCandidates.has_value()) {
        return MediaEntrySourceCandidates { *m_cachedCandidates };
    }

    const std::optional<QString> errorString = ensureSource();
    if (errorString.has_value()) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceCandidatesResult>(*errorString);
    }

    MediaEntrySourceCandidatesResult result = m_source->loadImageDocumentPageCandidates();
    if (const auto *candidates = std::get_if<MediaEntrySourceCandidates>(&result)) {
        m_cachedCandidates = candidates->candidates;
    }
    return result;
}

MediaEntrySourceImageDataResult MediaEntrySourceRunner::loadImageData(const QUrl &imageUrl)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const std::optional<QString> errorString = ensureSource();
    if (errorString.has_value()) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceImageDataResult>(*errorString);
    }

    return m_source->loadImageData(imageUrl);
}

std::optional<std::vector<ImageDocumentPageCandidate>>
MediaEntrySourceRunner::cachedImageDocumentPageCandidates()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cachedCandidates;
}

std::optional<QString> MediaEntrySourceRunner::ensureSource()
{
    if (m_source != nullptr) {
        return std::nullopt;
    }
    if (m_openAttempted) {
        return m_openErrorString;
    }

    m_openAttempted = true;
    MediaEntrySourceOpenResult result = m_sourceFactory(m_openedCollectionScope);
    if (const auto *error = std::get_if<MediaEntrySourceError>(&result)) {
        m_openErrorString = error->errorString;
        return m_openErrorString;
    }

    const auto *source = std::get_if<MediaEntrySourcePtr>(&result);
    if (source == nullptr || *source == nullptr) {
        m_openErrorString = Backend::fallbackMediaEntrySourceOpenError(m_openedCollectionScope);
        return m_openErrorString;
    }

    m_source = *source;
    return std::nullopt;
}
}
