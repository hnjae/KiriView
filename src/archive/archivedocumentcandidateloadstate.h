// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEDOCUMENTCANDIDATELOADSTATE_H
#define KIRIVIEW_ARCHIVEDOCUMENTCANDIDATELOADSTATE_H

#include "async/imageasynccallbacks.h"
#include "async/imageasyncoperationstate.h"
#include "async/imageiojob.h"
#include "navigation/imagecandidatecallbacks.h"

#include <QtGlobal>
#include <optional>
#include <vector>

class QObject;

namespace KiriView {
struct ArchiveDocumentCandidateLoad {
    ImageIoJobCompletion completion;
    ImageCandidatesCallback callback;
    ErrorCallback errorCallback;
};

struct ArchiveDocumentCandidateLoadBatch {
    quint64 operationId = 0;
};

class ArchiveDocumentCandidateLoadState final
{
public:
    ImageIoJob addLoad(
        QObject *receiver, ImageCandidatesCallback callback, ErrorCallback errorCallback);
    std::optional<ArchiveDocumentCandidateLoadBatch> startBatch();
    bool acceptsBatch(ArchiveDocumentCandidateLoadBatch batch) const;
    bool batchInProgress() const;
    std::vector<ArchiveDocumentCandidateLoad> finishBatch(ArchiveDocumentCandidateLoadBatch batch);
    void cancel();

private:
    void reset();

    std::vector<ArchiveDocumentCandidateLoad> m_pendingLoads;
    ImageAsyncOperationState m_batch;
};
}

#endif
