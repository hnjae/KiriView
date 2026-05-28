// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_TESTS_MEDIA_ENTRY_SOURCE_TEST_SUPPORT_H
#define KIRIVIEW_TESTS_MEDIA_ENTRY_SOURCE_TEST_SUPPORT_H

#include "archive/archivebackend.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"

#include <QByteArray>
#include <QString>
#include <QUrl>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

namespace KiriView::TestSupport {
struct InstrumentedArchiveFixture {
    std::vector<ImageNavigationCandidate> candidates;
    std::map<QString, QByteArray> dataByUrl;
};

struct InstrumentedMediaEntrySourceState {
    std::atomic<int> openCount = 0;
    std::atomic<int> candidateLoadCount = 0;
    std::atomic<int> dataLoadCount = 0;
    std::atomic<int> waitingCandidateLoadCount = 0;
    std::atomic<int> waitingDataLoadCount = 0;
    std::mutex mutex;
    std::mutex loadBlockMutex;
    std::condition_variable loadBlockChanged;
    std::map<QString, InstrumentedArchiveFixture> fixturesByRootUrl;
    bool blockCandidateLoads = false;
    bool blockDataLoads = false;
    bool releaseLoads = false;
};

class InstrumentedMediaEntrySource final : public MediaEntrySource
{
public:
    InstrumentedMediaEntrySource(OpenedCollectionScopeLocation archiveCollection,
        std::shared_ptr<InstrumentedMediaEntrySourceState> state)
        : m_archiveCollection(std::move(archiveCollection))
        , m_state(std::move(state))
    {
    }

    ArchiveImageCandidatesResult loadImageCandidates() override
    {
        ++m_state->candidateLoadCount;
        waitIfBlocked(InstrumentedArchiveLoadKind::Candidate);
        std::lock_guard<std::mutex> lock(m_state->mutex);
        return ArchiveImageCandidates {
            fixture().candidates,
        };
    }

    ArchiveImageDataResult loadImageData(const QUrl &imageUrl) override
    {
        ++m_state->dataLoadCount;
        waitIfBlocked(InstrumentedArchiveLoadKind::Data);

        std::lock_guard<std::mutex> lock(m_state->mutex);
        const auto data = fixture().dataByUrl.find(keyForUrl(imageUrl));
        if (data == fixture().dataByUrl.cend()) {
            return ArchiveError { QStringLiteral("missing fake archive image data") };
        }

        return ArchiveImageData { data->second };
    }

private:
    enum class InstrumentedArchiveLoadKind {
        Candidate,
        Data,
    };

    void waitIfBlocked(InstrumentedArchiveLoadKind kind)
    {
        std::unique_lock<std::mutex> lock(m_state->loadBlockMutex);
        const bool blocked = kind == InstrumentedArchiveLoadKind::Candidate
            ? m_state->blockCandidateLoads
            : m_state->blockDataLoads;
        if (!blocked || m_state->releaseLoads) {
            return;
        }

        std::atomic<int> &waitingCount = kind == InstrumentedArchiveLoadKind::Candidate
            ? m_state->waitingCandidateLoadCount
            : m_state->waitingDataLoadCount;
        ++waitingCount;
        m_state->loadBlockChanged.notify_all();
        m_state->loadBlockChanged.wait(lock, [this]() { return m_state->releaseLoads; });
    }

    const InstrumentedArchiveFixture &fixture() const
    {
        return m_state->fixturesByRootUrl.at(keyForUrl(m_archiveCollection.rootUrl()));
    }

    OpenedCollectionScopeLocation m_archiveCollection;
    std::shared_ptr<InstrumentedMediaEntrySourceState> m_state;
};

inline MediaEntrySourceFactory instrumentedMediaEntrySourceFactory(
    std::shared_ptr<InstrumentedMediaEntrySourceState> state)
{
    return
        [state = std::move(state)](
            const OpenedCollectionScopeLocation &archiveCollection) -> MediaEntrySourceOpenResult {
            ++state->openCount;
            std::lock_guard<std::mutex> lock(state->mutex);
            if (!state->fixturesByRootUrl.count(keyForUrl(archiveCollection.rootUrl()))) {
                return ArchiveError { QStringLiteral("missing fake archive fixture") };
            }

            return MediaEntrySourcePtr(
                std::make_shared<InstrumentedMediaEntrySource>(archiveCollection, state));
        };
}

inline void blockInstrumentedArchiveCandidateLoads(
    const std::shared_ptr<InstrumentedMediaEntrySourceState> &state)
{
    std::lock_guard<std::mutex> lock(state->loadBlockMutex);
    state->blockCandidateLoads = true;
    state->releaseLoads = false;
}

inline void blockInstrumentedArchiveDataLoads(
    const std::shared_ptr<InstrumentedMediaEntrySourceState> &state)
{
    std::lock_guard<std::mutex> lock(state->loadBlockMutex);
    state->blockDataLoads = true;
    state->releaseLoads = false;
}

inline void releaseInstrumentedArchiveLoads(
    const std::shared_ptr<InstrumentedMediaEntrySourceState> &state)
{
    {
        std::lock_guard<std::mutex> lock(state->loadBlockMutex);
        state->releaseLoads = true;
    }
    state->loadBlockChanged.notify_all();
}

inline std::optional<OpenedCollectionScopeLocation> archiveCollectionForLocalArchiveUrl(
    const QUrl &archiveUrl)
{
    return openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
}

inline void addInstrumentedArchiveFixture(std::shared_ptr<InstrumentedMediaEntrySourceState> state,
    const OpenedCollectionScopeLocation &archiveCollection,
    std::vector<ImageNavigationCandidate> candidates)
{
    InstrumentedArchiveFixture fixture;
    fixture.candidates = std::move(candidates);
    for (const ImageNavigationCandidate &candidate : fixture.candidates) {
        fixture.dataByUrl[keyForUrl(candidate.url)] = QByteArrayLiteral("image");
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    state->fixturesByRootUrl[keyForUrl(archiveCollection.rootUrl())] = std::move(fixture);
}
}

#endif
