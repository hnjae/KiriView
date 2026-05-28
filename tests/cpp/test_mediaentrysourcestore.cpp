// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archive/mediaentrysourcestore.h"

#include "async/imagecallback.h"
#include "document/imagedocumentsourceloadrequest.h"
#include "image_test_support.h"
#include "media_entry_source_test_support.h"
#include "navigation/imagenavigationservice.h"
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
using KiriView::ImageNavigationCandidate;
using KiriView::NavigationDirection;
using KiriView::TestSupport::addInstrumentedArchiveFixture;
using KiriView::TestSupport::archiveCollectionForLocalArchiveUrl;
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::blockInstrumentedArchiveDataLoads;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::instrumentedMediaEntrySourceFactory;
using KiriView::TestSupport::InstrumentedMediaEntrySourceState;
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
    std::function<void(const QUrl &)> openUrl = {},
    KiriView::ImageNavigationService::PageNavigationChangedCallback pageNavigationChanged = {})
{
    return KiriView::ImageNavigationService::Callbacks {
        [openUrl = std::move(openUrl)](KiriView::ImageNavigationPlan plan) mutable {
            for (const KiriView::ImageNavigationEffect &effect : plan) {
                if (const auto *openEffect
                    = std::get_if<KiriView::OpenImageNavigationUrlEffect>(&effect)) {
                    KiriView::invokeIfSet(openUrl, openEffect->target.url);
                }
            }
        },
        std::move(pageNavigationChanged),
        {},
    };
}

std::optional<KiriView::ImageCandidateListContext> navigationContext(
    KiriView::DisplayedImageLocation displayedImage)
{
    return KiriView::imageCandidateListContextForDisplayedImage(std::move(displayedImage));
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
    void predecodeLoadsAdjacentArchiveImagesThroughSource();
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
    addInstrumentedArchiveFixture(
        state, *archiveCollection, { imageCandidate(firstUrl), imageCandidate(secondUrl) });

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    std::vector<ImageNavigationCandidate> candidates;
    QByteArray firstData;
    QByteArray secondData;
    store.loadOpenedCollectionCandidates(nullptr, *archiveCollection,
        [&candidates](
            std::vector<ImageNavigationCandidate> loaded) { candidates = std::move(loaded); },
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
    addInstrumentedArchiveFixture(state, *archiveCollection, { imageCandidate(pageUrl) });

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    int callbackCount = 0;
    KiriView::ImageIoJob firstJob = store.loadOpenedCollectionCandidates(this, *archiveCollection,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});
    KiriView::ImageIoJob secondJob = store.loadOpenedCollectionCandidates(this, *archiveCollection,
        [&callbackCount](std::vector<ImageNavigationCandidate>) { ++callbackCount; }, {});

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
    addInstrumentedArchiveFixture(state, *firstArchiveCollection, { imageCandidate(firstPageUrl) });
    addInstrumentedArchiveFixture(
        state, *secondArchiveCollection, { imageCandidate(secondPageUrl) });
    blockInstrumentedArchiveDataLoads(state);

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
    releaseInstrumentedArchiveLoads(state);

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
    addInstrumentedArchiveFixture(state, *archiveCollection,
        { imageCandidate(firstUrl), imageCandidate(secondUrl), imageCandidate(thirdUrl) });

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    QUrl openedUrl;
    KiriView::ImageNavigationService service(this,
        store.wrapCandidateProvider(archiveOnlyProvider()),
        navigationCallbacks([&openedUrl](const QUrl &url) { openedUrl = url; }));

    service.updatePageNavigation(navigationContext(
        KiriView::DisplayedImageLocation::fromOpenedCollectionScope(firstUrl, *archiveCollection)));
    QTRY_COMPARE(service.imageCount(), 3);
    QCOMPARE(service.currentPageNumber(), 1);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);

    service.openAdjacentImage(
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
    QCOMPARE(service.imageCount(), 3);
    QCOMPARE(state->openCount.load(), 1);
    QCOMPARE(state->candidateLoadCount.load(), 1);
}

void TestMediaEntrySourceStore::predecodeLoadsAdjacentArchiveImagesThroughSource()
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
    addInstrumentedArchiveFixture(state, *archiveCollection,
        { imageCandidate(firstUrl), imageCandidate(displayedUrl), imageCandidate(thirdUrl) });

    KiriView::MediaEntrySourceStore store(instrumentedMediaEntrySourceFactory(state));
    KiriView::ImagePredecodeCoordinator coordinator(this,
        store.wrapCandidateProvider(archiveOnlyProvider()),
        store.wrapDecodeDependencies(KiriView::ImageDecodeDependencies {
            {},
            staticImageDataDecoder(),
        }),
        KiriView::PowerSaverProvider {}, 1024 * 1024);

    KiriView::DisplayedPredecodeImage displayedImage {
        KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
            displayedUrl, *archiveCollection),
        true,
        staticTestImagePayload(testImage()),
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
    addInstrumentedArchiveFixture(state, *firstArchiveCollection, { imageCandidate(firstPageUrl) });
    addInstrumentedArchiveFixture(
        state, *secondArchiveCollection, { imageCandidate(secondPageUrl) });

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
