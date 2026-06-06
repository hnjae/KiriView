// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/mediaentrysourcestore.h"

#include "async/imagecallback.h"
#include "document/imagedocumentsourceloadrequest.h"
#include "image_test_support.h"
#include "media_entry_source_test_support.h"
#include "navigation/imagedocumentpagenavigationservice.h"
#include "predecode/imagepredecodecoordinator.h"

#include <QByteArray>
#include <QTest>
#include <functional>
#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace {
using KiriView::ImageDocumentPageCandidate;
using KiriView::NavigationDirection;
using KiriView::TestSupport::addInstrumentedMediaEntrySourceFixture;
using KiriView::TestSupport::archiveCollectionForLocalArchiveUrl;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::blockInstrumentedMediaEntrySourceDataLoads;
using KiriView::TestSupport::imageDocumentPageCandidate;
using KiriView::TestSupport::instrumentedMediaEntrySourceFactory;
using KiriView::TestSupport::InstrumentedMediaEntrySourceState;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::releaseInstrumentedMediaEntrySourceLoads;
using KiriView::TestSupport::staticDisplayTestImagePayload;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::testImage;

KiriView::ImageDocumentPageCandidateProvider openedCollectionOnlyProvider()
{
    return KiriView::ImageDocumentPageCandidateProvider {
        [](QObject *, QUrl, KiriView::ImageDocumentPageCandidatesCallback,
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

KiriView::ImageDocumentPageNavigationService::Callbacks navigationCallbacks(
    std::function<void(const QUrl &)> openUrl = {},
    KiriView::ImageDocumentPageNavigationService::PageNavigationChangedCallback
        pageNavigationChanged
    = {})
{
    return KiriView::ImageDocumentPageNavigationService::Callbacks {
        [openUrl = std::move(openUrl)](KiriView::ImageDocumentPageNavigationPlan plan) mutable {
            for (const KiriView::ImageDocumentPageNavigationEffect &effect : plan) {
                if (const auto *openEffect
                    = std::get_if<KiriView::OpenImageDocumentPageUrlEffect>(&effect)) {
                    KiriView::invokeIfSet(openUrl, openEffect->target.url);
                }
            }
        },
        std::move(pageNavigationChanged),
        {},
    };
}

std::optional<KiriView::ImageDocumentPageCandidateListContext> navigationContext(
    KiriView::DisplayedImageLocation displayedImage)
{
    return KiriView::imageDocumentPageCandidateListContextForDisplayedImage(
        std::move(displayedImage));
}
}

class TestMediaEntrySourceStore : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void candidateAndDataLoadsShareOneArchiveOpen();
    void simultaneousCandidateLoadsSharePendingSourceLoad();
    void staleDataLoadCompletionIsIgnoredAfterArchiveSwitch();
    void navigationReusesCachedOpenedCollectionCandidates();
    void predecodeLoadsAdjacentOpenedCollectionImagesThroughSource();
    void lifecycleClearsSourceForDifferentArchiveNormalImageAndClear();
};

void TestMediaEntrySourceStore::candidateAndDataLoadsShareOneArchiveOpen()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    addInstrumentedMediaEntrySourceFixture(state, *archiveCollection,
        { imageDocumentPageCandidate(firstUrl), imageDocumentPageCandidate(secondUrl) });

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    std::vector<ImageDocumentPageCandidate> candidates;
    QByteArray firstData;
    QByteArray secondData;
    store.loadOpenedCollectionCandidates(nullptr, *archiveCollection,
        [&candidates](
            std::vector<ImageDocumentPageCandidate> loaded) { candidates = std::move(loaded); },
        {});
    store.loadOpenedCollectionCandidates(nullptr, *archiveCollection, [](auto) { }, {});
    store.loadOpenedCollectionImageData(nullptr,
        KiriView::ImageDecodeRequest::fromLocation(1,
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                firstUrl, *archiveCollection)),
        [&firstData](QByteArray data) { firstData = std::move(data); }, {});
    store.loadOpenedCollectionImageData(nullptr,
        KiriView::ImageDecodeRequest::fromLocation(2,
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                secondUrl, *archiveCollection)),
        [&secondData](QByteArray data) { secondData = std::move(data); }, {});

    QCOMPARE(candidates.size(), std::size_t(2));
    QCOMPARE(firstData, QByteArrayLiteral("image"));
    QCOMPARE(secondData, QByteArrayLiteral("image"));
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 2);
}

