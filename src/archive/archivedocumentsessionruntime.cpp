// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivedocumentsessionruntime.h"

#include "archivebackend_p.h"
#include "async/imagecallback.h"
#include "async/imageioworkerjob.h"

#include <mutex>
#include <optional>
#include <utility>
#include <variant>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;

KiriView::ArchiveDocumentSessionFactory defaultSessionFactory(
    KiriView::ArchiveDocumentSessionFactory sessionFactory)
{
    return sessionFactory ? std::move(sessionFactory) : KiriView::openArchiveDocumentSession;
}

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
class ArchiveDocumentSessionRunner final
{
public:
    ArchiveDocumentSessionRunner(
        ArchiveDocumentLocation archiveDocument, ArchiveDocumentSessionFactory sessionFactory)
        : m_archiveDocument(std::move(archiveDocument))
        , m_sessionFactory(defaultSessionFactory(std::move(sessionFactory)))
    {
    }

    ArchiveImageCandidatesResult loadImageCandidates()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_cachedCandidates.has_value()) {
            return ArchiveImageCandidates { *m_cachedCandidates };
        }

        const std::optional<QString> errorString = ensureSession();
        if (errorString.has_value()) {
            return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(*errorString);
        }

        ArchiveImageCandidatesResult result = m_session->loadImageCandidates();
        if (const auto *candidates = std::get_if<ArchiveImageCandidates>(&result)) {
            m_cachedCandidates = candidates->candidates;
        }
        return result;
    }

    ArchiveImageDataResult loadImageData(const QUrl &imageUrl)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const std::optional<QString> errorString = ensureSession();
        if (errorString.has_value()) {
            return Backend::archiveErrorResult<ArchiveImageDataResult>(*errorString);
        }

        return m_session->loadImageData(imageUrl);
    }

    std::optional<std::vector<ImageNavigationCandidate>> cachedImageCandidates()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cachedCandidates;
    }

private:
    std::optional<QString> ensureSession()
    {
        if (m_session != nullptr) {
            return std::nullopt;
        }
        if (m_openAttempted) {
            return m_openErrorString;
        }

        m_openAttempted = true;
        ArchiveDocumentSessionOpenResult result = m_sessionFactory(m_archiveDocument);
        if (const auto *error = std::get_if<ArchiveError>(&result)) {
            m_openErrorString = error->errorString;
            return m_openErrorString;
        }

        const auto *session = std::get_if<ArchiveDocumentSessionPtr>(&result);
        if (session == nullptr || *session == nullptr) {
            m_openErrorString = Backend::fallbackArchiveOpenError(m_archiveDocument);
            return m_openErrorString;
        }

        m_session = *session;
        return std::nullopt;
    }

    ArchiveDocumentLocation m_archiveDocument;
    ArchiveDocumentSessionFactory m_sessionFactory;
    ArchiveDocumentSessionPtr m_session;
    QString m_openErrorString;
    std::optional<std::vector<ImageNavigationCandidate>> m_cachedCandidates;
    bool m_openAttempted = false;
    std::mutex m_mutex;
};

ArchiveDocumentSessionRuntime::ArchiveDocumentSessionRuntime(
    QObject *context, ArchiveDocumentSessionFactory sessionFactory)
    : m_context(context)
    , m_sessionFactory(defaultSessionFactory(std::move(sessionFactory)))
{
}

ArchiveDocumentSessionRuntime::~ArchiveDocumentSessionRuntime() { clear(); }

void ArchiveDocumentSessionRuntime::clear()
{
    cancelCandidateLoadBatch();
    m_generation.invalidate();
    m_archiveDocument = ArchiveDocumentLocation::none();
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
    m_archiveDocument = std::move(archiveDocument);
    m_runner = std::make_shared<ArchiveDocumentSessionRunner>(m_archiveDocument, m_sessionFactory);
}

bool ArchiveDocumentSessionRuntime::hasCurrentArchiveDocument() const
{
    return m_runner != nullptr && !m_archiveDocument.isEmpty();
}

bool ArchiveDocumentSessionRuntime::hasCurrentArchiveDocument(
    const ArchiveDocumentLocation &archiveDocument) const
{
    return hasCurrentArchiveDocument()
        && sameArchiveDocumentLocation(m_archiveDocument, archiveDocument);
}

ImageIoJob ArchiveDocumentSessionRuntime::loadArchiveImages(QObject *receiver,
    ArchiveDocumentLocation archiveDocument, ImageCandidatesCallback callback,
    ErrorCallback errorCallback)
{
    switchToArchiveDocument(std::move(archiveDocument));
    if (m_runner == nullptr) {
        invokeIfSet(errorCallback, Backend::fallbackArchiveOpenError(m_archiveDocument));
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
    switchToArchiveDocument(request.archiveDocument());
    if (m_runner == nullptr) {
        invokeIfSet(errorCallback, Backend::fallbackArchiveOpenError(m_archiveDocument));
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
