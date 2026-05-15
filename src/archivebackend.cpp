// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend.h"

#include "archivebackend_p.h"
#include "archiveformat.h"
#include "archivepath.h"
#include "imageformatregistry.h"
#include "imagenavigationmodel.h"

#include <KLocalizedString>
#include <optional>
#include <utility>

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

template <typename Result, typename Load>
Result loadWithArchiveBackend(const KiriView::ArchiveDocumentLocation &archiveDocument, Load load)
{
    const Backend::ArchiveBackendOperations *backend
        = archiveBackendOperationsForDocument(archiveDocument);
    if (backend == nullptr) {
        return Backend::archiveErrorResult<Result>(
            Backend::fallbackArchiveOpenError(archiveDocument));
    }

    return load(*backend);
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
    if (archiveDocument.isEmpty()) {
        return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(
            i18nc("@info:status", "Could not open the selected archive."));
    }

    return loadWithArchiveBackend<ArchiveImageCandidatesResult>(
        archiveDocument, [&archiveDocument](const Backend::ArchiveBackendOperations &backend) {
            return backend.loadImageCandidates(archiveDocument);
        });
}

ArchiveImageDataResult loadArchiveDocumentImageData(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    const std::optional<QString> entryPath
        = Backend::archiveImageEntryPathForRead(archiveDocument, imageUrl);
    if (!entryPath.has_value()) {
        return Backend::archiveErrorResult<ArchiveImageDataResult>(
            Backend::archiveImageNotFoundError());
    }

    return loadWithArchiveBackend<ArchiveImageDataResult>(archiveDocument,
        [&archiveDocument, &entryPath](const Backend::ArchiveBackendOperations &backend) {
            return backend.loadImageData(archiveDocument, *entryPath);
        });
}

ArchiveDocumentSessionOpenResult openArchiveDocumentSession(
    const ArchiveDocumentLocation &archiveDocument)
{
    if (archiveDocument.isEmpty()) {
        return Backend::archiveErrorResult<ArchiveDocumentSessionOpenResult>(
            i18nc("@info:status", "Could not open the selected archive."));
    }

    return loadWithArchiveBackend<ArchiveDocumentSessionOpenResult>(
        archiveDocument, [&archiveDocument](const Backend::ArchiveBackendOperations &backend) {
            return backend.openSession(archiveDocument);
        });
}
}