void TestMediaEntrySourceStore::simultaneousCandidateLoadsSharePendingSourceLoad()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl pageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedMediaEntrySourceFixture(
        state, *archiveCollection, { imageDocumentPageCandidate(pageUrl) });

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = store.loadOpenedCollectionCandidates(this, *archiveCollection,
        [&callbackCount](std::vector<ImageDocumentPageCandidate>) { ++callbackCount; }, {});
    KiriView::ImageIoJob secondJob = store.loadOpenedCollectionCandidates(this, *archiveCollection,
        [&callbackCount](std::vector<ImageDocumentPageCandidate>) { ++callbackCount; }, {});

    QTRY_COMPARE(callbackCount, 2);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestMediaEntrySourceStore::staleDataLoadCompletionIsIgnoredAfterArchiveSwitch()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const QUrl firstArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl secondArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> firstArchiveCollection
        = archiveCollectionForLocalArchiveUrl(firstArchiveUrl);
    const std::optional<KiriView::OpenedCollectionScopeLocation> secondArchiveCollection
        = archiveCollectionForLocalArchiveUrl(secondArchiveUrl);
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
    blockInstrumentedMediaEntrySourceDataLoads(state);

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    int staleCallbackCount = 0;
    KiriView::ImageIoJob staleJob = store.loadOpenedCollectionImageData(this,
        KiriView::ImageDecodeRequest::fromLocation(1,
            KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
                firstPageUrl, *firstArchiveCollection)),
        [&staleCallbackCount](QByteArray) { ++staleCallbackCount; }, {});
    QTRY_COMPARE(state->waitingDataLoadCount.load(), 1);

    store.prepareForSourceLoad(KiriView::ImageDocumentSourceLoadRequest::fromUrl(secondArchiveUrl),
        *firstArchiveCollection);
    releaseInstrumentedMediaEntrySourceLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(store.hasCurrentOpenedCollectionScope(*secondArchiveCollection));
}

void TestMediaEntrySourceStore::navigationReusesCachedOpenedCollectionCandidates()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    addInstrumentedMediaEntrySourceFixture(state, *archiveCollection,
        { imageDocumentPageCandidate(firstUrl), imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl) });

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    QUrl openedUrl;
    KiriView::ImageDocumentPageNavigationService service(this,
        store.wrapCandidateProvider(openedCollectionOnlyProvider()),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }));

    service.updatePageNavigation(navigationContext(
        KiriView::DisplayedImageLocation::fromOpenedCollectionScope(firstUrl, *archiveCollection)));
    QTRY_COMPARE(service.pageCount(), 3);
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    service.openAdjacentPage(
        navigationContext(KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
            firstUrl, *archiveCollection)),
        NavigationDirection::Next);
    QCOMPARE(openedUrl, secondUrl);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    service.updatePageNavigation(
        navigationContext(KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
            secondUrl, *archiveCollection)));
    QCOMPARE(service.currentPageNumber(), 2);
    QCOMPARE(service.pageCount(), 3);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestMediaEntrySourceStore::predecodeLoadsAdjacentOpenedCollectionImagesThroughSource()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl displayedUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    addInstrumentedMediaEntrySourceFixture(state, *archiveCollection,
        { imageDocumentPageCandidate(firstUrl), imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(thirdUrl) });

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    KiriView::ImagePredecodeCoordinator coordinator(this,
        store.wrapCandidateProvider(openedCollectionOnlyProvider()),
        store.wrapDecodeDependencies(KiriView::ImageDecodeDependencies {
            {},
            staticImageDataDecoder(),
        }),
        KiriView::PowerSaverProvider {}, 1024 * 1024);

    KiriView::DisplayedPredecodeImage displayedImage {
        KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
            displayedUrl, *archiveCollection),
        true,
        staticDisplayTestImagePayload(testImage()),
    };
    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        displayedImage.location,
        { std::move(displayedImage) },
        {},
        -1,
        {},
    });

    QTRY_VERIFY(coordinator.findPredecodedImage(thirdUrl).has_value());
    QTRY_VERIFY(coordinator.findPredecodedImage(firstUrl).has_value());
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
    QCOMPARE(state->dataLoadCount.load(), 2);
}

void TestMediaEntrySourceStore::lifecycleClearsSourceForDifferentArchiveNormalImageAndClear()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const QUrl firstArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl secondArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> firstArchiveCollection
        = archiveCollectionForLocalArchiveUrl(firstArchiveUrl);
    const std::optional<KiriView::OpenedCollectionScopeLocation> secondArchiveCollection
        = archiveCollectionForLocalArchiveUrl(secondArchiveUrl);
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

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    store.loadOpenedCollectionCandidates(nullptr, *firstArchiveCollection, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 1);
    QVERIFY(store.hasCurrentOpenedCollectionScope(*firstArchiveCollection));

    store.prepareForSourceLoad(KiriView::ImageDocumentSourceLoadRequest::fromUrl(secondArchiveUrl),
        *firstArchiveCollection);
    QVERIFY(store.hasCurrentOpenedCollectionScope(*secondArchiveCollection));
    store.loadOpenedCollectionCandidates(nullptr, *secondArchiveCollection, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 2);

    store.prepareForSourceLoad(KiriView::ImageDocumentSourceLoadRequest::fromUrl(
                                   localUrl(QStringLiteral("/images/01.png"))),
        *secondArchiveCollection);
    QVERIFY(!store.hasCurrentOpenedCollectionScope());

    store.loadOpenedCollectionCandidates(nullptr, *secondArchiveCollection, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 3);
    store.clear();
    QVERIFY(!store.hasCurrentOpenedCollectionScope());
}

QTEST_GUILESS_MAIN(TestMediaEntrySourceStore)

#include "test_mediaentrysourcestore.moc"
