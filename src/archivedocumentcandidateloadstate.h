// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEDOCUMENTCANDIDATELOADSTATE_H
#define KIRIVIEW_ARCHIVEDOCUMENTCANDIDATELOADSTATE_H

#include "imageasyncdependencies.h"
#include "imageiojob.h"

#include <QtGlobal>
#include <vector>

class QObject;

namespace KiriView {
struct ArchiveDocumentCandidateLoad {
    ImageIoJobCompletion completion;
    ImageCandidatesCallback callback;
    ErrorCallback errorCallback;
};

class ArchiveDocumentCandidateLoadState final
{
public:
    ImageIoJob addLoad(QObject *receiver, quint64 generation, ImageCandidatesCallback callback,
        ErrorCallback errorCallback);
    bool startBatch(quint64 generation);
    bool acceptsBatch(quint64 generation) const;
    std::vector<ArchiveDocumentCandidateLoad> finishBatch(quint64 generation);
    void cancel();

private:
    void ensureGeneration(quint64 generation);
    void reset();

    std::vector<ArchiveDocumentCandidateLoad> m_pendingLoads;
    quint64 m_generation = 0;
    bool m_hasGeneration = false;
    bool m_inProgress = false;
};
}

#endif
