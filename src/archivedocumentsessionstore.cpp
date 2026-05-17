// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivedocumentsessionstore.h"

#include "archivebackend_p.h"
#include "imageasyncworker.h"
#include "imagecallback.h"
#include "imagecontainer.h"
#include "imagedocumentsourceloadrequest.h"
#include "imageloadplan.h"

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

std::optional<KiriView::ArchiveDocumentLocation> archiveDocumentForSourceLoad(
    const KiriView::ImageDocumentSourceLoadRequest &request,
    const KiriView::ArchiveDocumentLocation &displayedArchiveDocument)
{
    const KiriView::ImageArchiveLoadPlan plan
        = KiriView::imageArchiveLoadPlan(KiriView::ImageLoadRequest::fromLocation(
            request.sourceUrl, displayedArchiveDocument, request.containerNavigationUrl));
    if (plan.archiveDocument.isEmpty()) {
        return std::nullopt;
    }

    return plan.archiveDocument;
}

void cancelArchiveSessionToken(QObject *object)
{
    if (object != nullptr) {
        object->deleteLater();
    }
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
        const std::optional<QString> errorString = ensureSession();
        if (errorString.has_value()) {
            return Backend::archiveErrorResult<ArchiveImageCandidatesResult>(*errorString);
        }

        return m_session->loadImageCandidates();
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
    bool m_openAttempted = false;
    std::mutex m_mutex;
};

struct ArchiveDocumentCandidateLoad {
    QObject *token = nullptr;
    std::shared_ptr<ImageIoJobState> state;
    ImageCandidatesCallback callback;
    ErrorCallback errorCallback;
};

struct ArchiveDocumentCandidateLoadBatch {
    quint64 generation = 0;
    std::vector<std::shared_ptr<ArchiveDocumentCandidateLoad>> pendingLoads;
    bool inProgress = false;
};

ArchiveDocumentSessionStore::ArchiveDocumentSessionStore(
    ArchiveDocumentSessionFactory sessionFactory, QObject *parent)
    : QObject(parent)
    , m_sessionFactory(defaultSessionFactory(std::move(sessionFactory)))
{
}

ArchiveDocumentSessionStore::~ArchiveDocumentSessionStore() { clear(); }

ImageNavigationCandidateProvider ArchiveDocumentSessionStore::wrapCandidateProvider(
    ImageNavigationCandidateProvider provider)
{
    provider.archiveImages = [this](QObject *receiver, ArchiveDocumentLocation archiveDocument,
                                 ImageCandidatesCallback callback, ErrorCallback errorCallback) {
        return loadArchiveImages(
            receiver, std::move(archiveDocument), std::move(callback), std::move(errorCallback));
    };
    return provider;
}

ImageDecodeDependencies ArchiveDocumentSessionStore::wrapDecodeDependencies(
    ImageDecodeDependencies dependencies)
{
    ImageDataLoader upstreamDataLoader = std::move(dependencies.dataLoader);
    dependencies.dataLoader
        = [this, upstreamDataLoader = std::move(upstreamDataLoader)](QObject *receiver,
              ImageDecodeRequest request, ImageDataCallback callback, ErrorCallback errorCallback) {
              if (archiveDocumentContainsUrl(request.archiveDocument(), request.imageUrl())) {
                  return loadArchiveImageData(
                      receiver, std::move(request), std::move(callback), std::move(errorCallback));
              }

              if (!upstreamDataLoader) {
                  invokeIfSet(errorCallback, QString());
                  return ImageIoJob();
              }

              return upstreamDataLoader(
                  receiver, std::move(request), std::move(callback), std::move(errorCallback));
          };
    return dependencies;
}

ImageAsyncDependencies ArchiveDocumentSessionStore::wrapDependencies(
    ImageAsyncDependencies dependencies)
{
    dependencies.candidateProvider
        = wrapCandidateProvider(std::move(dependencies.candidateProvider));
    dependencies.imageDecode = wrapDecodeDependencies(std::move(dependencies.imageDecode));
    return dependencies;
}

void ArchiveDocumentSessionStore::prepareForSourceLoad(
    const ImageDocumentSourceLoadRequest &request,
    const ArchiveDocumentLocation &displayedArchiveDocument)
{
    const std::optional<ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForSourceLoad(request, displayedArchiveDocument);
    if (archiveDocument.has_value()) {
        switchToArchiveDocument(*archiveDocument);
        return;
    }

    clear();
}

void ArchiveDocumentSessionStore::clear()
{
    cancelCandidateLoadBatch();
    m_generation.invalidate();
    m_archiveDocument = ArchiveDocumentLocation::none();
    m_runner.reset();
    m_cachedCandidates.reset();
}

bool ArchiveDocumentSessionStore::hasCurrentArchiveDocument() const
{
    return m_runner != nullptr && !m_archiveDocument.isEmpty();
}

bool ArchiveDocumentSessionStore::hasCurrentArchiveDocument(
    const ArchiveDocumentLocation &archiveDocument) const
{
    return hasCurrentArchiveDocument()
        && sameArchiveDocumentLocation(m_archiveDocument, archiveDocument);
}

