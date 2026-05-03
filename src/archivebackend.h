// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEBACKEND_H
#define KIRIVIEW_ARCHIVEBACKEND_H

#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QByteArray>
#include <QString>
#include <vector>

namespace KiriView {
struct ArchiveImageCandidatesResult {
    std::vector<ImageNavigationCandidate> candidates;
    QString errorString;
    bool success = false;
};

struct ArchiveImageDataResult {
    QByteArray data;
    QString errorString;
    bool success = false;
};

ArchiveImageCandidatesResult loadArchiveDocumentImageCandidates(
    const ArchiveDocumentLocation &archiveDocument);
ArchiveImageDataResult loadArchiveDocumentImageData(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl);
}

#endif
