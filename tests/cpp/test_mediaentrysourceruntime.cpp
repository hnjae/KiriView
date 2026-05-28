// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/mediaentrysourceruntime.h"

#include "image_test_support.h"
#include "media_entry_source_test_support.h"

#include <QByteArray>
#include <QTest>
#include <memory>
#include <optional>
#include <vector>

namespace {
using KiriView::ImageNavigationCandidate;
using KiriView::TestSupport::addInstrumentedArchiveFixture;
using KiriView::TestSupport::archiveCollectionForLocalArchiveUrl;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::blockInstrumentedArchiveCandidateLoads;
using KiriView::TestSupport::blockInstrumentedArchiveDataLoads;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::instrumentedMediaEntrySourceFactory;
using KiriView::TestSupport::InstrumentedMediaEntrySourceState;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::releaseInstrumentedArchiveLoads;
}

class TestMediaEntrySourceRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void synchronousLoadsShareLazyOpenAndCandidateCache();
    void simultaneousCandidateLoadsSharePendingBatch();
    void candidateLoadAddedDuringActiveBatchSharesWorker();
    void candidateBatchCancellationPreventsStaleCallbacks();
    void dataCompletionAfterArchiveSwitchIsIgnored();
};

void TestMediaEntrySourceRuntime::synchronousLoadsShareLazyOpenAndCandidateCache()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    addInstrumentedArchiveFixture(
        state, *archiveCollection, { imageCandidate(firstUrl), imageCandidate(secondUrl) });

    KiriView::MediaEntrySourceRuntime runtime(this, instrumentedMediaEntrySourceFactory(state));
    std::vector<ImageNavigationCandidate> firstCandidates;
    std::vector<ImageNavigationCandidate> cachedCandidates;
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
    addInstrumentedArchiveFixture(state, *archiveCollection, { imageCandidate(pageUrl) });
    blockInstrumentedArchiveCandidateLoads(state);

    KiriView::MediaEntrySourceRuntime runtime(this, instrumentedMediaEntrySourceFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = runtime.loadOpenedCollectionCandidates(this, *archiveCollection,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    KiriView::ImageIoJob secondJob
        = runtime.loadOpenedCollectionCandidates(this, *archiveCollection,
            [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    releaseInstrumentedArchiveLoads(state);

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
    addInstrumentedArchiveFixture(state, *archiveCollection, { imageCandidate(pageUrl) });
    blockInstrumentedArchiveCandidateLoads(state);

    KiriView::MediaEntrySourceRuntime runtime(this, instrumentedMediaEntrySourceFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = runtime.loadOpenedCollectionCandidates(this, *archiveCollection,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    KiriView::ImageIoJob secondJob
        = runtime.loadOpenedCollectionCandidates(this, *archiveCollection,
            [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    releaseInstrumentedArchiveLoads(state);

    QTRY_COMPARE(callbackCount, 2);
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
    addInstrumentedArchiveFixture(state, *firstArchiveCollection, { imageCandidate(firstPageUrl) });
    addInstrumentedArchiveFixture(
        state, *secondArchiveCollection, { imageCandidate(secondPageUrl) });
    blockInstrumentedArchiveCandidateLoads(state);

    KiriView::MediaEntrySourceRuntime runtime(this, instrumentedMediaEntrySourceFactory(state));
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = runtime.loadOpenedCollectionCandidates(this,
        *firstArchiveCollection,
        [&staleCallbackCount](std::vector<ImageNavigationCandidate>) { ++staleCallbackCount; }, {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    runtime.switchToOpenedCollectionScope(*secondArchiveCollection);
    releaseInstrumentedArchiveLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(runtime.hasCurrentOpenedCollectionScope(*secondArchiveCollection));
}

void TestMediaEntrySourceRuntime::dataCompletionAfterArchiveSwitchIsIgnored()
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
    addInstrumentedArchiveFixture(state, *firstArchiveCollection, { imageCandidate(firstPageUrl) });
    addInstrumentedArchiveFixture(
        state, *secondArchiveCollection, { imageCandidate(secondPageUrl) });
    blockInstrumentedArchiveDataLoads(state);

    KiriView::MediaEntrySourceRuntime runtime(this, instrumentedMediaEntrySourceFactory(state));
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = runtime.loadOpenedCollectionImageData(this,
        KiriView::ImageDecodeRequest::fromLocation(1,
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                firstPageUrl, *firstArchiveCollection)),
        [&staleCallbackCount](QByteArray) { ++staleCallbackCount; }, {});
    QTRY_COMPARE(state->waitingDataLoadCount.load(), 1);

    runtime.switchToOpenedCollectionScope(*secondArchiveCollection);
    releaseInstrumentedArchiveLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(runtime.hasCurrentOpenedCollectionScope(*secondArchiveCollection));
}

QTEST_GUILESS_MAIN(TestMediaEntrySourceRuntime)

#include "test_mediaentrysourceruntime.moc"
