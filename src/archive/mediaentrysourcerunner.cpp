// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourcerunner.h"

#include "archivebackend_p.h"

#include <utility>
#include <variant>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;

KiriView::MediaEntrySourceFactory defaultSourceFactory(
    KiriView::MediaEntrySourceFactory sourceFactory)
{
    return sourceFactory ? std::move(sourceFactory) : KiriView::openMediaEntrySource;
}
}

namespace KiriView {
MediaEntrySourceRunner::MediaEntrySourceRunner(
    OpenedCollectionScopeLocation archiveCollection, MediaEntrySourceFactory sourceFactory)
    : m_archiveCollection(std::move(archiveCollection))
    , m_sourceFactory(defaultSourceFactory(std::move(sourceFactory)))
{
}

const OpenedCollectionScopeLocation &MediaEntrySourceRunner::openedCollectionScope() const
{
    return m_archiveCollection;
}

ArchiveImageCandidatesResult MediaEntrySourceRunner::loadImageCandidates()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_cachedCandidates.has_value()) {
        return ArchiveImageCandidates { *m_cachedCandidates };
    }

    const std::optional<QString> errorString = ensureSource();
    if (errorString.has_value()) {
        return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(*errorString);
    }

    ArchiveImageCandidatesResult result = m_source->loadImageCandidates();
    if (const auto *candidates = std::get_if<ArchiveImageCandidates>(&result)) {
        m_cachedCandidates = candidates->candidates;
    }
    return result;
}

ArchiveImageDataResult MediaEntrySourceRunner::loadImageData(const QUrl &imageUrl)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const std::optional<QString> errorString = ensureSource();
    if (errorString.has_value()) {
        return Backend::archiveErrorResult<ArchiveImageDataResult>(*errorString);
    }

    return m_source->loadImageData(imageUrl);
}

std::optional<std::vector<ImageNavigationCandidate>> MediaEntrySourceRunner::cachedImageCandidates()
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
    MediaEntrySourceOpenResult result = m_sourceFactory(m_archiveCollection);
    if (const auto *error = std::get_if<ArchiveError>(&result)) {
        m_openErrorString = error->errorString;
        return m_openErrorString;
    }

    const auto *source = std::get_if<MediaEntrySourcePtr>(&result);
    if (source == nullptr || *source == nullptr) {
        m_openErrorString = Backend::fallbackArchiveOpenError(m_archiveCollection);
        return m_openErrorString;
    }

    m_source = *source;
    return std::nullopt;
}
}
