// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCEBACKEND_H
#define KIRIVIEW_MEDIAENTRYSOURCEBACKEND_H

#include "location/imagelocation.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QByteArray>
#include <QString>
#include <functional>
#include <memory>
#include <variant>
#include <vector>

namespace KiriView {
struct MediaEntrySourceError {
    QString errorString;
};

struct MediaEntrySourceCandidates {
    std::vector<ImageDocumentPageCandidate> candidates;
};

struct MediaEntrySourceImageData {
    QByteArray data;
};

template <typename Value> using MediaEntrySourceResult = std::variant<Value, MediaEntrySourceError>;
using MediaEntrySourceCandidatesResult = MediaEntrySourceResult<MediaEntrySourceCandidates>;
using MediaEntrySourceImageDataResult = MediaEntrySourceResult<MediaEntrySourceImageData>;

class MediaEntrySource
{
public:
    virtual ~MediaEntrySource() = default;

    virtual MediaEntrySourceCandidatesResult loadImageDocumentPageCandidates() = 0;
    virtual MediaEntrySourceImageDataResult loadImageData(const QUrl &imageUrl) = 0;
};

using MediaEntrySourcePtr = std::shared_ptr<MediaEntrySource>;
using MediaEntrySourceOpenResult = MediaEntrySourceResult<MediaEntrySourcePtr>;
using MediaEntrySourceFactory
    = std::function<MediaEntrySourceOpenResult(const OpenedCollectionScopeLocation &)>;

MediaEntrySourceCandidatesResult loadMediaEntrySourceCandidates(
    const OpenedCollectionScopeLocation &openedCollectionScope);
MediaEntrySourceImageDataResult loadMediaEntrySourceImageData(
    const OpenedCollectionScopeLocation &openedCollectionScope, const QUrl &imageUrl);
MediaEntrySourceOpenResult openMediaEntrySource(
    const OpenedCollectionScopeLocation &openedCollectionScope);
}

#endif
