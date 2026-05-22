// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivedocumentsessionruntime.h"

#include "archivebackend_p.h"
#include "archivedocumentsessionrunner.h"
#include "async/imagecallback.h"
#include "async/imageioworkerjob.h"

#include <utility>
#include <variant>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;

void finishArchiveCandidateResult(const KiriView::ArchiveImageCandidatesResult &result,
    const KiriView::ImageCandidatesCallback &callback, const KiriView::ErrorCallback &errorCallback)
{
    if (const auto *error = std::get_if<KiriView::ArchiveError>(&result)) {
        KiriView::invokeIfSet(errorCallback, error->errorString);
        return;
    }

    const auto *candidates = std::get_if<KiriView::ArchiveImageCandidates>(&result);
    if (candidates != nullptr) {
        KiriView::invokeIfSet(callback, candidates->candidates);
    }
}

void finishArchiveDataResult(KiriView::ArchiveImageDataResult result,
    KiriView::ImageDataCallback callback, KiriView::ErrorCallback errorCallback)
{
    if (const auto *error = std::get_if<KiriView::ArchiveError>(&result)) {
        KiriView::invokeIfSet(errorCallback, error->errorString);
        return;
    }

    auto *data = std::get_if<KiriView::ArchiveImageData>(&result);
    if (data != nullptr) {
        KiriView::invokeIfSet(callback, std::move(data->data));
    }
}

}

namespace KiriView {
ArchiveDocumentSessionRuntime::ArchiveDocumentSessionRuntime(
    QObject *context, ArchiveDocumentSessionFactory sessionFactory)
    : m_context(context)
    , m_sessionFactory(std::move(sessionFactory))
{
}

ArchiveDocumentSessionRuntime::~ArchiveDocumentSessionRuntime() { clear(); }

void ArchiveDocumentSessionRuntime::clear()
{
    cancelCandidateLoadBatch();
    m_generation.invalidate();
    m_runner.reset();
}

void ArchiveDocumentSessionRuntime::switchToArchiveDocument(ArchiveDocumentLocation archiveDocument)
{
    if (archiveDocument.isEmpty()) {
        clear();
        return;
    }
    if (hasCurrentArchiveDocument(archiveDocument)) {
        return;
    }

    cancelCandidateLoadBatch();
    m_generation.invalidate();
    m_runner = std::make_shared<ArchiveDocumentSessionRunner>(
        std::move(archiveDocument), m_sessionFactory);
}

bool ArchiveDocumentSessionRuntime::hasCurrentArchiveDocument() const
{
    return m_runner != nullptr && !m_runner->archiveDocument().isEmpty();
}

bool ArchiveDocumentSessionRuntime::hasCurrentArchiveDocument(
    const ArchiveDocumentLocation &archiveDocument) const
{
    return hasCurrentArchiveDocument()
        && sameArchiveDocumentLocation(m_runner->archiveDocument(), archiveDocument);
}

ImageIoJob ArchiveDocumentSessionRuntime::loadArchiveImages(QObject *receiver,
    ArchiveDocumentLocation archiveDocument, ImageCandidatesCallback callback,
    ErrorCallback errorCallback)
{
    const ArchiveDocumentLocation requestedArchiveDocument = archiveDocument;
    switchToArchiveDocument(std::move(archiveDocument));
    if (m_runner == nullptr) {
        invokeIfSet(errorCallback, Backend::fallbackArchiveOpenError(requestedArchiveDocument));
        return ImageIoJob();
    }

    if (const std::optional<std::vector<ImageNavigationCandidate>> cachedCandidates
        = m_runner->cachedImageCandidates()) {
        invokeIfSet(callback, *cachedCandidates);
        return ImageIoJob();
    }

    if (receiver == nullptr) {
        finishArchiveCandidateResult(m_runner->loadImageCandidates(), callback, errorCallback);
        return ImageIoJob();
    }

    const quint64 generation = m_generation.current();
    ImageIoJob ioJob = m_candidateLoadState.addLoad(
        receiver, generation, std::move(callback), std::move(errorCallback));
    if (m_candidateLoadState.startBatch(generation)) {
        startCandidateLoad(generation);
    }

    return ioJob;
}

ImageIoJob ArchiveDocumentSessionRuntime::loadArchiveImageData(QObject *receiver,
    ImageDecodeRequest request, ImageDataCallback callback, ErrorCallback errorCallback)
{
    const ArchiveDocumentLocation requestedArchiveDocument = request.archiveDocument();
    switchToArchiveDocument(request.archiveDocument());
    if (m_runner == nullptr) {
        invokeIfSet(errorCallback, Backend::fallbackArchiveOpenError(requestedArchiveDocument));
        return ImageIoJob();
    }

    if (receiver == nullptr) {
        finishArchiveDataResult(m_runner->loadImageData(request.imageUrl()), std::move(callback),
            std::move(errorCallback));
        return ImageIoJob();
    }

    const quint64 generation = m_generation.current();
    const QUrl imageUrl = request.imageUrl();
    std::shared_ptr<ArchiveDocumentSessionRunner> runner = m_runner;

    return startImageIoWorkerJob(
        m_context, receiver,
        [runner = std::move(runner), imageUrl]() { return runner->loadImageData(imageUrl); },
        [generation, this, callback = std::move(callback),
            errorCallback = std::move(errorCallback)](ArchiveImageDataResult result) mutable {
            if (!m_generation.accepts(generation)) {
                return;
            }

            finishArchiveDataResult(
                std::move(result), std::move(callback), std::move(errorCallback));
        });
}

void ArchiveDocumentSessionRuntime::startCandidateLoad(quint64 generation)
{
    if (m_runner == nullptr || !m_generation.accepts(generation)
        || !m_candidateLoadState.acceptsBatch(generation)) {
        return;
    }

    std::shared_ptr<ArchiveDocumentSessionRunner> runner = m_runner;
    runAsyncWorker(
        m_context, [runner = std::move(runner)]() { return runner->loadImageCandidates(); },
        [this, generation](ArchiveImageCandidatesResult result) mutable {
            finishCandidateLoad(generation, std::move(result));
        });
}

void ArchiveDocumentSessionRuntime::finishCandidateLoad(
    quint64 generation, ArchiveImageCandidatesResult result)
{
    if (!m_generation.accepts(generation)) {
        return;
    }

    std::vector<ArchiveDocumentCandidateLoad> pendingLoads
        = m_candidateLoadState.finishBatch(generation);

    for (const ArchiveDocumentCandidateLoad &load : pendingLoads) {
        load.completion.claimAndDelete(
            [&]() { finishArchiveCandidateResult(result, load.callback, load.errorCallback); });
    }
}

void ArchiveDocumentSessionRuntime::cancelCandidateLoadBatch() { m_candidateLoadState.cancel(); }
}
