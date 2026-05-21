// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/archivedocumentsessionstore.h"

#include "archive_session_test_support.h"
#include "async/imagecallback.h"
#include "document/imagedocumentsourceloadrequest.h"
#include "image_test_support.h"
#include "navigation/imagenavigationservice.h"
#include "predecode/imagepredecodecoordinator.h"

#include <QByteArray>
#include <QTest>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::ImageNavigationCandidate;
using KiriView::NavigationDirection;
using KiriView::TestSupport::addInstrumentedArchiveFixture;
using KiriView::TestSupport::archiveDocumentForLocalArchiveUrl;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::blockInstrumentedArchiveDataLoads;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::instrumentedArchiveSessionFactory;
using KiriView::TestSupport::InstrumentedArchiveSessionState;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::releaseInstrumentedArchiveLoads;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

KiriView::ImageNavigationCandidateProvider archiveOnlyProvider()
{
    return KiriView::ImageNavigationCandidateProvider {
        [](QObject *, QUrl, KiriView::ImageCandidatesCallback,
            KiriView::ErrorCallback errorCallback) {
            KiriView::invokeIfSet(errorCallback, QStringLiteral("unexpected directory listing"));
            return KiriView::ImageIoJob();
        },
        [](QObject *, QUrl, KiriView::ContainerCandidatesCallback,
            KiriView::ErrorCallback errorCallback) {
            KiriView::invokeIfSet(errorCallback, QStringLiteral("unexpected container listing"));
            return KiriView::ImageIoJob();
        },
        {},
        {},
    };
}

KiriView::ImageNavigationService::Callbacks navigationCallbacks(
    KiriView::ImageNavigationService::OpenUrlCallback openUrl = {},
    KiriView::ImageNavigationService::PageNavigationChangedCallback pageNavigationChanged = {})
{
    return KiriView::ImageNavigationService::Callbacks {
        std::move(openUrl),
        {},
        {},
        std::move(pageNavigationChanged),
        {},
        {},
    };
}

std::optional<KiriView::ImageCandidateListContext> navigationContext(
    KiriView::DisplayedImageLocation displayedImage)
{
    return KiriView::imageCandidateListContextForDisplayedImage(std::move(displayedImage));
}
}

class TestArchiveDocumentSessionStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void candidateAndDataLoadsShareOneArchiveOpen();
    void simultaneousCandidateLoadsSharePendingSessionLoad();
    void staleDataLoadCompletionIsIgnoredAfterArchiveSwitch();
    void navigationReusesCachedArchiveCandidates();
    void predecodeLoadsAdjacentArchiveImagesThroughSession();
    void lifecycleClearsSessionForDifferentArchiveNormalImageAndClear();
};

void TestArchiveDocumentSessionStore::candidateAndDataLoadsShareOneArchiveOpen()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    addInstrumentedArchiveFixture(
        state, *archiveDocument, { imageCandidate(firstUrl), imageCandidate(secondUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedArchiveSessionFactory(state));
    std::vector<ImageNavigationCandidate> candidates;
    QByteArray firstData;
    QByteArray secondData;
    store.loadArchiveImages(nullptr, *archiveDocument,
        [&candidates](
            std::vector<ImageNavigationCandidate> loaded) { candidates = std::move(loaded); },
        {});
    store.loadArchiveImages(nullptr, *archiveDocument, [](auto) { }, {});
    store.loadArchiveImageData(nullptr,
        KiriView::ImageDecodeRequest::fromLocation(
            1, KiriView::DisplayedImageLocation::fromArchiveDocument(firstUrl, *archiveDocument)),
        [&firstData](QByteArray data) { firstData = std::move(data); }, {});
    store.loadArchiveImageData(nullptr,
        KiriView::ImageDecodeRequest::fromLocation(
            2, KiriView::DisplayedImageLocation::fromArchiveDocument(secondUrl, *archiveDocument)),
        [&secondData](QByteArray data) { secondData = std::move(data); }, {});

    QCOMPARE(candidates.size(), std::size_t(2));
    QCOMPARE(firstData, QByteArrayLiteral("image"));
    QCOMPARE(secondData, QByteArrayLiteral("image"));
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 2);
}

