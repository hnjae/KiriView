// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivedocumentsessionruntime.h"

#include "image_test_support.h"
#include "imagecontainer.h"

#include <QTest>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace {
using KiriView::ImageNavigationCandidate;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::keyForUrl;
using KiriView::TestSupport::localUrl;

struct ArchiveFixture {
    std::vector<ImageNavigationCandidate> candidates;
    std::map<QString, QByteArray> dataByUrl;
};

struct InstrumentedArchiveSessionState {
    std::atomic<int> openCount = 0;
    std::atomic<int> candidateLoadCount = 0;
    std::atomic<int> dataLoadCount = 0;
    std::atomic<int> waitingCandidateLoadCount = 0;
    std::atomic<int> waitingDataLoadCount = 0;
    std::mutex mutex;
    std::mutex loadBlockMutex;
    std::condition_variable loadBlockChanged;
    std::map<QString, ArchiveFixture> fixturesByRootUrl;
    bool blockCandidateLoads = false;
    bool blockDataLoads = false;
    bool releaseLoads = false;
};

class InstrumentedArchiveSession final : public KiriView::ArchiveDocumentSession
{
public:
    InstrumentedArchiveSession(KiriView::ArchiveDocumentLocation archiveDocument,
        std::shared_ptr<InstrumentedArchiveSessionState> state)
        : m_archiveDocument(std::move(archiveDocument))
        , m_state(std::move(state))
    {
    }

    KiriView::ArchiveImageCandidatesResult loadImageCandidates() override
    {
        ++m_state->candidateLoadCount;
        waitIfBlocked(m_state->blockCandidateLoads, m_state->waitingCandidateLoadCount);
        std::lock_guard<std::mutex> lock(m_state->mutex);
        return KiriView::ArchiveImageCandidates {
            fixture().candidates,
        };
    }

    KiriView::ArchiveImageDataResult loadImageData(const QUrl &imageUrl) override
    {
        ++m_state->dataLoadCount;
        waitIfBlocked(m_state->blockDataLoads, m_state->waitingDataLoadCount);

        std::lock_guard<std::mutex> lock(m_state->mutex);
        const auto data = fixture().dataByUrl.find(keyForUrl(imageUrl));
        if (data == fixture().dataByUrl.cend()) {
            return KiriView::ArchiveError { QStringLiteral("missing fake archive image data") };
        }

        return KiriView::ArchiveImageData { data->second };
    }

private:
    void waitIfBlocked(bool blocked, std::atomic<int> &waitingCount)
    {
        std::unique_lock<std::mutex> lock(m_state->loadBlockMutex);
        if (blocked && !m_state->releaseLoads) {
            ++waitingCount;
            m_state->loadBlockChanged.notify_all();
            m_state->loadBlockChanged.wait(lock, [this]() { return m_state->releaseLoads; });
        }
    }

    const ArchiveFixture &fixture() const
    {
        return m_state->fixturesByRootUrl.at(keyForUrl(m_archiveDocument.rootUrl()));
    }

    KiriView::ArchiveDocumentLocation m_archiveDocument;
    std::shared_ptr<InstrumentedArchiveSessionState> m_state;
};

KiriView::ArchiveDocumentSessionFactory instrumentedSessionFactory(
    std::shared_ptr<InstrumentedArchiveSessionState> state)
{
    return [state = std::move(state)](const KiriView::ArchiveDocumentLocation &archiveDocument)
               -> KiriView::ArchiveDocumentSessionOpenResult {
        ++state->openCount;
        std::lock_guard<std::mutex> lock(state->mutex);
        if (!state->fixturesByRootUrl.count(keyForUrl(archiveDocument.rootUrl()))) {
            return KiriView::ArchiveError { QStringLiteral("missing fake archive fixture") };
        }

        return KiriView::ArchiveDocumentSessionPtr(
            std::make_shared<InstrumentedArchiveSession>(archiveDocument, state));
    };
}

void blockCandidateLoads(const std::shared_ptr<InstrumentedArchiveSessionState> &state)
{
    std::lock_guard<std::mutex> lock(state->loadBlockMutex);
    state->blockCandidateLoads = true;
    state->releaseLoads = false;
}

void blockDataLoads(const std::shared_ptr<InstrumentedArchiveSessionState> &state)
{
    std::lock_guard<std::mutex> lock(state->loadBlockMutex);
    state->blockDataLoads = true;
    state->releaseLoads = false;
}

void releaseLoads(const std::shared_ptr<InstrumentedArchiveSessionState> &state)
{
    {
        std::lock_guard<std::mutex> lock(state->loadBlockMutex);
        state->releaseLoads = true;
    }
    state->loadBlockChanged.notify_all();
}

std::optional<KiriView::ArchiveDocumentLocation> archiveDocumentFor(const QUrl &archiveUrl)
{
    return KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
}

void addArchiveFixture(std::shared_ptr<InstrumentedArchiveSessionState> state,
    const KiriView::ArchiveDocumentLocation &archiveDocument,
    std::vector<ImageNavigationCandidate> candidates)
{
    ArchiveFixture fixture;
    fixture.candidates = std::move(candidates);
    for (const ImageNavigationCandidate &candidate : fixture.candidates) {
        fixture.dataByUrl[keyForUrl(candidate.url)] = QByteArrayLiteral("image");
    }

    std::lock_guard<std::mutex> lock(state->mutex);
    state->fixturesByRootUrl[keyForUrl(archiveDocument.rootUrl())] = std::move(fixture);
}
}

class TestArchiveDocumentSessionRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void synchronousLoadsShareLazyOpenAndCandidateCache();
    void simultaneousCandidateLoadsSharePendingBatch();
    void candidateBatchCancellationPreventsStaleCallbacks();
    void dataCompletionAfterArchiveSwitchIsIgnored();
};

void TestArchiveDocumentSessionRuntime::synchronousLoadsShareLazyOpenAndCandidateCache()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentFor(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    addArchiveFixture(
        state, *archiveDocument, { imageCandidate(firstUrl), imageCandidate(secondUrl) });

    KiriView::ArchiveDocumentSessionRuntime runtime(this, instrumentedSessionFactory(state));
    std::vector<ImageNavigationCandidate> firstCandidates;
    std::vector<ImageNavigationCandidate> cachedCandidates;
    QByteArray data;

    runtime.loadArchiveImages(nullptr, *archiveDocument,
        [&firstCandidates](auto loaded) { firstCandidates = std::move(loaded); }, {});
    runtime.loadArchiveImages(nullptr, *archiveDocument,
        [&cachedCandidates](auto loaded) { cachedCandidates = std::move(loaded); }, {});
    runtime.loadArchiveImageData(nullptr,
        KiriView::ImageDecodeRequest::fromLocation(
            1, KiriView::DisplayedImageLocation::fromArchiveDocument(firstUrl, *archiveDocument)),
        [&data](QByteArray loaded) { data = std::move(loaded); }, {});

    QCOMPARE(firstCandidates.size(), std::size_t(2));
    QCOMPARE(cachedCandidates.size(), std::size_t(2));
    QCOMPARE(data, QByteArrayLiteral("image"));
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 1);
}

void TestArchiveDocumentSessionRuntime::simultaneousCandidateLoadsSharePendingBatch()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentFor(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    addArchiveFixture(state, *archiveDocument, { imageCandidate(pageUrl) });
    blockCandidateLoads(state);

    KiriView::ArchiveDocumentSessionRuntime runtime(this, instrumentedSessionFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = runtime.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    KiriView::ImageIoJob secondJob = runtime.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    releaseLoads(state);

    QTRY_COMPARE(callbackCount, 2);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestArchiveDocumentSessionRuntime::candidateBatchCancellationPreventsStaleCallbacks()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ArchiveDocumentLocation> firstArchiveDocument
        = archiveDocumentFor(localUrl(QStringLiteral("/books/a.cbz")));
    const std::optional<KiriView::ArchiveDocumentLocation> secondArchiveDocument
        = archiveDocumentFor(localUrl(QStringLiteral("/books/b.cbz")));
    QVERIFY(firstArchiveDocument.has_value());
    QVERIFY(secondArchiveDocument.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    addArchiveFixture(state, *firstArchiveDocument, { imageCandidate(firstPageUrl) });
    addArchiveFixture(state, *secondArchiveDocument, { imageCandidate(secondPageUrl) });
    blockCandidateLoads(state);

    KiriView::ArchiveDocumentSessionRuntime runtime(this, instrumentedSessionFactory(state));
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = runtime.loadArchiveImages(this, *firstArchiveDocument,
        [&staleCallbackCount](std::vector<ImageNavigationCandidate>) { ++staleCallbackCount; }, {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    runtime.switchToArchiveDocument(*secondArchiveDocument);
    releaseLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(runtime.hasCurrentArchiveDocument(*secondArchiveDocument));
}

void TestArchiveDocumentSessionRuntime::dataCompletionAfterArchiveSwitchIsIgnored()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ArchiveDocumentLocation> firstArchiveDocument
        = archiveDocumentFor(localUrl(QStringLiteral("/books/a.cbz")));
    const std::optional<KiriView::ArchiveDocumentLocation> secondArchiveDocument
        = archiveDocumentFor(localUrl(QStringLiteral("/books/b.cbz")));
    QVERIFY(firstArchiveDocument.has_value());
    QVERIFY(secondArchiveDocument.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    addArchiveFixture(state, *firstArchiveDocument, { imageCandidate(firstPageUrl) });
    addArchiveFixture(state, *secondArchiveDocument, { imageCandidate(secondPageUrl) });
    blockDataLoads(state);

    KiriView::ArchiveDocumentSessionRuntime runtime(this, instrumentedSessionFactory(state));
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = runtime.loadArchiveImageData(this,
        KiriView::ImageDecodeRequest::fromLocation(1,
            KiriView::DisplayedImageLocation::fromArchiveDocument(
                firstPageUrl, *firstArchiveDocument)),
        [&staleCallbackCount](QByteArray) { ++staleCallbackCount; }, {});
    QTRY_COMPARE(state->waitingDataLoadCount.load(), 1);

    runtime.switchToArchiveDocument(*secondArchiveDocument);
    releaseLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(runtime.hasCurrentArchiveDocument(*secondArchiveDocument));
}

QTEST_GUILESS_MAIN(TestArchiveDocumentSessionRuntime)

#include "test_archivedocumentsessionruntime.moc"
