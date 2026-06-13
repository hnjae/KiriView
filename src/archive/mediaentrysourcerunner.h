// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCERUNNER_H
#define KIRIVIEW_MEDIAENTRYSOURCERUNNER_H

#include "mediaentrysourcebackend.h"

#include <mutex>
#include <optional>
#include <vector>

namespace kiriview {
class MediaEntrySourceRunner final
{
public:
    MediaEntrySourceRunner(
        OpenedCollectionScopeLocation openedCollectionScope, MediaEntrySourceFactory sourceFactory);

    const OpenedCollectionScopeLocation &openedCollectionScope() const;

    MediaEntrySourceCandidatesResult loadImageDocumentPageCandidates();
    MediaEntrySourceImageDataResult loadImageData(const QUrl &imageUrl);
    std::optional<std::vector<ImageDocumentPageCandidate>> cachedImageDocumentPageCandidates();

private:
    std::optional<QString> ensureSource();

    OpenedCollectionScopeLocation m_openedCollectionScope;
    MediaEntrySourceFactory m_sourceFactory;
    MediaEntrySourcePtr m_source;
    QString m_openErrorString;
    std::optional<std::vector<ImageDocumentPageCandidate>> m_cachedCandidates;
    bool m_openAttempted = false;
    std::mutex m_mutex;
};
}

#endif
