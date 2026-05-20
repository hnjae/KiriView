// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEBACKEND_H
#define KIRIVIEW_ARCHIVEBACKEND_H

#include "imagelocation.h"
#include "imagenavigationtypes.h"

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

class ArchiveDocumentSession
{
public:
    virtual ~ArchiveDocumentSession() = default;

    virtual ArchiveImageCandidatesResult loadImageCandidates() = 0;
    virtual ArchiveImageDataResult loadImageData(const QUrl &imageUrl) = 0;
};

using ArchiveDocumentSessionPtr = std::shared_ptr<ArchiveDocumentSession>;
using ArchiveDocumentSessionOpenResult = ArchiveResult<ArchiveDocumentSessionPtr>;
using ArchiveDocumentSessionFactory
    = std::function<ArchiveDocumentSessionOpenResult(const ArchiveDocumentLocation &)>;

ArchiveImageCandidatesResult loadArchiveDocumentImageCandidates(
    const ArchiveDocumentLocation &archiveDocument);
ArchiveImageDataResult loadArchiveDocumentImageData(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl);
ArchiveDocumentSessionOpenResult openArchiveDocumentSession(
    const ArchiveDocumentLocation &archiveDocument);
}

#endif
