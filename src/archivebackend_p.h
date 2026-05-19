// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEBACKEND_P_H
#define KIRIVIEW_ARCHIVEBACKEND_P_H

#include "archivebackend.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QByteArray>
#include <QString>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView::ArchiveBackendDetail {
using ArchiveDocumentSessionOpener
    = ArchiveDocumentSessionOpenResult (*)(const ArchiveDocumentLocation &);

struct ArchiveBackendOperations {
    ArchiveDocumentSessionOpener openSession;
};

class ArchiveDocumentSessionWithCandidateSnapshot : public ArchiveDocumentSession
{
public:
    explicit ArchiveDocumentSessionWithCandidateSnapshot(
        std::vector<ImageNavigationCandidate> candidates);

    ArchiveImageCandidatesResult loadImageCandidates() final;

protected:
    void replaceCandidateSnapshot(std::vector<ImageNavigationCandidate> candidates);

private:
    std::vector<ImageNavigationCandidate> m_candidates;
};

std::optional<ImageNavigationCandidate> archiveImageCandidate(
    const ArchiveDocumentLocation &archiveDocument, const QString &entryPath);
std::optional<QString> archiveImageEntryPathForRead(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl);
QString fallbackArchiveOpenError(const ArchiveDocumentLocation &archiveDocument);
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
