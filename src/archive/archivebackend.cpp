// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivebackend.h"

#include "archivebackend_p.h"
#include "archiveformat.h"
#include "archivepath.h"
#include "decoding/imageformatregistry.h"
#include "navigation/imagenavigationmodel.h"
#include "navigation/mediaformatregistry.h"

#include <KLocalizedString>
#include <optional>
#include <utility>
#include <variant>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;

const Backend::ArchiveBackendOperations *archiveBackendOperationsForOpenedCollection(
    const KiriView::OpenedCollectionScopeLocation &archiveCollection)
{
    if (archiveCollection.isDirectory()) {
        return Backend::directoryBackendOperations();
    }

    switch (KiriView::archiveStorageBackendForRootScheme(archiveCollection.rootUrl().scheme())) {
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

KiriView::MediaEntrySourceOpenResult openWithArchiveBackend(
    const KiriView::OpenedCollectionScopeLocation &archiveCollection)
{
    const Backend::ArchiveBackendOperations *backend
        = archiveBackendOperationsForOpenedCollection(archiveCollection);
    if (backend == nullptr) {
        return Backend::archiveErrorResult<KiriView::MediaEntrySourceOpenResult>(
            Backend::fallbackArchiveOpenError(archiveCollection));
    }

    return backend->openSource(archiveCollection);
}
}

namespace KiriView::ArchiveBackendDetail {
MediaEntrySourceWithCandidateSnapshot::MediaEntrySourceWithCandidateSnapshot(
    std::vector<ImageNavigationCandidate> candidates)
{
    replaceCandidateSnapshot(std::move(candidates));
}

ArchiveImageCandidatesResult MediaEntrySourceWithCandidateSnapshot::loadImageCandidates()
{
    return ArchiveImageCandidates { m_candidates };
}

void MediaEntrySourceWithCandidateSnapshot::replaceCandidateSnapshot(
    std::vector<ImageNavigationCandidate> candidates)
{
    sortImageNavigationCandidates(&candidates);
    m_candidates = std::move(candidates);
}

std::optional<ImageNavigationCandidate> archiveMediaCandidate(
    const OpenedCollectionScopeLocation &archiveCollection, const QString &entryPath)
{
    const QString candidateName = normalizedArchiveEntryPath(entryPath);
    if (candidateName.isEmpty() || !isSupportedOrdinaryMediaFileName(candidateName)) {
        return std::nullopt;
    }

    const QUrl url = archiveEntryUrl(archiveCollection, candidateName);
    if (url.isEmpty()) {
        return std::nullopt;
    }

    return ImageNavigationCandidate { url, candidateName,
        isSupportedDirectVideoFileName(candidateName) ? ImageNavigationCandidateKind::Video
                                                      : ImageNavigationCandidateKind::Image };
}

std::optional<QString> archiveImageEntryPathForRead(
    const OpenedCollectionScopeLocation &archiveCollection, const QUrl &imageUrl)
{
    const QString entryPath = archiveEntryPathForUrl(archiveCollection, imageUrl);
    if (archiveCollection.isEmpty() || entryPath.isEmpty()) {
        return std::nullopt;
    }

    return entryPath;
}

QString fallbackArchiveOpenError(const OpenedCollectionScopeLocation &archiveCollection)
{
    const QString fileName = archiveCollection.fileUrl().fileName();
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
ArchiveImageCandidatesResult loadMediaEntrySourceCandidates(
    const OpenedCollectionScopeLocation &archiveCollection)
{
    MediaEntrySourceOpenResult opened = openMediaEntrySource(archiveCollection);
    if (const auto *error = std::get_if<ArchiveError>(&opened)) {
        return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(error->errorString);
    }

    const auto *source = std::get_if<MediaEntrySourcePtr>(&opened);
    if (source == nullptr || *source == nullptr) {
        return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(
            Backend::fallbackArchiveOpenError(archiveCollection));
    }

    return (*source)->loadImageCandidates();
}

ArchiveImageDataResult loadMediaEntrySourceImageData(
    const OpenedCollectionScopeLocation &archiveCollection, const QUrl &imageUrl)
{
    MediaEntrySourceOpenResult opened = openMediaEntrySource(archiveCollection);
    if (const auto *error = std::get_if<ArchiveError>(&opened)) {
        return Backend::archiveErrorResult<ArchiveImageDataResult>(error->errorString);
    }

    const auto *source = std::get_if<MediaEntrySourcePtr>(&opened);
    if (source == nullptr || *source == nullptr) {
        return Backend::archiveErrorResult<ArchiveImageDataResult>(
            Backend::fallbackArchiveOpenError(archiveCollection));
    }

    return (*source)->loadImageData(imageUrl);
}

MediaEntrySourceOpenResult openMediaEntrySource(
    const OpenedCollectionScopeLocation &archiveCollection)
{
    if (archiveCollection.isEmpty()) {
        return Backend::archiveErrorResult<MediaEntrySourceOpenResult>(
            i18nc("@info:status", "Could not open the selected archive."));
    }

    return openWithArchiveBackend(archiveCollection);
}
}
