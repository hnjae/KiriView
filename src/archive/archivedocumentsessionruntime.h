// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEDOCUMENTSESSIONRUNTIME_H
#define KIRIVIEW_ARCHIVEDOCUMENTSESSIONRUNTIME_H

#include "archivebackend.h"
#include "archivedocumentcandidateloadstate.h"
#include "async/imageasyncticket.h"
#include "async/imageiojob.h"
#include "decoding/imagedecodedependencies.h"

#include <memory>

class QObject;

namespace KiriView {
class ArchiveDocumentSessionRunner;

class ArchiveDocumentSessionRuntime final
{
public:
    explicit ArchiveDocumentSessionRuntime(
        QObject *context, ArchiveDocumentSessionFactory sessionFactory = {});
    ~ArchiveDocumentSessionRuntime();

    void clear();
    void switchToArchiveDocument(ArchiveDocumentLocation archiveDocument);

    bool hasCurrentArchiveDocument() const;
    bool hasCurrentArchiveDocument(const ArchiveDocumentLocation &archiveDocument) const;

    ImageIoJob loadArchiveImages(QObject *receiver, ArchiveDocumentLocation archiveDocument,
        ImageCandidatesCallback callback, ErrorCallback errorCallback);
    ImageIoJob loadArchiveImageData(QObject *receiver, ImageDecodeRequest request,
        ImageDataCallback callback, ErrorCallback errorCallback);

private:
    void startCandidateLoad(quint64 generation);
    void finishCandidateLoad(quint64 generation, ArchiveImageCandidatesResult result);
    void cancelCandidateLoadBatch();

    QObject *m_context = nullptr;
    ArchiveDocumentSessionFactory m_sessionFactory;
    ArchiveDocumentLocation m_archiveDocument;
    std::shared_ptr<ArchiveDocumentSessionRunner> m_runner;
    ArchiveDocumentCandidateLoadState m_candidateLoadState;
    ImageAsyncTicket m_generation;
};
}

#endif
