// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEBACKEND_P_H
#define KIRIVIEW_ARCHIVEBACKEND_P_H

#include "archivebackend.h"
#include "location/imagelocation.h"
#include "navigation/imagenavigationtypes.h"

#include <QByteArray>
#include <QString>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView::ArchiveBackendDetail {
using MediaEntrySourceOpener
    = MediaEntrySourceOpenResult (*)(const OpenedCollectionScopeLocation &);

struct ArchiveBackendOperations {
    MediaEntrySourceOpener openSource;
};

class MediaEntrySourceWithCandidateSnapshot : public MediaEntrySource
{
public:
    explicit MediaEntrySourceWithCandidateSnapshot(
        std::vector<ImageNavigationCandidate> candidates);

    ArchiveImageCandidatesResult loadImageCandidates() final;

protected:
    void replaceCandidateSnapshot(std::vector<ImageNavigationCandidate> candidates);

private:
    std::vector<ImageNavigationCandidate> m_candidates;
};

std::optional<ImageNavigationCandidate> archiveMediaCandidate(
    const OpenedCollectionScopeLocation &archiveCollection, const QString &entryPath);
std::optional<QString> archiveImageEntryPathForRead(
    const OpenedCollectionScopeLocation &archiveCollection, const QUrl &imageUrl);
QString fallbackArchiveOpenError(const OpenedCollectionScopeLocation &archiveCollection);
QString archiveImageNotFoundError();
QString archiveImageReadError();

template <typename Result> Result archiveErrorResult(QString errorString)
{
    return Result { ArchiveError { std::move(errorString) } };
}

ArchiveImageCandidatesResult archiveImageCandidatesResult(
    std::vector<ImageNavigationCandidate> candidates);
ArchiveImageDataResult archiveImageDataResult(QByteArray data);

const ArchiveBackendOperations *kArchiveBackendOperations();
const ArchiveBackendOperations *libArchiveBackendOperations();
const ArchiveBackendOperations *directoryBackendOperations();
}

#endif
