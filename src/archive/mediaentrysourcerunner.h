// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCERUNNER_H
#define KIRIVIEW_MEDIAENTRYSOURCERUNNER_H

#include "archivebackend.h"

#include <mutex>
#include <optional>
#include <vector>

namespace KiriView {
class MediaEntrySourceRunner final
{
public:
    MediaEntrySourceRunner(
        OpenedCollectionScopeLocation archiveCollection, MediaEntrySourceFactory sourceFactory);

    const OpenedCollectionScopeLocation &openedCollectionScope() const;

    ArchiveImageCandidatesResult loadImageCandidates();
    ArchiveImageDataResult loadImageData(const QUrl &imageUrl);
    std::optional<std::vector<ImageNavigationCandidate>> cachedImageCandidates();

private:
    std::optional<QString> ensureSource();

    OpenedCollectionScopeLocation m_archiveCollection;
    MediaEntrySourceFactory m_sourceFactory;
    MediaEntrySourcePtr m_source;
    QString m_openErrorString;
    std::optional<std::vector<ImageNavigationCandidate>> m_cachedCandidates;
    bool m_openAttempted = false;
    std::mutex m_mutex;
};
}

#endif
