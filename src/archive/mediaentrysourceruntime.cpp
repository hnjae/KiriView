// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourceruntime.h"

#include "async/imagecallback.h"
#include "async/imageioworkerjob.h"
#include "mediaentrysourcebackend_p.h"
#include "mediaentrysourcerunner.h"

#include <optional>
#include <utility>
#include <variant>

namespace {
namespace Backend = kiriview::MediaEntrySourceBackendDetail;

void finishMediaEntrySourceCandidateResult(const kiriview::MediaEntrySourceCandidatesResult& result,
    const kiriview::ImageDocumentPageCandidatesCallback& callback,
    const kiriview::ErrorCallback& errorCallback)
{
    if (const auto* error = std::get_if<kiriview::MediaEntrySourceError>(&result)) {
        kiriview::invokeIfSet(errorCallback, error->errorString);
        return;
    }

    const auto* candidates = std::get_if<kiriview::MediaEntrySourceCandidates>(&result);
    if (candidates != nullptr) {
        kiriview::invokeIfSet(callback, candidates->candidates);
    }
}

void finishMediaEntrySourceDataResult(kiriview::MediaEntrySourceImageDataResult result,
    kiriview::ImageDataCallback callback, kiriview::ErrorCallback errorCallback)
{
    if (const auto* error = std::get_if<kiriview::MediaEntrySourceError>(&result)) {
        kiriview::invokeIfSet(errorCallback, error->errorString);
        return;
    }

    auto* data = std::get_if<kiriview::MediaEntrySourceImageData>(&result);
    if (data != nullptr) {
        kiriview::invokeIfSet(callback, std::move(data->data));
    }
}

}

namespace kiriview {
MediaEntrySourceRuntime::MediaEntrySourceRuntime(
    QObject* context, MediaEntrySourceFactory sourceFactory, ImageWorkerScheduler workerScheduler)
    : m_context(context)
    , m_sourceFactory(std::move(sourceFactory))
    , m_workerScheduler(std::move(workerScheduler))
{
}

MediaEntrySourceRuntime::~MediaEntrySourceRuntime() { clear(); }

void MediaEntrySourceRuntime::clear()
{
    cancelCandidateLoadBatch();
    m_sourceGeneration.invalidate();
    m_runner.reset();
}

void MediaEntrySourceRuntime::switchToOpenedCollectionScope(
    OpenedCollectionScopeLocation openedCollectionScope)
{
    if (openedCollectionScope.isEmpty()) {
        clear();
        return;
    }
    if (hasCurrentOpenedCollectionScope(openedCollectionScope)) {
        return;
    }

    cancelCandidateLoadBatch();
    m_sourceGeneration.invalidate();
    m_runner = std::make_shared<MediaEntrySourceRunner>(
        std::move(openedCollectionScope), m_sourceFactory);
}

bool MediaEntrySourceRuntime::hasCurrentOpenedCollectionScope() const
{
    return m_runner != nullptr && !m_runner->openedCollectionScope().isEmpty();
}

bool MediaEntrySourceRuntime::hasCurrentOpenedCollectionScope(
    const OpenedCollectionScopeLocation& openedCollectionScope) const
{
    return hasCurrentOpenedCollectionScope()
        && sameOpenedCollectionScopeLocation(
            m_runner->openedCollectionScope(), openedCollectionScope);
}

ImageIoJob MediaEntrySourceRuntime::loadOpenedCollectionCandidates(QObject* receiver,
    OpenedCollectionScopeLocation openedCollectionScope,
    ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback)
{
    const OpenedCollectionScopeLocation requestedOpenedCollectionScope = openedCollectionScope;
    switchToOpenedCollectionScope(std::move(openedCollectionScope));
    if (m_runner == nullptr) {
        invokeIfSet(errorCallback,
            Backend::fallbackMediaEntrySourceOpenError(requestedOpenedCollectionScope));
        return ImageIoJob();
    }

    if (receiver != nullptr && m_candidateLoadState.batchInProgress()) {
        return m_candidateLoadState.addLoad(
            receiver, std::move(callback), std::move(errorCallback));
    }

    if (const std::optional<std::vector<ImageDocumentPageCandidate>> cachedCandidates
        = m_runner->cachedImageDocumentPageCandidates()) {
        invokeIfSet(callback, *cachedCandidates);
        return ImageIoJob();
    }

    if (receiver == nullptr) {
        finishMediaEntrySourceCandidateResult(
            m_runner->loadImageDocumentPageCandidates(), callback, errorCallback);
        return ImageIoJob();
    }

    ImageIoJob ioJob
        = m_candidateLoadState.addLoad(receiver, std::move(callback), std::move(errorCallback));
    if (const std::optional<MediaEntrySourceCandidateLoadBatch> batch
        = m_candidateLoadState.startBatch()) {
        startCandidateLoad(*batch);
    }

    return ioJob;
}

ImageIoJob MediaEntrySourceRuntime::loadOpenedCollectionImageData(QObject* receiver,
    ImageDecodeRequest request, ImageDataCallback callback, ErrorCallback errorCallback)
{
    const OpenedCollectionScopeLocation& requestedOpenedCollectionScope
        = request.openedCollectionScope();
    switchToOpenedCollectionScope(request.openedCollectionScope());
    if (m_runner == nullptr) {
        invokeIfSet(errorCallback,
            Backend::fallbackMediaEntrySourceOpenError(requestedOpenedCollectionScope));
        return ImageIoJob();
    }

    if (receiver == nullptr) {
        finishMediaEntrySourceDataResult(m_runner->loadImageData(request.imageUrl()),
            std::move(callback), std::move(errorCallback));
        return ImageIoJob();
    }

    const quint64 generation = m_sourceGeneration.current();
    const QUrl& imageUrl = request.imageUrl();
    std::shared_ptr<MediaEntrySourceRunner> runner = m_runner;

    return startImageIoWorkerJob(
        m_context, receiver, m_workerScheduler,
        [runner = std::move(runner), imageUrl]() { return runner->loadImageData(imageUrl); },
        [generation, this, callback = std::move(callback),
            errorCallback = std::move(errorCallback)](
            MediaEntrySourceImageDataResult result) mutable {
            if (!m_sourceGeneration.accepts(generation)) {
                return;
            }

            finishMediaEntrySourceDataResult(
                std::move(result), std::move(callback), std::move(errorCallback));
        });
}

void MediaEntrySourceRuntime::startCandidateLoad(MediaEntrySourceCandidateLoadBatch batch)
{
    if (m_runner == nullptr || !m_candidateLoadState.acceptsBatch(batch)) {
        return;
    }

    std::shared_ptr<MediaEntrySourceRunner> runner = m_runner;
    m_workerScheduler.run(
        m_context,
        [runner = std::move(runner)]() { return runner->loadImageDocumentPageCandidates(); },
        [this, batch](MediaEntrySourceCandidatesResult result) mutable {
            finishCandidateLoad(batch, std::move(result));
        });
}

void MediaEntrySourceRuntime::finishCandidateLoad(
    MediaEntrySourceCandidateLoadBatch batch, MediaEntrySourceCandidatesResult result)
{
    std::vector<MediaEntrySourceCandidateLoad> pendingLoads
        = m_candidateLoadState.finishBatch(batch);

    for (const MediaEntrySourceCandidateLoad& load : pendingLoads) {
        load.completion.claimAndDelete([&]() {
            finishMediaEntrySourceCandidateResult(result, load.callback, load.errorCallback);
        });
    }
}

void MediaEntrySourceRuntime::cancelCandidateLoadBatch() { m_candidateLoadState.cancel(); }
}
