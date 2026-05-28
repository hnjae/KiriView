// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourceruntime.h"

#include "archivebackend_p.h"
#include "async/imagecallback.h"
#include "async/imageioworkerjob.h"
#include "mediaentrysourcerunner.h"

#include <optional>
#include <utility>
#include <variant>

namespace {
namespace Backend = KiriView::ArchiveBackendDetail;

void finishMediaEntrySourceCandidateResult(const KiriView::ArchiveImageCandidatesResult &result,
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

void finishMediaEntrySourceDataResult(KiriView::ArchiveImageDataResult result,
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
MediaEntrySourceRuntime::MediaEntrySourceRuntime(
    QObject *context, MediaEntrySourceFactory sourceFactory)
    : m_context(context)
    , m_sourceFactory(std::move(sourceFactory))
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
    OpenedCollectionScopeLocation archiveCollection)
{
    if (archiveCollection.isEmpty()) {
        clear();
        return;
    }
    if (hasCurrentOpenedCollectionScope(archiveCollection)) {
        return;
    }

    cancelCandidateLoadBatch();
    m_sourceGeneration.invalidate();
    m_runner
        = std::make_shared<MediaEntrySourceRunner>(std::move(archiveCollection), m_sourceFactory);
}

bool MediaEntrySourceRuntime::hasCurrentOpenedCollectionScope() const
{
    return m_runner != nullptr && !m_runner->openedCollectionScope().isEmpty();
}

bool MediaEntrySourceRuntime::hasCurrentOpenedCollectionScope(
    const OpenedCollectionScopeLocation &archiveCollection) const
{
    return hasCurrentOpenedCollectionScope()
        && sameOpenedCollectionScopeLocation(m_runner->openedCollectionScope(), archiveCollection);
}

ImageIoJob MediaEntrySourceRuntime::loadOpenedCollectionCandidates(QObject *receiver,
    OpenedCollectionScopeLocation archiveCollection, ImageCandidatesCallback callback,
    ErrorCallback errorCallback)
{
    const OpenedCollectionScopeLocation requestedArchiveCollection = archiveCollection;
    switchToOpenedCollectionScope(std::move(archiveCollection));
    if (m_runner == nullptr) {
        invokeIfSet(errorCallback, Backend::fallbackArchiveOpenError(requestedArchiveCollection));
        return ImageIoJob();
    }

    if (receiver != nullptr && m_candidateLoadState.batchInProgress()) {
        return m_candidateLoadState.addLoad(
            receiver, std::move(callback), std::move(errorCallback));
    }

    if (const std::optional<std::vector<ImageNavigationCandidate>> cachedCandidates
        = m_runner->cachedImageCandidates()) {
        invokeIfSet(callback, *cachedCandidates);
        return ImageIoJob();
    }

    if (receiver == nullptr) {
        finishMediaEntrySourceCandidateResult(
            m_runner->loadImageCandidates(), callback, errorCallback);
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

ImageIoJob MediaEntrySourceRuntime::loadOpenedCollectionImageData(QObject *receiver,
    ImageDecodeRequest request, ImageDataCallback callback, ErrorCallback errorCallback)
{
    const OpenedCollectionScopeLocation requestedArchiveCollection
        = request.openedCollectionScope();
    switchToOpenedCollectionScope(request.openedCollectionScope());
    if (m_runner == nullptr) {
        invokeIfSet(errorCallback, Backend::fallbackArchiveOpenError(requestedArchiveCollection));
        return ImageIoJob();
    }

    if (receiver == nullptr) {
        finishMediaEntrySourceDataResult(m_runner->loadImageData(request.imageUrl()),
            std::move(callback), std::move(errorCallback));
        return ImageIoJob();
    }

    const quint64 generation = m_sourceGeneration.current();
    const QUrl imageUrl = request.imageUrl();
    std::shared_ptr<MediaEntrySourceRunner> runner = m_runner;

    return startImageIoWorkerJob(
        m_context, receiver,
        [runner = std::move(runner), imageUrl]() { return runner->loadImageData(imageUrl); },
        [generation, this, callback = std::move(callback),
            errorCallback = std::move(errorCallback)](ArchiveImageDataResult result) mutable {
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
    runAsyncWorker(
        m_context, [runner = std::move(runner)]() { return runner->loadImageCandidates(); },
        [this, batch](ArchiveImageCandidatesResult result) mutable {
            finishCandidateLoad(batch, std::move(result));
        });
}

void MediaEntrySourceRuntime::finishCandidateLoad(
    MediaEntrySourceCandidateLoadBatch batch, ArchiveImageCandidatesResult result)
{
    std::vector<MediaEntrySourceCandidateLoad> pendingLoads
        = m_candidateLoadState.finishBatch(batch);

    for (const MediaEntrySourceCandidateLoad &load : pendingLoads) {
        load.completion.claimAndDelete([&]() {
            finishMediaEntrySourceCandidateResult(result, load.callback, load.errorCallback);
        });
    }
}

void MediaEntrySourceRuntime::cancelCandidateLoadBatch() { m_candidateLoadState.cancel(); }
}
