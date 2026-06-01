// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCEBACKEND_P_H
#define KIRIVIEW_MEDIAENTRYSOURCEBACKEND_P_H

#include "location/imagelocation.h"
#include "mediaentrysourcebackend.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QByteArray>
#include <QString>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView::MediaEntrySourceBackendDetail {
using MediaEntrySourceOpener
    = MediaEntrySourceOpenResult (*)(const OpenedCollectionScopeLocation &);

struct MediaEntrySourceBackendOperations {
    MediaEntrySourceOpener openSource;
};

class MediaEntrySourceWithCandidateSnapshot : public MediaEntrySource
{
public:
    explicit MediaEntrySourceWithCandidateSnapshot(
        std::vector<ImageDocumentPageCandidate> candidates);

    MediaEntrySourceCandidatesResult loadImageDocumentPageCandidates() final;

protected:
    void replaceCandidateSnapshot(std::vector<ImageDocumentPageCandidate> candidates);

private:
    std::vector<ImageDocumentPageCandidate> m_candidates;
};

std::optional<ImageDocumentPageCandidate> openedCollectionImageDocumentPageCandidate(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QString &entryPath);
std::optional<QString> openedCollectionImageEntryPathForRead(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &imageUrl);
QString fallbackMediaEntrySourceOpenError(
    const OpenedCollectionScopeLocation &openedCollectionScope);
QString openedCollectionImageNotFoundError();
QString openedCollectionImageReadError();
QString openedCollectionThumbnailMetadataUnsupportedError();

template <typename Result> Result mediaEntrySourceErrorResult(QString errorString)
{
    return Result { MediaEntrySourceError { std::move(errorString) } };
}

MediaEntrySourceCandidatesResult mediaEntrySourceCandidatesResult(
    std::vector<ImageDocumentPageCandidate> candidates);
MediaEntrySourceImageDataResult mediaEntrySourceImageDataResult(QByteArray data);
MediaEntrySourceThumbnailMetadataResult mediaEntrySourceThumbnailMetadataResult(
    MediaEntrySourceThumbnailMetadata metadata);

const MediaEntrySourceBackendOperations *kArchiveMediaEntrySourceBackendOperations();
const MediaEntrySourceBackendOperations *libArchiveMediaEntrySourceBackendOperations();
const MediaEntrySourceBackendOperations *directoryCollectionMediaEntrySourceBackendOperations();
}

#endif
