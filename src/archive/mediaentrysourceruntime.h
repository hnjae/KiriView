// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCERUNTIME_H
#define KIRIVIEW_MEDIAENTRYSOURCERUNTIME_H

#include "async/imageasyncticket.h"
#include "async/imageiojob.h"
#include "async/imageworkerscheduler.h"
#include "decoding/imagedecodedependencies.h"
#include "mediaentrysourcebackend.h"
#include "mediaentrysourcecandidateloadstate.h"

#include <memory>

class QObject;

namespace KiriView {
class MediaEntrySourceRunner;

class MediaEntrySourceRuntime final
{
public:
    explicit MediaEntrySourceRuntime(QObject *context, MediaEntrySourceFactory sourceFactory = {},
        ImageWorkerScheduler workerScheduler = {});
    ~MediaEntrySourceRuntime();

    void clear();
    void switchToOpenedCollectionScope(OpenedCollectionScopeLocation openedCollectionScope);

    bool hasCurrentOpenedCollectionScope() const;
    bool hasCurrentOpenedCollectionScope(
        const OpenedCollectionScopeLocation &openedCollectionScope) const;

    ImageIoJob loadOpenedCollectionCandidates(QObject *receiver,
        OpenedCollectionScopeLocation openedCollectionScope,
        ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback);
    ImageIoJob loadOpenedCollectionImageData(QObject *receiver, ImageDecodeRequest request,
        ImageDataCallback callback, ErrorCallback errorCallback);

private:
    void startCandidateLoad(MediaEntrySourceCandidateLoadBatch batch);
    void finishCandidateLoad(
        MediaEntrySourceCandidateLoadBatch batch, MediaEntrySourceCandidatesResult result);
    void cancelCandidateLoadBatch();

    QObject *m_context = nullptr;
    MediaEntrySourceFactory m_sourceFactory;
    ImageWorkerScheduler m_workerScheduler;
    std::shared_ptr<MediaEntrySourceRunner> m_runner;
    MediaEntrySourceCandidateLoadState m_candidateLoadState;
    ImageAsyncTicket m_sourceGeneration;
};
}

#endif
