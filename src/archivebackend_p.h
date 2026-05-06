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
using ArchiveImageCandidatesLoader
    = ArchiveImageCandidatesResult (*)(const ArchiveDocumentLocation &);
using ArchiveImageDataLoader
    = ArchiveImageDataResult (*)(const ArchiveDocumentLocation &, const QString &);

struct ArchiveBackendOperations {
    ArchiveImageCandidatesLoader loadImageCandidates;
    ArchiveImageDataLoader loadImageData;
};

std::optional<ImageNavigationCandidate> archiveImageCandidate(
    const ArchiveDocumentLocation &archiveDocument, const QString &entryPath);
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
}

#endif
