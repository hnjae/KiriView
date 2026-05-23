// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivedocumentcandidateloadstate.h"

#include <QObject>
#include <utility>

namespace {
void cancelArchiveCandidateLoadToken(QObject *object)
{
    if (object != nullptr) {
        object->deleteLater();
    }
}
}

namespace KiriView {
ImageIoJob ArchiveDocumentCandidateLoadState::addLoad(
    QObject *receiver, ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    QObject *token = new QObject(receiver);
    ImageIoJob job(token, cancelArchiveCandidateLoadToken);
    m_pendingLoads.push_back(ArchiveDocumentCandidateLoad {
        job.completion(),
        std::move(callback),
        std::move(errorCallback),
    });
    return job;
}

std::optional<ArchiveDocumentCandidateLoadBatch> ArchiveDocumentCandidateLoadState::startBatch()
{
    if (m_batch.active() || m_pendingLoads.empty()) {
        return std::nullopt;
    }

    return ArchiveDocumentCandidateLoadBatch {
        m_batch.start(),
    };
}

bool ArchiveDocumentCandidateLoadState::acceptsBatch(ArchiveDocumentCandidateLoadBatch batch) const
{
    return m_batch.accepts(batch.operationId);
}

bool ArchiveDocumentCandidateLoadState::batchInProgress() const { return m_batch.active(); }

std::vector<ArchiveDocumentCandidateLoad> ArchiveDocumentCandidateLoadState::finishBatch(
    ArchiveDocumentCandidateLoadBatch batch)
{
    if (!m_batch.finish(batch.operationId)) {
        return {};
    }

    std::vector<ArchiveDocumentCandidateLoad> pendingLoads = std::move(m_pendingLoads);
    reset();
    return pendingLoads;
}

void ArchiveDocumentCandidateLoadState::cancel()
{
    for (const ArchiveDocumentCandidateLoad &load : m_pendingLoads) {
        load.completion.cancel();
    }
    reset();
}

void ArchiveDocumentCandidateLoadState::reset()
{
    m_pendingLoads.clear();
    m_batch.cancel();
}
}
