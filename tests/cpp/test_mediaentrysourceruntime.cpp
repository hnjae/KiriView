// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/mediaentrysourceruntime.h"

#include "async/imageworkerscheduler.h"
#include "image_test_support.h"
#include "media_entry_source_test_support.h"

#include <QByteArray>
#include <QTest>
#include <memory>
#include <optional>
#include <vector>

namespace {
using KiriView::ImageDocumentPageCandidate;
using KiriView::TestSupport::addInstrumentedMediaEntrySourceFixture;
using KiriView::TestSupport::archiveCollectionForLocalArchiveUrl;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::blockInstrumentedMediaEntrySourceCandidateLoads;
using KiriView::TestSupport::imageDocumentPageCandidate;
using KiriView::TestSupport::instrumentedMediaEntrySourceFactory;
using KiriView::TestSupport::InstrumentedMediaEntrySourceState;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::releaseInstrumentedMediaEntrySourceLoads;

struct ManualImageWorkerSchedule {
    KiriView::ImageWorkerOperation work;
    KiriView::ImageWorkerCompletion completion;
};

class ManualImageWorkerScheduler
{
public:
    KiriView::ImageWorkerScheduler scheduler()
    {
        return KiriView::ImageWorkerScheduler([this](QObject *, KiriView::ImageWorkerOperation work,
                                                  KiriView::ImageWorkerCompletion completion) {
            m_schedules.push_back(
                ManualImageWorkerSchedule { std::move(work), std::move(completion) });
        });
    }

    std::size_t scheduleCount() const { return m_schedules.size(); }

    void runWork(std::size_t index)
    {
        if (m_schedules.at(index).work) {
            m_schedules.at(index).work();
        }
    }

    void finish(std::size_t index)
    {
        if (m_schedules.at(index).completion) {
            m_schedules.at(index).completion();
        }
    }

private:
    std::vector<ManualImageWorkerSchedule> m_schedules;
};
}

class TestMediaEntrySourceRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void synchronousLoadsShareLazyOpenAndCandidateCache();
    void simultaneousCandidateLoadsSharePendingBatch();
    void candidateLoadAddedDuringActiveBatchSharesWorker();
    void candidateBatchCancellationPreventsStaleCallbacks();
    void dataCompletionAfterOpenedCollectionSwitchIsIgnored();
};

void TestMediaEntrySourceRuntime::synchronousLoadsShareLazyOpenAndCandidateCache()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    addInstrumentedMediaEntrySourceFixture(state, *archiveCollection,
        { imageDocumentPageCandidate(firstUrl), imageDocumentPageCandidate(secondUrl) });

    KiriView::MediaEntrySourceRuntime runtime(this, instrumentedMediaEntrySourceFactory(state));
    std::vector<ImageDocumentPageCandidate> firstCandidates;
    std::vector<ImageDocumentPageCandidate> cachedCandidates;
    QByteArray data;

    runtime.loadOpenedCollectionCandidates(nullptr, *archiveCollection,
        [&firstCandidates](auto loaded) { firstCandidates = std::move(loaded); }, {});
    runtime.loadOpenedCollectionCandidates(nullptr, *archiveCollection,
        [&cachedCandidates](auto loaded) { cachedCandidates = std::move(loaded); }, {});
    runtime.loadOpenedCollectionImageData(nullptr,
        KiriView::ImageDecodeRequest::fromLocation(1,
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                firstUrl, *archiveCollection)),
        [&data](QByteArray loaded) { data = std::move(loaded); }, {});

    QCOMPARE(firstCandidates.size(), std::size_t(2));
    QCOMPARE(cachedCandidates.size(), std::size_t(2));
    QCOMPARE(data, QByteArrayLiteral("image"));
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 1);
}

