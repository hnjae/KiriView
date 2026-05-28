// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCEBACKEND_P_H
#define KIRIVIEW_MEDIAENTRYSOURCEBACKEND_P_H

#include "location/imagelocation.h"
#include "mediaentrysourcebackend.h"
#include "navigation/imagenavigationtypes.h"

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
        std::vector<ImageNavigationCandidate> candidates);

    MediaEntrySourceCandidatesResult loadImageCandidates() final;

protected:
    void replaceCandidateSnapshot(std::vector<ImageNavigationCandidate> candidates);

private:
    std::vector<ImageNavigationCandidate> m_candidates;
};

std::optional<ImageNavigationCandidate> openedCollectionMediaCandidate(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QString &entryPath);
std::optional<QString> openedCollectionImageEntryPathForRead(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &imageUrl);
QString fallbackMediaEntrySourceOpenError(
    const OpenedCollectionScopeLocation &openedCollectionScope);
QString openedCollectionImageNotFoundError();
QString openedCollectionImageReadError();

template <typename Result> Result mediaEntrySourceErrorResult(QString errorString)
{
    return Result { MediaEntrySourceError { std::move(errorString) } };
}

MediaEntrySourceCandidatesResult mediaEntrySourceCandidatesResult(
    std::vector<ImageNavigationCandidate> candidates);
MediaEntrySourceImageDataResult mediaEntrySourceImageDataResult(QByteArray data);

const MediaEntrySourceBackendOperations *kArchiveMediaEntrySourceBackendOperations();
const MediaEntrySourceBackendOperations *libArchiveMediaEntrySourceBackendOperations();
const MediaEntrySourceBackendOperations *directoryMediaEntrySourceBackendOperations();
}

#endif
