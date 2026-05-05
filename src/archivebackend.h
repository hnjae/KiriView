// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEBACKEND_H
#define KIRIVIEW_ARCHIVEBACKEND_H

#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QByteArray>
#include <QString>
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

using ArchiveImageCandidatesResult = std::variant<ArchiveImageCandidates, ArchiveError>;
using ArchiveImageDataResult = std::variant<ArchiveImageData, ArchiveError>;

ArchiveImageCandidatesResult loadArchiveDocumentImageCandidates(
    const ArchiveDocumentLocation &archiveDocument);
ArchiveImageDataResult loadArchiveDocumentImageData(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl);
}

#endif
