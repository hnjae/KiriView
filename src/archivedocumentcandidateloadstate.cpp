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
ImageIoJob ArchiveDocumentCandidateLoadState::addLoad(QObject *receiver, quint64 generation,
    ImageCandidatesCallback callback, ErrorCallback errorCallback)
{
    ensureGeneration(generation);

    QObject *token = new QObject(receiver);
    ImageIoJob job(token, cancelArchiveCandidateLoadToken);
    m_pendingLoads.push_back(ArchiveDocumentCandidateLoad {
        job.completion(),
        std::move(callback),
        std::move(errorCallback),
    });
    return job;
}

bool ArchiveDocumentCandidateLoadState::startBatch(quint64 generation)
{
    if (!acceptsBatch(generation) || m_inProgress || m_pendingLoads.empty()) {
        return false;
    }

    m_inProgress = true;
    return true;
}

bool ArchiveDocumentCandidateLoadState::acceptsBatch(quint64 generation) const
{
    return m_hasGeneration && m_generation == generation;
}

std::vector<ArchiveDocumentCandidateLoad> ArchiveDocumentCandidateLoadState::finishBatch(
    quint64 generation)
{
    if (!acceptsBatch(generation)) {
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

void ArchiveDocumentCandidateLoadState::ensureGeneration(quint64 generation)
{
    if (!m_hasGeneration || m_generation == generation) {
        m_generation = generation;
        m_hasGeneration = true;
        return;
    }

    cancel();
    m_generation = generation;
    m_hasGeneration = true;
}

void ArchiveDocumentCandidateLoadState::reset()
{
    m_pendingLoads.clear();
    m_hasGeneration = false;
    m_inProgress = false;
}
}