void TestArchiveDocumentSessionStore::simultaneousCandidateLoadsSharePendingSessionLoad()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl pageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedArchiveFixture(state, *archiveDocument, { imageCandidate(pageUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedArchiveSessionFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = store.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    KiriView::ImageIoJob secondJob = store.loadArchiveImages(this, *archiveDocument,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});

    QTRY_COMPARE(callbackCount, 2);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestArchiveDocumentSessionStore::staleDataLoadCompletionIsIgnoredAfterArchiveSwitch()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl firstArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl secondArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> firstArchiveDocument
        = archiveDocumentForLocalArchiveUrl(firstArchiveUrl);
    const std::optional<KiriView::ArchiveDocumentLocation> secondArchiveDocument
        = archiveDocumentForLocalArchiveUrl(secondArchiveUrl);
    QVERIFY(firstArchiveDocument.has_value());
    QVERIFY(secondArchiveDocument.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedArchiveFixture(state, *firstArchiveDocument, { imageCandidate(firstPageUrl) });
    addInstrumentedArchiveFixture(state, *secondArchiveDocument, { imageCandidate(secondPageUrl) });
    blockInstrumentedArchiveDataLoads(state);

    KiriView::ArchiveDocumentSessionStore store(instrumentedArchiveSessionFactory(state));
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = store.loadArchiveImageData(this,
        KiriView::ImageDecodeRequest::fromLocation(1,
            KiriView::DisplayedImageLocation::fromArchiveDocument(
                firstPageUrl, *firstArchiveDocument)),
        [&staleCallbackCount](QByteArray) { ++staleCallbackCount; }, {});
    QTRY_COMPARE(state->waitingDataLoadCount.load(), 1);

    store.prepareForSourceLoad(
        KiriView::ImageDocumentSourceLoadRequest::fromUrl(secondArchiveUrl), *firstArchiveDocument);
    releaseInstrumentedArchiveLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(store.hasCurrentArchiveDocument(*secondArchiveDocument));
}

void TestArchiveDocumentSessionStore::navigationReusesCachedArchiveCandidates()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    addInstrumentedArchiveFixture(state, *archiveDocument,
        { imageCandidate(firstUrl), imageCandidate(secondUrl), imageCandidate(thirdUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedArchiveSessionFactory(state));
    QUrl openedUrl;
    KiriView::ImageNavigationService service(this,
        store.wrapCandidateProvider(archiveOnlyProvider()),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }));

    service.updatePageNavigation(navigationContext(
        KiriView::DisplayedImageLocation::fromArchiveDocument(firstUrl, *archiveDocument)));
    QTRY_COMPARE(service.imageCount(), 3);
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    service.openAdjacentImage(
        navigationContext(
            KiriView::DisplayedImageLocation::fromArchiveDocument(firstUrl, *archiveDocument)),
        NavigationDirection::Next);
    QCOMPARE(openedUrl, secondUrl);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    service.updatePageNavigation(navigationContext(
        KiriView::DisplayedImageLocation::fromArchiveDocument(secondUrl, *archiveDocument)));
    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.imageCount(), 3);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestArchiveDocumentSessionStore::predecodeLoadsAdjacentArchiveImagesThroughSession()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = archiveDocumentForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl displayedUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    addInstrumentedArchiveFixture(state, *archiveDocument,
        { imageCandidate(firstUrl), imageCandidate(displayedUrl), imageCandidate(thirdUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedArchiveSessionFactory(state));
    KiriView::ImagePredecodeCoordinator coordinator(this,
        store.wrapCandidateProvider(archiveOnlyProvider()),
        store.wrapDecodeDependencies(KiriView::ImageDecodeDependencies {
            {},
            staticImageDataDecoder(),
        }));

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromArchiveDocument(displayedUrl, *archiveDocument),
            true,
            staticTestImagePayload(testImage()),
        },
    });

    QTRY_VERIFY(coordinator.findPredecodedImage(thirdUrl).has_value());
    QTRY_VERIFY(coordinator.findPredecodedImage(firstUrl).has_value());
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 2);
}

void TestArchiveDocumentSessionStore::lifecycleClearsSessionForDifferentArchiveNormalImageAndClear()
{
    auto state = std::make_shared<InstrumentedArchiveSessionState>();
    const QUrl firstArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl secondArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> firstArchiveDocument
        = archiveDocumentForLocalArchiveUrl(firstArchiveUrl);
    const std::optional<KiriView::ArchiveDocumentLocation> secondArchiveDocument
        = archiveDocumentForLocalArchiveUrl(secondArchiveUrl);
    QVERIFY(firstArchiveDocument.has_value());
    QVERIFY(secondArchiveDocument.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedArchiveFixture(state, *firstArchiveDocument, { imageCandidate(firstPageUrl) });
    addInstrumentedArchiveFixture(state, *secondArchiveDocument, { imageCandidate(secondPageUrl) });

    KiriView::ArchiveDocumentSessionStore store(instrumentedArchiveSessionFactory(state));
    store.loadArchiveImages(nullptr, *firstArchiveDocument, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 1);
    QVERIFY(store.hasCurrentArchiveDocument(*firstArchiveDocument));

    store.prepareForSourceLoad(
        KiriView::ImageDocumentSourceLoadRequest::fromUrl(secondArchiveUrl), *firstArchiveDocument);
    QVERIFY(store.hasCurrentArchiveDocument(*secondArchiveDocument));
    store.loadArchiveImages(nullptr, *secondArchiveDocument, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 2);

    store.prepareForSourceLoad(KiriView::ImageDocumentSourceLoadRequest::fromUrl(
                                   localUrl(QStringLiteral("/images/01.png"))),
        *secondArchiveDocument);
    QVERIFY(!store.hasCurrentArchiveDocument());

    store.loadArchiveImages(nullptr, *secondArchiveDocument, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 3);
    store.clear();
    QVERIFY(!store.hasCurrentArchiveDocument());
}

QTEST_GUILESS_MAIN(TestArchiveDocumentSessionStore)

#include "test_archivedocumentsessionstore.moc"
