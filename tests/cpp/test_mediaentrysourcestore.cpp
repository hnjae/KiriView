// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/mediaentrysourcestore.h"

#include "async/imagecallback.h"
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
using kiriview::ImageDocumentPageCandidate;
using kiriview::NavigationDirection;
using kiriview::TestSupport::addInstrumentedMediaEntrySourceFixture;
using kiriview::TestSupport::archiveCollectionForLocalArchiveUrl;
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::blockInstrumentedMediaEntrySourceDataLoads;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::instrumentedMediaEntrySourceFactory;
using kiriview::TestSupport::InstrumentedMediaEntrySourceState;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::releaseInstrumentedMediaEntrySourceLoads;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::staticImageDataDecoder;
using kiriview::TestSupport::testImage;

kiriview::ImageDocumentPageCandidateProvider openedCollectionOnlyProvider()
{
    return kiriview::ImageDocumentPageCandidateProvider {
        [](QObject*, QUrl, kiriview::ImageDocumentPageCandidatesCallback,
            kiriview::ErrorCallback errorCallback) {
            kiriview::invokeIfSet(errorCallback, QStringLiteral("unexpected directory listing"));
            return kiriview::ImageIoJob();
        },
        [](QObject*, QUrl, kiriview::ContainerCandidatesCallback,
            kiriview::ErrorCallback errorCallback) {
            kiriview::invokeIfSet(errorCallback, QStringLiteral("unexpected container listing"));
            return kiriview::ImageIoJob();
        },
        {},
        {},
    };
}

kiriview::ImageDocumentPageNavigationService::Callbacks navigationCallbacks(
    std::function<void(const QUrl&)> openUrl = {},
    kiriview::ImageDocumentPageNavigationService::PageNavigationChangedCallback
        pageNavigationChanged
    = {})
{
    return kiriview::ImageDocumentPageNavigationService::Callbacks {
        [openUrl = std::move(openUrl)](kiriview::ImageDocumentPageNavigationPlan plan) mutable {
            for (const kiriview::ImageDocumentPageNavigationEffect& effect : plan) {
                if (const auto* openEffect
                    = std::get_if<kiriview::OpenImageDocumentPageUrlEffect>(&effect)) {
                    kiriview::invokeIfSet(openUrl, openEffect->target.url);
                }
            }
        },
        std::move(pageNavigationChanged),
        {},
    };
}

