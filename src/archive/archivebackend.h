// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEBACKEND_H
#define KIRIVIEW_ARCHIVEBACKEND_H

#include "location/imagelocation.h"
#include "navigation/imagenavigationtypes.h"

#include <QByteArray>
#include <QString>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

namespace KiriView {
struct ArchiveError {
    QString errorString;
};

struct ArchiveImageCandidates {
    std::vector<ImageNavigationCandidate> candidates;
};

struct ArchiveImageData {
    QByteArray data;
};

template <typename Value> using ArchiveResult = std::variant<Value, ArchiveError>;
using ArchiveImageCandidatesResult = ArchiveResult<ArchiveImageCandidates>;
using ArchiveImageDataResult = ArchiveResult<ArchiveImageData>;

class MediaEntrySource
{
public:
    virtual ~MediaEntrySource() = default;

    virtual ArchiveImageCandidatesResult loadImageCandidates() = 0;
    virtual ArchiveImageDataResult loadImageData(const QUrl &imageUrl) = 0;
};

using MediaEntrySourcePtr = std::shared_ptr<MediaEntrySource>;
using MediaEntrySourceOpenResult = ArchiveResult<MediaEntrySourcePtr>;
using MediaEntrySourceFactory
    = std::function<MediaEntrySourceOpenResult(const OpenedCollectionScopeLocation &)>;

ArchiveImageCandidatesResult loadMediaEntrySourceCandidates(
    const OpenedCollectionScopeLocation &archiveCollection);
ArchiveImageDataResult loadMediaEntrySourceImageData(
    const OpenedCollectionScopeLocation &archiveCollection, const QUrl &imageUrl);
MediaEntrySourceOpenResult openMediaEntrySource(
    const OpenedCollectionScopeLocation &archiveCollection);
}

#endif
