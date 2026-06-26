// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCEBACKEND_H
#define KIRIVIEW_MEDIAENTRYSOURCEBACKEND_H

#include "location/imagelocation.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

namespace kiriview {
enum class MediaEntrySourceBackendKind {
    Unknown,
    Unsupported,
    Directory,
    KArchive,
    LibArchive,
};

enum class MediaEntrySourceOperation {
    OpenCollection,
    ListCandidates,
    ReadImageData,
    LoadThumbnailMetadata,
};

struct MediaEntrySourceError
{
    MediaEntrySourceBackendKind backend = MediaEntrySourceBackendKind::Unknown;
    MediaEntrySourceOperation operation = MediaEntrySourceOperation::OpenCollection;
    QUrl collectionUrl;
    QString entryPath;
    QString errorString;
    QString diagnosticDetail;
};

struct MediaEntrySourceCandidates
{
    std::vector<ImageDocumentPageCandidate> candidates;
};

struct MediaEntrySourceImageData
{
    QByteArray data;
};

enum class MediaEntryContentChecksumAlgorithm {
    Crc32,
};

struct MediaEntryContentChecksum
{
    MediaEntryContentChecksumAlgorithm algorithm = MediaEntryContentChecksumAlgorithm::Crc32;
    quint64 value = 0;
};

struct MediaEntrySourceThumbnailMetadata
{
    MediaEntryContentChecksum checksum;
    qint64 uncompressedSize = -1;
};

template <typename Value> using MediaEntrySourceResult = std::variant<Value, MediaEntrySourceError>;
using MediaEntrySourceCandidatesResult = MediaEntrySourceResult<MediaEntrySourceCandidates>;
using MediaEntrySourceImageDataResult = MediaEntrySourceResult<MediaEntrySourceImageData>;
using MediaEntrySourceThumbnailMetadataResult
    = MediaEntrySourceResult<MediaEntrySourceThumbnailMetadata>;

class MediaEntrySource
{
public:
    MediaEntrySource() = default;

public:
    virtual ~MediaEntrySource() = default;

    virtual MediaEntrySourceCandidatesResult loadImageDocumentPageCandidates() = 0;
    virtual MediaEntrySourceImageDataResult loadImageData(const QUrl& imageUrl) = 0;
    virtual MediaEntrySourceThumbnailMetadataResult loadThumbnailMetadata(const QUrl& imageUrl);
    Q_DISABLE_COPY(MediaEntrySource)
};

using MediaEntrySourcePtr = std::shared_ptr<MediaEntrySource>;
using MediaEntrySourceOpenResult = MediaEntrySourceResult<MediaEntrySourcePtr>;
using MediaEntrySourceFactory
    = std::function<MediaEntrySourceOpenResult(const OpenedCollectionScopeLocation&)>;

MediaEntrySourceCandidatesResult loadMediaEntrySourceCandidates(
    const OpenedCollectionScopeLocation& openedCollectionScope);
MediaEntrySourceImageDataResult loadMediaEntrySourceImageData(
    const OpenedCollectionScopeLocation& openedCollectionScope, const QUrl& imageUrl);
MediaEntrySourceThumbnailMetadataResult loadMediaEntrySourceThumbnailMetadata(
    const OpenedCollectionScopeLocation& openedCollectionScope, const QUrl& imageUrl);
MediaEntrySourceOpenResult openMediaEntrySource(
    const OpenedCollectionScopeLocation& openedCollectionScope);
}

#endif
