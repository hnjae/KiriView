// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEDOCUMENTSESSIONSTORE_H
#define KIRIVIEW_ARCHIVEDOCUMENTSESSIONSTORE_H

#include "archivebackend.h"
#include "imageasyncdependencies.h"
#include "imageasyncticket.h"
#include "imageiojob.h"

#include <QObject>
#include <memory>
#include <optional>
#include <vector>

namespace KiriView {
struct ImageDocumentSourceLoadRequest;
class ArchiveDocumentSessionRunner;
struct ArchiveDocumentCandidateLoad;
struct ArchiveDocumentCandidateLoadBatch;

class ArchiveDocumentSessionStore final : public QObject
{
public:
    explicit ArchiveDocumentSessionStore(
        ArchiveDocumentSessionFactory sessionFactory = {}, QObject *parent = nullptr);
    ~ArchiveDocumentSessionStore() override;

    ImageNavigationCandidateProvider wrapCandidateProvider(
        ImageNavigationCandidateProvider provider);
    ImageDecodeDependencies wrapDecodeDependencies(ImageDecodeDependencies dependencies);
    ImageAsyncDependencies wrapDependencies(ImageAsyncDependencies dependencies);

    void prepareForSourceLoad(const ImageDocumentSourceLoadRequest &request,
        const ArchiveDocumentLocation &displayedArchiveDocument);
    void clear();

    bool hasCurrentArchiveDocument() const;
    bool hasCurrentArchiveDocument(const ArchiveDocumentLocation &archiveDocument) const;

    ImageIoJob loadArchiveImages(QObject *receiver, ArchiveDocumentLocation archiveDocument,
        ImageCandidatesCallback callback, ErrorCallback errorCallback);
    ImageIoJob loadArchiveImageData(QObject *receiver, ImageDecodeRequest request,
        ImageDataCallback callback, ErrorCallback errorCallback);

private:
    void switchToArchiveDocument(ArchiveDocumentLocation archiveDocument);
    void startCandidateLoad(quint64 generation);
    void finishCandidateLoad(quint64 generation, ArchiveImageCandidatesResult result);
    ArchiveDocumentCandidateLoadBatch &currentCandidateLoadBatch();
    void cancelCandidateLoadBatch();

    ArchiveDocumentSessionFactory m_sessionFactory;
    ArchiveDocumentLocation m_archiveDocument;
    std::shared_ptr<ArchiveDocumentSessionRunner> m_runner;
    std::optional<std::vector<ImageNavigationCandidate>> m_cachedCandidates;
    std::unique_ptr<ArchiveDocumentCandidateLoadBatch> m_candidateLoadBatch;
    ImageAsyncTicket m_generation;
};
}

#endif