std::optional<kiriview::ImageDocumentPageCandidateListContext> navigationContext(
    kiriview::DisplayedImageLocation displayedImage)
{
    return kiriview::imageDocumentPageCandidateListContextForDisplayedImage(
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
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    addInstrumentedMediaEntrySourceFixture(state, *archiveCollection,
        { imageDocumentPageCandidate(firstUrl), imageDocumentPageCandidate(secondUrl) });

    kiriview::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    std::vector<ImageDocumentPageCandidate> candidates;
    QByteArray firstData;
    QByteArray secondData;
    store.loadOpenedCollectionCandidates(nullptr, *archiveCollection,
        [&candidates](
            std::vector<ImageDocumentPageCandidate> loaded) { candidates = std::move(loaded); },
        {});
    store.loadOpenedCollectionCandidates(nullptr, *archiveCollection, [](auto) { }, {});
    store.loadOpenedCollectionImageData(nullptr,
        kiriview::ImageDecodeRequest::fromLocation(1,
            kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
                firstUrl, *archiveCollection)),
        [&firstData](QByteArray data) { firstData = std::move(data); }, {});
    store.loadOpenedCollectionImageData(nullptr,
        kiriview::ImageDecodeRequest::fromLocation(2,
            kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
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
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl pageUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    addInstrumentedMediaEntrySourceFixture(
        state, *archiveCollection, { imageDocumentPageCandidate(pageUrl) });

    kiriview::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    int callbackCount = 0;
    kiriview::ImageIoJob firstJob = store.loadOpenedCollectionCandidates(this, *archiveCollection,
        [&callbackCount](std::vector<ImageDocumentPageCandidate>) { ++callbackCount; }, {});
    kiriview::ImageIoJob secondJob = store.loadOpenedCollectionCandidates(this, *archiveCollection,
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
    const std::optional<kiriview::OpenedCollectionScopeLocation> firstArchiveCollection
        = archiveCollectionForLocalArchiveUrl(firstArchiveUrl);
    const std::optional<kiriview::OpenedCollectionScopeLocation> secondArchiveCollection
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

    kiriview::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    int staleCallbackCount = 0;
    kiriview::ImageIoJob staleJob = store.loadOpenedCollectionImageData(this,
        kiriview::ImageDecodeRequest::fromLocation(1,
            kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
                firstPageUrl, *firstArchiveCollection)),
        [&staleCallbackCount](QByteArray) { ++staleCallbackCount; }, {});
    QTRY_COMPARE(state->waitingDataLoadCount.load(), 1);

    store.prepareForOpenedCollectionScope(*secondArchiveCollection);
    releaseInstrumentedMediaEntrySourceLoads(state);

    QTRY_VERIFY(!staleJob.isActive());
    QCOMPARE(staleCallbackCount, 0);
    QVERIFY(store.hasCurrentOpenedCollectionScope(*secondArchiveCollection));
}

void TestMediaEntrySourceStore::navigationReusesCachedOpenedCollectionCandidates()
{
    auto state = std::make_shared<InstrumentedMediaEntrySourceState>();
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    addInstrumentedMediaEntrySourceFixture(state, *archiveCollection,
        { imageDocumentPageCandidate(firstUrl), imageDocumentPageCandidate(secondUrl),
            imageDocumentPageCandidate(thirdUrl) });

    kiriview::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    QUrl openedUrl;
    kiriview::ImageDocumentPageNavigationService service(this,
        store.wrapCandidateProvider(openedCollectionOnlyProvider()),
        navigationCallbacks([&openedUrl](const QUrl& url) { openedUrl = url; }));

    service.updatePageNavigation(navigationContext(
        kiriview::DisplayedImageLocation::fromOpenedCollectionScope(firstUrl, *archiveCollection)));
    QTRY_COMPARE(service.pageCount(), 3);
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    service.openAdjacentPage(
        navigationContext(kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            firstUrl, *archiveCollection)),
        NavigationDirection::Next);
    QCOMPARE(openedUrl, secondUrl);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    service.updatePageNavigation(
        navigationContext(kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
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
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = archiveCollectionForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl displayedUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    addInstrumentedMediaEntrySourceFixture(state, *archiveCollection,
        { imageDocumentPageCandidate(firstUrl), imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(thirdUrl) });

    kiriview::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    kiriview::ImagePredecodeCoordinator coordinator(this,
        store.wrapCandidateProvider(openedCollectionOnlyProvider()),
        store.wrapDecodeDependencies(kiriview::ImageDecodeDependencies {
            {},
            staticImageDataDecoder(),
        }),
        kiriview::PowerSaverProvider {}, 1024 * 1024);

    kiriview::DisplayedPredecodeImage displayedImage {
        kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            displayedUrl, *archiveCollection),
        true,
        staticDisplayTestImagePayload(testImage()),
    };
    coordinator.schedule(kiriview::ImagePredecodeCoordinator::Context {
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
    const std::optional<kiriview::OpenedCollectionScopeLocation> firstArchiveCollection
        = archiveCollectionForLocalArchiveUrl(firstArchiveUrl);
    const std::optional<kiriview::OpenedCollectionScopeLocation> secondArchiveCollection
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

    kiriview::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    store.loadOpenedCollectionCandidates(nullptr, *firstArchiveCollection, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 1);
    QVERIFY(store.hasCurrentOpenedCollectionScope(*firstArchiveCollection));

    store.prepareForOpenedCollectionScope(*secondArchiveCollection);
    QVERIFY(store.hasCurrentOpenedCollectionScope(*secondArchiveCollection));
    store.loadOpenedCollectionCandidates(nullptr, *secondArchiveCollection, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 2);

    store.prepareForOpenedCollectionScope(kiriview::OpenedCollectionScopeLocation::none());
    QVERIFY(!store.hasCurrentOpenedCollectionScope());

    store.loadOpenedCollectionCandidates(nullptr, *secondArchiveCollection, [](auto) { }, {});
    QCOMPARE(state->openCount.load(), 3);
    store.clear();
    QVERIFY(!store.hasCurrentOpenedCollectionScope());
}

QTEST_GUILESS_MAIN(TestMediaEntrySourceStore)

#include "test_mediaentrysourcestore.moc"
