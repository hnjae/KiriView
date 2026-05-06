// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend.h"

#include "archivebackend_p.h"
#include "archiveformat.h"
#include "archivepath.h"
#include "imageformatregistry.h"

#include <optional>
#include <utility>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;

std::optional<QString> archiveImageEntryPathForRead(
    const KiriView::ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    const QString entryPath = KiriView::archiveEntryPathForUrl(archiveDocument, imageUrl);
    if (archiveDocument.isEmpty() || entryPath.isEmpty()) {
        return std::nullopt;
    }

    return entryPath;
}

const Backend::ArchiveBackendOperations *archiveBackendOperationsForDocument(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
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
}

namespace KiriView::ArchiveBackendDetail {
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

QString fallbackArchiveOpenError(const ArchiveDocumentLocation &archiveDocument)
{
    const QString fileName = archiveDocument.fileUrl().fileName();
    if (fileName.isEmpty()) {
        return QStringLiteral("Could not open the selected archive.");
    }

    return QStringLiteral("Could not open %1.").arg(fileName);
}

QString archiveImageNotFoundError()
{
    return QStringLiteral("Could not find the selected image in the archive.");
}

QString archiveImageReadError()
{
    return QStringLiteral("Could not read the selected archive image.");
}

ArchiveImageCandidatesResult archiveImageCandidatesResult(
    std::vector<ImageNavigationCandidate> candidates)
{
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
    if (archiveDocument.isEmpty()) {
        return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(
            QStringLiteral("Could not open the selected archive."));
    }

    const Backend::ArchiveBackendOperations *backend
        = archiveBackendOperationsForDocument(archiveDocument);
    if (backend == nullptr) {
        return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(
            Backend::fallbackArchiveOpenError(archiveDocument));
    }

    return backend->loadImageCandidates(archiveDocument);
}

ArchiveImageDataResult loadArchiveDocumentImageData(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    const std::optional<QString> entryPath
        = archiveImageEntryPathForRead(archiveDocument, imageUrl);
    if (!entryPath.has_value()) {
        return Backend::archiveErrorResult<ArchiveImageDataResult>(
            Backend::archiveImageNotFoundError());
    }

    const Backend::ArchiveBackendOperations *backend
        = archiveBackendOperationsForDocument(archiveDocument);
    if (backend == nullptr) {
        return Backend::archiveErrorResult<ArchiveImageDataResult>(
            Backend::fallbackArchiveOpenError(archiveDocument));
    }

    return backend->loadImageData(archiveDocument, *entryPath);
}
}
