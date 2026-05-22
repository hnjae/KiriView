// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEDOCUMENTSESSIONRUNNER_H
#define KIRIVIEW_ARCHIVEDOCUMENTSESSIONRUNNER_H

#include "archivebackend.h"

#include <mutex>
#include <optional>
#include <vector>

namespace KiriView {
class ArchiveDocumentSessionRunner final
{
public:
    ArchiveDocumentSessionRunner(
        ArchiveDocumentLocation archiveDocument, ArchiveDocumentSessionFactory sessionFactory);

    const ArchiveDocumentLocation &archiveDocument() const;

    ArchiveImageCandidatesResult loadImageCandidates();
    ArchiveImageDataResult loadImageData(const QUrl &imageUrl);
    std::optional<std::vector<ImageNavigationCandidate>> cachedImageCandidates();

private:
    std::optional<QString> ensureSession();

    ArchiveDocumentLocation m_archiveDocument;
    ArchiveDocumentSessionFactory m_sessionFactory;
    ArchiveDocumentSessionPtr m_session;
    QString m_openErrorString;
    std::optional<std::vector<ImageNavigationCandidate>> m_cachedCandidates;
    bool m_openAttempted = false;
    std::mutex m_mutex;
};
}

#endif
