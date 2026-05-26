// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivedocumentsessionrunner.h"

#include "archivebackend_p.h"

#include <utility>
#include <variant>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;

KiriView::ArchiveDocumentSessionFactory defaultSessionFactory(
    KiriView::ArchiveDocumentSessionFactory sessionFactory)
{
    return sessionFactory ? std::move(sessionFactory) : KiriView::openArchiveDocumentSession;
}
}

namespace KiriView {
ArchiveDocumentSessionRunner::ArchiveDocumentSessionRunner(
    ImagePageScopeLocation archiveDocument, ArchiveDocumentSessionFactory sessionFactory)
    : m_archiveDocument(std::move(archiveDocument))
    , m_sessionFactory(defaultSessionFactory(std::move(sessionFactory)))
{
}

const ImagePageScopeLocation &ArchiveDocumentSessionRunner::imagePageScope() const
{
    return m_archiveDocument;
}

ArchiveImageCandidatesResult ArchiveDocumentSessionRunner::loadImageCandidates()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_cachedCandidates.has_value()) {
        return ArchiveImageCandidates { *m_cachedCandidates };
    }

    const std::optional<QString> errorString = ensureSession();
    if (errorString.has_value()) {
        return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(*errorString);
    }

    ArchiveImageCandidatesResult result = m_session->loadImageCandidates();
    if (const auto *candidates = std::get_if<ArchiveImageCandidates>(&result)) {
        m_cachedCandidates = candidates->candidates;
    }
    return result;
}

ArchiveImageDataResult ArchiveDocumentSessionRunner::loadImageData(const QUrl &imageUrl)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const std::optional<QString> errorString = ensureSession();
    if (errorString.has_value()) {
        return Backend::archiveErrorResult<ArchiveImageDataResult>(*errorString);
    }

    return m_session->loadImageData(imageUrl);
}

std::optional<std::vector<ImageNavigationCandidate>>
ArchiveDocumentSessionRunner::cachedImageCandidates()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cachedCandidates;
}

std::optional<QString> ArchiveDocumentSessionRunner::ensureSession()
{
    if (m_session != nullptr) {
        return std::nullopt;
    }
    if (m_openAttempted) {
        return m_openErrorString;
    }

    m_openAttempted = true;
    ArchiveDocumentSessionOpenResult result = m_sessionFactory(m_archiveDocument);
    if (const auto *error = std::get_if<ArchiveError>(&result)) {
        m_openErrorString = error->errorString;
        return m_openErrorString;
    }

    const auto *session = std::get_if<ArchiveDocumentSessionPtr>(&result);
    if (session == nullptr || *session == nullptr) {
        m_openErrorString = Backend::fallbackArchiveOpenError(m_archiveDocument);
        return m_openErrorString;
    }

    m_session = *session;
    return std::nullopt;
}
}
