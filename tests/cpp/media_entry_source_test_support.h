// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_TESTS_MEDIA_ENTRY_SOURCE_TEST_SUPPORT_H
#define KIRIVIEW_TESTS_MEDIA_ENTRY_SOURCE_TEST_SUPPORT_H

#include "archive/mediaentrysourcebackend.h"
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

namespace kiriview::TestSupport {
struct InstrumentedMediaEntrySourceFixture {
    std::vector<ImageDocumentPageCandidate> candidates;
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
    std::map<QString, InstrumentedMediaEntrySourceFixture> fixturesByRootUrl;
    bool blockCandidateLoads = false;
    bool blockDataLoads = false;
    bool releaseLoads = false;
};

class InstrumentedMediaEntrySource final : public MediaEntrySource
{
public:
    InstrumentedMediaEntrySource(OpenedCollectionScopeLocation openedCollectionScope,
        std::shared_ptr<InstrumentedMediaEntrySourceState> state)
        : m_openedCollectionScope(std::move(openedCollectionScope))
        , m_state(std::move(state))
    {
    }

    MediaEntrySourceCandidatesResult loadImageDocumentPageCandidates() override
    {
        ++m_state->candidateLoadCount;
        waitIfBlocked(InstrumentedMediaEntrySourceLoadKind::Candidate);
        std::lock_guard<std::mutex> lock(m_state->mutex);
        return MediaEntrySourceCandidates {
            fixture().candidates,
        };
    }

    MediaEntrySourceImageDataResult loadImageData(const QUrl &imageUrl) override
    {
        ++m_state->dataLoadCount;
        waitIfBlocked(InstrumentedMediaEntrySourceLoadKind::Data);

        std::lock_guard<std::mutex> lock(m_state->mutex);
        const auto data = fixture().dataByUrl.find(keyForUrl(imageUrl));
        if (data == fixture().dataByUrl.cend()) {
            return MediaEntrySourceError { MediaEntrySourceBackendKind::Unknown,
                MediaEntrySourceOperation::ReadImageData, m_openedCollectionScope.fileUrl(),
                imageUrl.toString(), QStringLiteral("missing fake media entry source image data"),
                QStringLiteral("missing fake media entry source image data") };
        }

        return MediaEntrySourceImageData { data->second };
    }

private:
    enum class InstrumentedMediaEntrySourceLoadKind {
        Candidate,
        Data,
    };

    void waitIfBlocked(InstrumentedMediaEntrySourceLoadKind kind)
    {
        std::unique_lock<std::mutex> lock(m_state->loadBlockMutex);
        const bool blocked = kind == InstrumentedMediaEntrySourceLoadKind::Candidate
            ? m_state->blockCandidateLoads
            : m_state->blockDataLoads;
        if (!blocked || m_state->releaseLoads) {
            return;
        }

        std::atomic<int> &waitingCount = kind == InstrumentedMediaEntrySourceLoadKind::Candidate
            ? m_state->waitingCandidateLoadCount
            : m_state->waitingDataLoadCount;
        ++waitingCount;
        m_state->loadBlockChanged.notify_all();
        m_state->loadBlockChanged.wait(lock, [this]() { return m_state->releaseLoads; });
    }

    const InstrumentedMediaEntrySourceFixture &fixture() const
    {
        return m_state->fixturesByRootUrl.at(keyForUrl(m_openedCollectionScope.rootUrl()));
    }

    OpenedCollectionScopeLocation m_openedCollectionScope;
    std::shared_ptr<InstrumentedMediaEntrySourceState> m_state;
};

inline MediaEntrySourceFactory instrumentedMediaEntrySourceFactory(
    std::shared_ptr<InstrumentedMediaEntrySourceState> state)
{
    return [state = std::move(state)](const OpenedCollectionScopeLocation &openedCollectionScope)
               -> MediaEntrySourceOpenResult {
        ++state->openCount;
        std::lock_guard<std::mutex> lock(state->mutex);
        if (!state->fixturesByRootUrl.count(keyForUrl(openedCollectionScope.rootUrl()))) {
            return MediaEntrySourceError { MediaEntrySourceBackendKind::Unknown,
                MediaEntrySourceOperation::OpenCollection, openedCollectionScope.fileUrl(), {},
                QStringLiteral("missing fake media entry source fixture"),
                QStringLiteral("missing fake media entry source fixture") };
        }

        return MediaEntrySourcePtr(
            std::make_shared<InstrumentedMediaEntrySource>(openedCollectionScope, state));
    };
}

inline void blockInstrumentedMediaEntrySourceCandidateLoads(
    const std::shared_ptr<InstrumentedMediaEntrySourceState> &state)
{
    std::lock_guard<std::mutex> lock(state->loadBlockMutex);
    state->blockCandidateLoads = true;
    state->releaseLoads = false;
}

inline void blockInstrumentedMediaEntrySourceDataLoads(
    const std::shared_ptr<InstrumentedMediaEntrySourceState> &state)
{
    std::lock_guard<std::mutex> lock(state->loadBlockMutex);
    state->blockDataLoads = true;
    state->releaseLoads = false;
}

inline void releaseInstrumentedMediaEntrySourceLoads(
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

inline void addInstrumentedMediaEntrySourceFixture(
    std::shared_ptr<InstrumentedMediaEntrySourceState> state,
    const OpenedCollectionScopeLocation &openedCollectionScope,
    std::vector<ImageDocumentPageCandidate> candidates)
{
    InstrumentedMediaEntrySourceFixture fixture;
    fixture.candidates = std::move(candidates);
    for (const ImageDocumentPageCandidate &candidate : fixture.candidates) {
        fixture.dataByUrl[keyForUrl(candidate.url)] = QByteArrayLiteral("image");
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    state->fixturesByRootUrl[keyForUrl(openedCollectionScope.rootUrl())] = std::move(fixture);
}
}

#endif