ImageIoJob ArchiveDocumentSessionStore::loadArchiveImages(QObject *receiver,
    ArchiveDocumentLocation archiveDocument, ImageCandidatesCallback callback,
    ErrorCallback errorCallback)
{
    switchToArchiveDocument(std::move(archiveDocument));
    if (m_runner == nullptr) {
        invokeIfSet(errorCallback, Backend::fallbackArchiveOpenError(m_archiveDocument));
        return ImageIoJob();
    }

    if (m_cachedCandidates.has_value()) {
        invokeIfSet(callback, *m_cachedCandidates);
        return ImageIoJob();
    }

    if (receiver == nullptr) {
        ArchiveImageCandidatesResult result = m_runner->loadImageCandidates();
        if (const auto *candidates = std::get_if<ArchiveImageCandidates>(&result)) {
            m_cachedCandidates = candidates->candidates;
        }
        finishArchiveCandidateResult(result, callback, errorCallback);
        return ImageIoJob();
    }

    auto load = std::make_shared<ArchiveDocumentCandidateLoad>();
    load->token = new QObject(receiver);
    load->callback = std::move(callback);
    load->errorCallback = std::move(errorCallback);

    ImageIoJob ioJob(load->token, cancelArchiveSessionToken);
    load->state = ioJob.state();
    ArchiveDocumentCandidateLoadBatch &batch = currentCandidateLoadBatch();
    batch.pendingLoads.push_back(load);

    if (!batch.inProgress) {
        startCandidateLoad(batch.generation);
    }

    return ioJob;
}

ImageIoJob ArchiveDocumentSessionStore::loadArchiveImageData(QObject *receiver,
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

    auto *token = new QObject(receiver);
    ImageIoJob ioJob(token, cancelArchiveSessionToken);
    std::shared_ptr<ImageIoJobState> jobState = ioJob.state();
    const quint64 generation = m_generation.current();
    const QUrl imageUrl = request.imageUrl();
    std::shared_ptr<ArchiveDocumentSessionRunner> runner = m_runner;

    runAsyncWorker(
        this, [runner = std::move(runner), imageUrl]() { return runner->loadImageData(imageUrl); },
        [token, jobState, generation, this, callback = std::move(callback),
            errorCallback = std::move(errorCallback)](ArchiveImageDataResult result) mutable {
            jobState->claimAndRun(token, [&]() mutable {
                token->deleteLater();
                if (!m_generation.accepts(generation)) {
                    return;
                }

                finishArchiveDataResult(
                    std::move(result), std::move(callback), std::move(errorCallback));
            });
        });

    return ioJob;
}

void ArchiveDocumentSessionStore::switchToArchiveDocument(ArchiveDocumentLocation archiveDocument)
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
    m_cachedCandidates.reset();
}

ArchiveDocumentCandidateLoadBatch &ArchiveDocumentSessionStore::currentCandidateLoadBatch()
{
    if (m_candidateLoadBatch == nullptr
        || !m_generation.accepts(m_candidateLoadBatch->generation)) {
        m_candidateLoadBatch = std::make_unique<ArchiveDocumentCandidateLoadBatch>();
        m_candidateLoadBatch->generation = m_generation.current();
    }

    return *m_candidateLoadBatch;
}

void ArchiveDocumentSessionStore::startCandidateLoad(quint64 generation)
{
    if (m_runner == nullptr || !m_generation.accepts(generation) || m_candidateLoadBatch == nullptr
        || m_candidateLoadBatch->generation != generation) {
        return;
    }

    m_candidateLoadBatch->inProgress = true;
    std::shared_ptr<ArchiveDocumentSessionRunner> runner = m_runner;
    runAsyncWorker(
        this, [runner = std::move(runner)]() { return runner->loadImageCandidates(); },
        [this, generation](ArchiveImageCandidatesResult result) mutable {
            finishCandidateLoad(generation, std::move(result));
        });
}

void ArchiveDocumentSessionStore::finishCandidateLoad(
    quint64 generation, ArchiveImageCandidatesResult result)
{
    if (!m_generation.accepts(generation) || m_candidateLoadBatch == nullptr
        || m_candidateLoadBatch->generation != generation) {
        return;
    }

    std::unique_ptr<ArchiveDocumentCandidateLoadBatch> batch = std::move(m_candidateLoadBatch);
    if (const auto *candidates = std::get_if<ArchiveImageCandidates>(&result)) {
        m_cachedCandidates = candidates->candidates;
    }

    std::vector<std::shared_ptr<ArchiveDocumentCandidateLoad>> pendingLoads
        = std::move(batch->pendingLoads);

    for (const std::shared_ptr<ArchiveDocumentCandidateLoad> &load : pendingLoads) {
        if (load == nullptr || load->state == nullptr) {
            continue;
        }

        load->state->claimAndRun(load->token, [&]() {
            load->token->deleteLater();
            finishArchiveCandidateResult(result, load->callback, load->errorCallback);
        });
    }
}

void ArchiveDocumentSessionStore::cancelCandidateLoadBatch()
{
    if (m_candidateLoadBatch == nullptr) {
        return;
    }

    for (const std::shared_ptr<ArchiveDocumentCandidateLoad> &load :
        m_candidateLoadBatch->pendingLoads) {
        if (load != nullptr && load->state != nullptr) {
            load->state->cancel();
        }
    }
    m_candidateLoadBatch.reset();
}
}
