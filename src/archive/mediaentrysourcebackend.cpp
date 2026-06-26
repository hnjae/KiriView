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
namespace Backend = kiriview::MediaEntrySourceBackendDetail;

const Backend::MediaEntrySourceBackendOperations*
mediaEntrySourceBackendOperationsForOpenedCollection(
    const kiriview::OpenedCollectionScopeLocation& openedCollectionScope)
{
    if (openedCollectionScope.isDirectory()) {
        return Backend::directoryCollectionMediaEntrySourceBackendOperations();
    }

    switch (
        kiriview::archiveStorageBackendForRootScheme(openedCollectionScope.rootUrl().scheme())) {
    case kiriview::ArchiveStorageBackend::KZip:
    case kiriview::ArchiveStorageBackend::KTar:
    case kiriview::ArchiveStorageBackend::K7Zip:
        return Backend::kArchiveMediaEntrySourceBackendOperations();
    case kiriview::ArchiveStorageBackend::LibArchive:
        return Backend::libArchiveMediaEntrySourceBackendOperations();
    case kiriview::ArchiveStorageBackend::None:
        return nullptr;
    }

    return nullptr;
}

kiriview::MediaEntrySourceOpenResult openWithMediaEntrySourceBackend(
    const kiriview::OpenedCollectionScopeLocation& openedCollectionScope)
{
    const Backend::MediaEntrySourceBackendOperations* backend
        = mediaEntrySourceBackendOperationsForOpenedCollection(openedCollectionScope);
    if (backend == nullptr) {
        return Backend::mediaEntrySourceErrorResult<kiriview::MediaEntrySourceOpenResult>(
            Backend::mediaEntrySourceError(kiriview::MediaEntrySourceBackendKind::Unsupported,
                kiriview::MediaEntrySourceOperation::OpenCollection, openedCollectionScope,
                Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope)));
    }

    return backend->openSource(openedCollectionScope);
}
}

namespace kiriview {
MediaEntrySourceThumbnailMetadataResult MediaEntrySource::loadThumbnailMetadata(
    const QUrl& imageUrl)
{
    Q_UNUSED(imageUrl)
    return MediaEntrySourceBackendDetail::mediaEntrySourceErrorResult<
        MediaEntrySourceThumbnailMetadataResult>(
        MediaEntrySourceBackendDetail::mediaEntrySourceError(MediaEntrySourceBackendKind::Unknown,
            MediaEntrySourceOperation::LoadThumbnailMetadata, {},
            MediaEntrySourceBackendDetail::openedCollectionThumbnailMetadataUnsupportedError()));
}
}

namespace kiriview::MediaEntrySourceBackendDetail {
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
    const OpenedCollectionScopeLocation& openedCollectionScope, const QString& entryPath)
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
    const OpenedCollectionScopeLocation& openedCollectionScope, const QUrl& imageUrl)
{
    const QString entryPath = openedCollectionEntryPathForUrl(openedCollectionScope, imageUrl);
    if (openedCollectionScope.isEmpty() || entryPath.isEmpty()) {
        return std::nullopt;
    }

    return entryPath;
}

QString fallbackMediaEntrySourceOpenError(
    const OpenedCollectionScopeLocation& openedCollectionScope)
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

QString openedCollectionThumbnailMetadataUnsupportedError()
{
    return i18nc("@info:status", "Could not cache a preview thumbnail for this collection item.");
}

MediaEntrySourceError mediaEntrySourceError(MediaEntrySourceBackendKind backend,
    MediaEntrySourceOperation operation, const OpenedCollectionScopeLocation& openedCollectionScope,
    QString errorString, QString diagnosticDetail, QString entryPath)
{
    const QString resolvedDiagnosticDetail
        = diagnosticDetail.isEmpty() ? errorString : std::move(diagnosticDetail);
    return MediaEntrySourceError { backend, operation, openedCollectionScope.fileUrl(),
        std::move(entryPath), std::move(errorString), resolvedDiagnosticDetail };
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

MediaEntrySourceThumbnailMetadataResult mediaEntrySourceThumbnailMetadataResult(
    MediaEntrySourceThumbnailMetadata metadata)
{
    return metadata;
}
}

namespace kiriview {
MediaEntrySourceCandidatesResult loadMediaEntrySourceCandidates(
    const OpenedCollectionScopeLocation& openedCollectionScope)
{
    MediaEntrySourceOpenResult opened = openMediaEntrySource(openedCollectionScope);
    if (const auto* error = std::get_if<MediaEntrySourceError>(&opened)) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceCandidatesResult>(*error);
    }

    const auto* source = std::get_if<MediaEntrySourcePtr>(&opened);
    if (source == nullptr || *source == nullptr) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceCandidatesResult>(
            Backend::mediaEntrySourceError(MediaEntrySourceBackendKind::Unknown,
                MediaEntrySourceOperation::OpenCollection, openedCollectionScope,
                Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope)));
    }

    return (*source)->loadImageDocumentPageCandidates();
}

MediaEntrySourceImageDataResult loadMediaEntrySourceImageData(
    const OpenedCollectionScopeLocation& openedCollectionScope, const QUrl& imageUrl)
{
    MediaEntrySourceOpenResult opened = openMediaEntrySource(openedCollectionScope);
    if (const auto* error = std::get_if<MediaEntrySourceError>(&opened)) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceImageDataResult>(*error);
    }

    const auto* source = std::get_if<MediaEntrySourcePtr>(&opened);
    if (source == nullptr || *source == nullptr) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceImageDataResult>(
            Backend::mediaEntrySourceError(MediaEntrySourceBackendKind::Unknown,
                MediaEntrySourceOperation::OpenCollection, openedCollectionScope,
                Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope)));
    }

    return (*source)->loadImageData(imageUrl);
}

MediaEntrySourceThumbnailMetadataResult loadMediaEntrySourceThumbnailMetadata(
    const OpenedCollectionScopeLocation& openedCollectionScope, const QUrl& imageUrl)
{
    MediaEntrySourceOpenResult opened = openMediaEntrySource(openedCollectionScope);
    if (const auto* error = std::get_if<MediaEntrySourceError>(&opened)) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceThumbnailMetadataResult>(
            *error);
    }

    const auto* source = std::get_if<MediaEntrySourcePtr>(&opened);
    if (source == nullptr || *source == nullptr) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceThumbnailMetadataResult>(
            Backend::mediaEntrySourceError(MediaEntrySourceBackendKind::Unknown,
                MediaEntrySourceOperation::OpenCollection, openedCollectionScope,
                Backend::fallbackMediaEntrySourceOpenError(openedCollectionScope)));
    }

    return (*source)->loadThumbnailMetadata(imageUrl);
}

MediaEntrySourceOpenResult openMediaEntrySource(
    const OpenedCollectionScopeLocation& openedCollectionScope)
{
    if (openedCollectionScope.isEmpty()) {
        return Backend::mediaEntrySourceErrorResult<MediaEntrySourceOpenResult>(
            Backend::mediaEntrySourceError(MediaEntrySourceBackendKind::Unknown,
                MediaEntrySourceOperation::OpenCollection, openedCollectionScope,
                i18nc("@info:status", "Could not open the selected collection.")));
    }

    return openWithMediaEntrySourceBackend(openedCollectionScope);
}
}
