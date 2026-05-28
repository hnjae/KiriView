// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAENTRYSOURCECANDIDATELOADSTATE_H
#define KIRIVIEW_MEDIAENTRYSOURCECANDIDATELOADSTATE_H

#include "async/imageasynccallbacks.h"
#include "async/imageasyncoperationstate.h"
#include "async/imageiojob.h"
#include "navigation/imagecandidatecallbacks.h"

#include <QtGlobal>
#include <optional>
#include <vector>

class QObject;

namespace KiriView {
struct MediaEntrySourceCandidateLoad {
    ImageIoJobCompletion completion;
    ImageCandidatesCallback callback;
    ErrorCallback errorCallback;
};

struct MediaEntrySourceCandidateLoadBatch {
    quint64 operationId = 0;
};

class MediaEntrySourceCandidateLoadState final
{
public:
    ImageIoJob addLoad(
        QObject *receiver, ImageCandidatesCallback callback, ErrorCallback errorCallback);
    std::optional<MediaEntrySourceCandidateLoadBatch> startBatch();
    bool acceptsBatch(MediaEntrySourceCandidateLoadBatch batch) const;
    bool batchInProgress() const;
    std::vector<MediaEntrySourceCandidateLoad> finishBatch(
        MediaEntrySourceCandidateLoadBatch batch);
    void cancel();

private:
    void reset();

    std::vector<MediaEntrySourceCandidateLoad> m_pendingLoads;
    ImageAsyncOperationState m_batch;
};
}

#endif
