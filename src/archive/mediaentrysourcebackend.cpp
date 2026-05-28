// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourcebackend.h"

#include "archiveformat.h"
#include "archivepath.h"
#include "decoding/imageformatregistry.h"
#include "mediaentrysourcebackend_p.h"
#include "navigation/imagedocumentpagenavigationpolicy.h"
#include "navigation/mediaformatregistry.h"

#include <KLocalizedString>
#include <optional>
#include <utility>
#include <variant>

namespace {
namespace Backend = KiriView::MediaEntrySourceBackendDetail;

const Backend::MediaEntrySourceBackendOperations *
mediaEntrySourceBackendOperationsForOpenedCollection(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope)
{
    if (openedCollectionScope.isDirectory()) {
        return Backend::directoryCollectionMediaEntrySourceBackendOperations();
    }

    switch (
        KiriView::archiveStorageBackendForRootScheme(openedCollectionScope.rootUrl().scheme())) {
    case KiriView::ArchiveStorageBackend::KZip:
    case KiriView::ArchiveStorageBackend::KTar:
    case KiriView::ArchiveStorageBackend::K7Zip:
        return Backend::kArchiveMediaEntrySourceBackendOperations();
    case KiriView::ArchiveStorageBackend::LibArchive:
        return Backend::libArchiveMediaEntrySourceBackendOperations();
    case KiriView::ArchiveStorageBackend::None:
        return nullptr;
    }

    return nullptr;
}

KiriView::MediaEntrySourceOpenResult openWithMediaEntrySourceBackend(
    const KiriView::OpenedCollectionScopeLocation &openedCollectionScope)
{
    const Backend::MediaEntrySourceBackendOperations *backend
        = mediaEntrySourceBackendOperationsForOpenedCollection(openedCollectionScope);
    if (backend == nullptr) {
        return Backend::mediaEntrySourceErrorResult<KiriView::MediaEntrySourceOpenResult>(
            Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope));
    }

    return backend->openSource(openedCollectionScope);
}
}

namespace KiriView::MediaEntrySourceBackendDetail {
MediaEntrySourceWithCandidateSnapshot::MediaEntrySourceWithCandidateSnapshot(
    std::vector<ImageDocumentPageCandidate> candidates)
{
    replaceCandidateSnapshot(std::move(candidates));
}

MediaEntrySourceCandidatesResult
MediaEntrySourceWithCandidateSnapshot::loadImageDocumentPageCandidates()
{
    return MediaEntrySourceCandidates { m_candidates };
}

void MediaEntrySourceWithCandidateSnapshot::replaceCandidateSnapshot(
    std::vector<ImageDocumentPageCandidate> candidates)
{
    sortImageDocumentPageCandidates(&candidates);
    m_candidates = std::move(candidates);
}

std::optional<ImageDocumentPageCandidate> openedCollectionImageDocumentPageCandidate(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QString &entryPath)
{
    const QString candidateName = normalizedArchiveEntryPath(entryPath);
    if (candidateName.isEmpty() || !isSupportedOrdinaryMediaFileName(candidateName)) {
        return std::nullopt;
    }

    const QUrl url = openedCollectionEntryUrl(openedCollectionScope, candidateName);
    if (url.isEmpty()) {
        return std::nullopt;
    }

    return ImageDocumentPageCandidate { url, candidateName,
        isSupportedDirectVideoFileName(candidateName) ? ImageDocumentPageKind::Video
                                                      : ImageDocumentPageKind::Image };
}

std::optional<QString> openedCollectionImageEntryPathForRead(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &imageUrl)
{
    const QString entryPath = openedCollectionEntryPathForUrl(openedCollectionScope, imageUrl);
    if (openedCollectionScope.isEmpty() || entryPath.isEmpty()) {
        return std::nullopt;
    }

    return entryPath;
}

QString fallbackMediaEntrySourceOpenError(
    const OpenedCollectionScopeLocation &openedCollectionScope)
{
    const QString fileName = openedCollectionScope.fileUrl().fileName();
    if (fileName.isEmpty()) {
        return i18nc("@info:status", "Could not open the selected collection.");
    }

    return ki18nc("@info:status", "Could not open %1.").subs(fileName).toString();
}

QString openedCollectionImageNotFoundError()
{
    return i18nc("@info:status", "Could not find the selected image in the collection.");
}

QString openedCollectionImageReadError()
{
    return i18nc("@info:status", "Could not read the selected collection image.");
}

MediaEntrySourceCandidatesResult mediaEntrySourceCandidatesResult(
    std::vector<ImageDocumentPageCandidate> candidates)
{
    sortImageDocumentPageCandidates(&candidates);
    return MediaEntrySourceCandidates { std::move(candidates) };
}

MediaEntrySourceImageDataResult mediaEntrySourceImageDataResult(QByteArray data)
{
    return MediaEntrySourceImageData { std::move(data) };
}
}

namespace KiriView {
MediaEntrySourceCandidatesResult loadMediaEntrySourceCandidates(
    const OpenedCollectionScopeLocation &openedCollectionScope)
{
    MediaEntrySourceOpenResult opened = openMediaEntrySource(openedCollectionScope);
    if (const auto *error = std::get_if<MediaEntrySourceError>(&opened)) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceCandidatesResult>(
            error->errorString);
    }

    const auto *source = std::get_if<MediaEntrySourcePtr>(&opened);
    if (source == nullptr || *source == nullptr) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceCandidatesResult>(
            Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope));
    }

    return (*source)->loadImageDocumentPageCandidates();
}

MediaEntrySourceImageDataResult loadMediaEntrySourceImageData(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &imageUrl)
{
    MediaEntrySourceOpenResult opened = openMediaEntrySource(openedCollectionScope);
    if (const auto *error = std::get_if<MediaEntrySourceError>(&opened)) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceImageDataResult>(
            error->errorString);
    }

    const auto *source = std::get_if<MediaEntrySourcePtr>(&opened);
    if (source == nullptr || *source == nullptr) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceImageDataResult>(
            Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope));
    }

    return (*source)->loadImageData(imageUrl);
}

MediaEntrySourceOpenResult openMediaEntrySource(
    const OpenedCollectionScopeLocation &openedCollectionScope)
{
    if (openedCollectionScope.isEmpty()) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceOpenResult>(
            i18nc("@info:status", "Could not open the selected collection."));
    }

    return openWithMediaEntrySourceBackend(openedCollectionScope);
}
}
