// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend.h"

#include "archivebackend_p.h"
#include "archiveformat.h"
#include "archivepath.h"
#include "decoding/imageformatregistry.h"
#include "navigation/imagenavigationmodel.h"

#include <KLocalizedString>
#include <optional>
#include <utility>
#include <variant>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;

const Backend::ArchiveBackendOperations *archiveBackendOperationsForDocument(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    if (archiveDocument.isDirectory()) {
        return Backend::directoryBackendOperations();
    }

    switch (KiriView::archiveStorageBackendForRootScheme(archiveDocument.rootUrl().scheme())) {
    case KiriView::ArchiveStorageBackend::KZip:
    case KiriView::ArchiveStorageBackend::KTar:
    case KiriView::ArchiveStorageBackend::K7Zip:
        return Backend::kArchiveBackendOperations();
    case KiriView::ArchiveStorageBackend::LibArchive:
        return Backend::libArchiveBackendOperations();
    case KiriView::ArchiveStorageBackend::None:
        return nullptr;
    }

    return nullptr;
}

KiriView::ArchiveDocumentSessionOpenResult openWithArchiveBackend(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    const Backend::ArchiveBackendOperations *backend
        = archiveBackendOperationsForDocument(archiveDocument);
    if (backend == nullptr) {
        return Backend::archiveErrorResult<KiriView::ArchiveDocumentSessionOpenResult>(
            Backend::fallbackArchiveOpenError(archiveDocument));
    }

    return backend->openSession(archiveDocument);
}
}

namespace KiriView::ArchiveBackendDetail {
ArchiveDocumentSessionWithCandidateSnapshot::ArchiveDocumentSessionWithCandidateSnapshot(
    std::vector<ImageNavigationCandidate> candidates)
{
    replaceCandidateSnapshot(std::move(candidates));
}

ArchiveImageCandidatesResult ArchiveDocumentSessionWithCandidateSnapshot::loadImageCandidates()
{
    return ArchiveImageCandidates { m_candidates };
}

void ArchiveDocumentSessionWithCandidateSnapshot::replaceCandidateSnapshot(
    std::vector<ImageNavigationCandidate> candidates)
{
    sortImageNavigationCandidates(&candidates);
    m_candidates = std::move(candidates);
}

std::optional<ImageNavigationCandidate> archiveImageCandidate(
    const ArchiveDocumentLocation &archiveDocument, const QString &entryPath)
{
    const QString candidateName = normalizedArchiveEntryPath(entryPath);
    if (candidateName.isEmpty() || !isSupportedImageFileName(candidateName)) {
        return std::nullopt;
    }

    const QUrl url = archiveEntryUrl(archiveDocument, candidateName);
    if (url.isEmpty()) {
        return std::nullopt;
    }

    return ImageNavigationCandidate { url, candidateName };
}

std::optional<QString> archiveImageEntryPathForRead(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    const QString entryPath = archiveEntryPathForUrl(archiveDocument, imageUrl);
    if (archiveDocument.isEmpty() || entryPath.isEmpty()) {
        return std::nullopt;
    }

    return entryPath;
}

QString fallbackArchiveOpenError(const ArchiveDocumentLocation &archiveDocument)
{
    const QString fileName = archiveDocument.fileUrl().fileName();
    if (fileName.isEmpty()) {
        return i18nc("@info:status", "Could not open the selected archive.");
    }

    return ki18nc("@info:status", "Could not open %1.").subs(fileName).toString();
}

QString archiveImageNotFoundError()
{
    return i18nc("@info:status", "Could not find the selected image in the archive.");
}

QString archiveImageReadError()
{
    return i18nc("@info:status", "Could not read the selected archive image.");
}

ArchiveImageCandidatesResult archiveImageCandidatesResult(
    std::vector<ImageNavigationCandidate> candidates)
{
    sortImageNavigationCandidates(&candidates);
    return ArchiveImageCandidates { std::move(candidates) };
}

ArchiveImageDataResult archiveImageDataResult(QByteArray data)
{
    return ArchiveImageData { std::move(data) };
}
}

namespace KiriView {
ArchiveImageCandidatesResult loadArchiveDocumentImageCandidates(
    const ArchiveDocumentLocation &archiveDocument)
{
    ArchiveDocumentSessionOpenResult opened = openArchiveDocumentSession(archiveDocument);
    if (const auto *error = std::get_if<ArchiveError>(&opened)) {
        return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(error->errorString);
    }

    const auto *session = std::get_if<ArchiveDocumentSessionPtr>(&opened);
    if (session == nullptr || *session == nullptr) {
        return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(
            Backend::fallbackArchiveOpenError(archiveDocument));
    }

    return (*session)->loadImageCandidates();
}

ArchiveImageDataResult loadArchiveDocumentImageData(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    ArchiveDocumentSessionOpenResult opened = openArchiveDocumentSession(archiveDocument);
    if (const auto *error = std::get_if<ArchiveError>(&opened)) {
        return Backend::archiveErrorResult<ArchiveImageDataResult>(error->errorString);
    }

    const auto *session = std::get_if<ArchiveDocumentSessionPtr>(&opened);
    if (session == nullptr || *session == nullptr) {
        return Backend::archiveErrorResult<ArchiveImageDataResult>(
            Backend::fallbackArchiveOpenError(archiveDocument));
    }

    return (*session)->loadImageData(imageUrl);
}

ArchiveDocumentSessionOpenResult openArchiveDocumentSession(
    const ArchiveDocumentLocation &archiveDocument)
{
    if (archiveDocument.isEmpty()) {
        return Backend::archiveErrorResult<ArchiveDocumentSessionOpenResult>(
            i18nc("@info:status", "Could not open the selected archive."));
    }

    return openWithArchiveBackend(archiveDocument);
}
}