void TestMediaEntrySourceRuntime::simultaneousCandidateLoadsSharePendingBatch()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());
    const QUrl pageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedMediaEntrySourceFixture(
        state, *archiveCollection, { imageDocumentPageCandidate(pageUrl) });
    blockInstrumentedMediaEntrySourceCandidateLoads(state);

    KiriView::MediaEntrySourceRuntime runtime(this, instrumentedMediaEntrySourceFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = runtime.loadOpenedCollectionCandidates(this, *archiveCollection,
        [&callbackCount](std::vector<ImageDocumentPageCandidate>) { ++callbackCount; }, {});
    KiriView::ImageIoJob secondJob
        = runtime.loadOpenedCollectionCandidates(this, *archiveCollection,
            [&callbackCount](std::vector<ImageDocumentPageCandidate>) { ++callbackCount; }, {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    releaseInstrumentedMediaEntrySourceLoads(state);

    QTRY_COMPARE(callbackCount, 2);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestMediaEntrySourceRuntime::candidateLoadAddedDuringActiveBatchSharesWorker()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());
    const QUrl pageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedMediaEntrySourceFixture(
        state, *archiveCollection, { imageDocumentPageCandidate(pageUrl) });

    ManualImageWorkerScheduler workerScheduler;
    KiriView::MediaEntrySourceRuntime runtime(
        this, instrumentedMediaEntrySourceFactory(state), workerScheduler.scheduler());
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = runtime.loadOpenedCollectionCandidates(this, *archiveCollection,
        [&callbackCount](std::vector<ImageDocumentPageCandidate>) { ++callbackCount; }, {});

    KiriView::ImageIoJob secondJob
        = runtime.loadOpenedCollectionCandidates(this, *archiveCollection,
            [&callbackCount](std::vector<ImageDocumentPageCandidate>) { ++callbackCount; }, {});

    QCOMPARE(workerScheduler.scheduleCount(), std::size_t(1));
    QCOMPARE(state->candidateLoadCount.load(), 0);
    QCOMPARE(callbackCount, 0);
    QVERIFY(firstJob.isActive());
    QVERIFY(secondJob.isActive());

    workerScheduler.runWork(0);

    QCOMPARE(state->candidateLoadCount.load(), 1);
    QCOMPARE(callbackCount, 0);
    QVERIFY(firstJob.isActive());
    QVERIFY(secondJob.isActive());

    workerScheduler.finish(0);

    QCOMPARE(callbackCount, 2);
    QVERIFY(!firstJob.isActive());
    QVERIFY(!secondJob.isActive());
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestMediaEntrySourceRuntime::candidateBatchCancellationPreventsStaleCallbacks()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> firstArchiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/a.cbz")));
    const std::optional<KiriView::OpenedCollectionScopeLocation> secondArchiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/b.cbz")));
    QVERIFY(firstArchiveCollection.has_value());
    QVERIFY(secondArchiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedMediaEntrySourceFixture(
        state, *firstArchiveCollection, { imageDocumentPageCandidate(firstPageUrl) });
    addInstrumentedMediaEntrySourceFixture(
        state, *secondArchiveCollection, { imageDocumentPageCandidate(secondPageUrl) });
    blockInstrumentedMediaEntrySourceCandidateLoads(state);

    KiriView::MediaEntrySourceRuntime runtime(this, instrumentedMediaEntrySourceFactory(state));
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = runtime.loadOpenedCollectionCandidates(this,
        *firstArchiveCollection,
        [&staleCallbackCount](std::vector<ImageDocumentPageCandidate>) { ++staleCallbackCount; },
        {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    runtime.switchToOpenedCollectionScope(*secondArchiveCollection);
    releaseInstrumentedMediaEntrySourceLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(runtime.hasCurrentOpenedCollectionScope(*secondArchiveCollection));
}

void TestMediaEntrySourceRuntime::dataCompletionAfterOpenedCollectionSwitchIsIgnored()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> firstArchiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/a.cbz")));
    const std::optional<KiriView::OpenedCollectionScopeLocation> secondArchiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/b.cbz")));
    QVERIFY(firstArchiveCollection.has_value());
    QVERIFY(secondArchiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedMediaEntrySourceFixture(
        state, *firstArchiveCollection, { imageDocumentPageCandidate(firstPageUrl) });
    addInstrumentedMediaEntrySourceFixture(
        state, *secondArchiveCollection, { imageDocumentPageCandidate(secondPageUrl) });

    ManualImageWorkerScheduler workerScheduler;
    KiriView::MediaEntrySourceRuntime runtime(
        this, instrumentedMediaEntrySourceFactory(state), workerScheduler.scheduler());
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = runtime.loadOpenedCollectionImageData(this,
        KiriView::ImageDecodeRequest::fromLocation(1,
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                firstPageUrl, *firstArchiveCollection)),
        [&staleCallbackCount](QByteArray) { ++staleCallbackCount; }, {});

    QCOMPARE(workerScheduler.scheduleCount(), std::size_t(1));
    QCOMPARE(state->dataLoadCount.load(), 0);
    QVERIFY(staleJob.isActive());

    runtime.switchToOpenedCollectionScope(*secondArchiveCollection);
    workerScheduler.runWork(0);

    QCOMPARE(state->dataLoadCount.load(), 1);
    QCOMPARE(staleCallbackCount, 0);

    workerScheduler.finish(0);

    QVERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(runtime.hasCurrentOpenedCollectionScope(*secondArchiveCollection));
}

QTEST_GUILESS_MAIN(TestMediaEntrySourceRuntime)

#include "test_mediaentrysourceruntime.moc"
