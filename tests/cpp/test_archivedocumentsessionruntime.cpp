// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/archivedocumentsessionruntime.h"

#include "archive_session_test_support.h"
#include "image_test_support.h"

#include <QByteArray>
#include <QTest>
#include <memory>
#include <optional>
#include <vector>

namespace {
using KiriView::ImageNavigationCandidate;
using KiriView::TestSupport::addInstrumentedArchiveFixture;
using KiriView::TestSupport::archiveDocumentForLocalArchiveUrl;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::blockInstrumentedArchiveCandidateLoads;
using KiriView::TestSupport::blockInstrumentedArchiveDataLoads;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::instrumentedArchiveSessionFactory;
using KiriView::TestSupport::InstrumentedArchiveSessionState;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::releaseInstrumentedArchiveLoads;
}

class TestArchiveDocumentSessionRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void synchronousLoadsShareLazyOpenAndCandidateCache();
    void simultaneousCandidateLoadsSharePendingBatch();
    void candidateLoadAddedDuringActiveBatchSharesWorker();
    void candidateBatchCancellationPreventsStaleCallbacks();
    void dataCompletionAfterArchiveSwitchIsIgnored();
};

void TestArchiveDocumentSessionRuntime::synchronousLoadsShareLazyOpenAndCandidateCache()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    addInstrumentedArchiveFixture(
        state, *archiveDocument, { imageCandidate(firstUrl), imageCandidate(secondUrl) });

    KiriView::ArchiveDocumentSessionRuntime runtime(this, instrumentedArchiveSessionFactory(state));
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
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedArchiveFixture(state, *archiveDocument, { imageCandidate(pageUrl) });
    blockInstrumentedArchiveCandidateLoads(state);

    KiriView::ArchiveDocumentSessionRuntime runtime(this, instrumentedArchiveSessionFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = runtime.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    KiriView::ImageIoJob secondJob = runtime.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    releaseInstrumentedArchiveLoads(state);

    QTRY_COMPARE(callbackCount, 2);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestArchiveDocumentSessionRuntime::candidateLoadAddedDuringActiveBatchSharesWorker()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/book.cbz")));
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedArchiveFixture(state, *archiveDocument, { imageCandidate(pageUrl) });
    blockInstrumentedArchiveCandidateLoads(state);

    KiriView::ArchiveDocumentSessionRuntime runtime(this, instrumentedArchiveSessionFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = runtime.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    KiriView::ImageIoJob secondJob = runtime.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    releaseInstrumentedArchiveLoads(state);

    QTRY_COMPARE(callbackCount, 2);
    QVERIFY(!firstJob.isActive());
    QVERIFY(!secondJob.isActive());
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestArchiveDocumentSessionRuntime::candidateBatchCancellationPreventsStaleCallbacks()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ArchiveDocumentLocation> firstArchiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/a.cbz")));
    const std::optional<KiriView::ArchiveDocumentLocation> secondArchiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/b.cbz")));
    QVERIFY(firstArchiveDocument.has_value());
    QVERIFY(secondArchiveDocument.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedArchiveFixture(state, *firstArchiveDocument, { imageCandidate(firstPageUrl) });
    addInstrumentedArchiveFixture(state, *secondArchiveDocument, { imageCandidate(secondPageUrl) });
    blockInstrumentedArchiveCandidateLoads(state);

    KiriView::ArchiveDocumentSessionRuntime runtime(this, instrumentedArchiveSessionFactory(state));
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = runtime.loadArchiveImages(this, *firstArchiveDocument,
        [&staleCallbackCount](std::vector<ImageNavigationCandidate>) { ++staleCallbackCount; }, {});
    QTRY_COMPARE(state->waitingCandidateLoadCount.load(), 1);

    runtime.switchToArchiveDocument(*secondArchiveDocument);
    releaseInstrumentedArchiveLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(runtime.hasCurrentArchiveDocument(*secondArchiveDocument));
}

void TestArchiveDocumentSessionRuntime::dataCompletionAfterArchiveSwitchIsIgnored()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const std::optional<KiriView::ArchiveDocumentLocation> firstArchiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/a.cbz")));
    const std::optional<KiriView::ArchiveDocumentLocation> secondArchiveDocument
        = archiveDocumentForLocalArchiveUrl(localUrl(QStringLiteral("/books/b.cbz")));
    QVERIFY(firstArchiveDocument.has_value());
    QVERIFY(secondArchiveDocument.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedArchiveFixture(state, *firstArchiveDocument, { imageCandidate(firstPageUrl) });
    addInstrumentedArchiveFixture(state, *secondArchiveDocument, { imageCandidate(secondPageUrl) });
    blockInstrumentedArchiveDataLoads(state);

    KiriView::ArchiveDocumentSessionRuntime runtime(this, instrumentedArchiveSessionFactory(state));
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = runtime.loadArchiveImageData(this,
        KiriView::ImageDecodeRequest::fromLocation(1,
            KiriView::DisplayedImageLocation::fromArchiveDocument(
                firstPageUrl, *firstArchiveDocument)),
        [&staleCallbackCount](QByteArray) { ++staleCallbackCount; }, {});
    QTRY_COMPARE(state->waitingDataLoadCount.load(), 1);

    runtime.switchToArchiveDocument(*secondArchiveDocument);
    releaseInstrumentedArchiveLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(runtime.hasCurrentArchiveDocument(*secondArchiveDocument));
}

QTEST_GUILESS_MAIN(TestArchiveDocumentSessionRuntime)

#include "test_archivedocumentsessionruntime.moc"
