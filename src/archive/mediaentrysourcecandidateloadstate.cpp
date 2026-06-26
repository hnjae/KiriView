// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaentrysourcecandidateloadstate.h"

#include <QObject>
#include <utility>

namespace {
void cancelMediaEntrySourceCandidateLoadToken(QObject* object)
{
    if (object != nullptr) {
        object->deleteLater();
    }
}
}

namespace kiriview {
ImageIoJob MediaEntrySourceCandidateLoadState::addLoad(
    QObject* receiver, ImageDocumentPageCandidatesCallback callback, ErrorCallback errorCallback)
{
    QObject* token = new QObject(receiver);
    ImageIoJob job(token, cancelMediaEntrySourceCandidateLoadToken);
    m_pendingLoads.push_back(MediaEntrySourceCandidateLoad {
        job.completion(),
        std::move(callback),
        std::move(errorCallback),
    });
    return job;
}

std::optional<MediaEntrySourceCandidateLoadBatch> MediaEntrySourceCandidateLoadState::startBatch()
{
    if (m_batch.active() || m_pendingLoads.empty()) {
        return std::nullopt;
    }

    return MediaEntrySourceCandidateLoadBatch {
        m_batch.start(),
    };
}

bool MediaEntrySourceCandidateLoadState::acceptsBatch(
    MediaEntrySourceCandidateLoadBatch batch) const
{
    return m_batch.accepts(batch.operationId);
}

bool MediaEntrySourceCandidateLoadState::batchInProgress() const { return m_batch.active(); }

std::vector<MediaEntrySourceCandidateLoad> MediaEntrySourceCandidateLoadState::finishBatch(
    MediaEntrySourceCandidateLoadBatch batch)
{
    if (!m_batch.finish(batch.operationId)) {
        return {};
    }

    std::vector<MediaEntrySourceCandidateLoad> pendingLoads = std::move(m_pendingLoads);
    reset();
    return pendingLoads;
}

void MediaEntrySourceCandidateLoadState::cancel()
{
    for (const MediaEntrySourceCandidateLoad& load : m_pendingLoads) {
        load.completion.cancel();
    }
    reset();
}

void MediaEntrySourceCandidateLoadState::reset()
{
    m_pendingLoads.clear();
    m_batch.cancel();
}
}
