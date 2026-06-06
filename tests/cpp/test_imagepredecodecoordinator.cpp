// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "predecode/imagepredecodecoordinator.h"

#include <QObject>
#include <QSize>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::imageDocumentPageCandidate;
using KiriView::TestSupport::imagesDirectoryUrl;
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::ManualPowerSaverMonitor;
using KiriView::TestSupport::powerSaverProviderFor;
using KiriView::TestSupport::staticDisplayTestImagePayload;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::testImage;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageDocumentPageCandidateProvider;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

KiriView::PowerSaverProvider noOpPowerSaverProvider()
{
    return KiriView::PowerSaverProvider {
        [](QObject *, KiriView::PowerSaverChangedCallback) {
            return std::unique_ptr<KiriView::PowerSaverStateMonitor>();
        },
    };
}

KiriView::ImagePredecodeCoordinator createCoordinator(QObject *parent,
    FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader,
    KiriView::PowerSaverProvider powerSaverProvider)
{
    return KiriView::ImagePredecodeCoordinator(parent, candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()),
        std::move(powerSaverProvider), testCacheByteBudget);
}

KiriView::ImagePredecodeCoordinator createCoordinator(
    QObject *parent, FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    return createCoordinator(parent, candidateProvider, dataLoader, noOpPowerSaverProvider());
}

std::vector<KiriView::ImageDocumentPageCandidate> imageDocumentPageCandidates(int count)
{
    std::vector<KiriView::ImageDocumentPageCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        candidates.push_back(imageDocumentPageCandidate(indexedImageUrl(index)));
    }
    return candidates;
}

KiriView::StaticDisplayImagePayload displayTestImagePayload(
    const QImage &image, qreal firstDisplayPixelsPerSourcePixel = 0.0)
{
    const KiriView::DisplayImageQuality quality = firstDisplayPixelsPerSourcePixel > 0.0
        ? KiriView::DisplayImageQuality::FirstDisplay
        : KiriView::DisplayImageQuality::Exact;
    return staticDisplayTestImagePayload(image, image, firstDisplayPixelsPerSourcePixel, quality);
}

KiriView::ImagePredecodeCoordinator::Context predecodeContext(
    KiriView::DisplayedPredecodeImage primaryImage,
    std::optional<KiriView::DisplayedPredecodeImage> secondaryImage = std::nullopt,
    KiriView::ImageFirstDisplayDecodeContext firstDisplayContext = {}, int pageIndex = -1)
{
    const KiriView::DisplayedImageLocation currentLocation = primaryImage.location;
    std::vector<KiriView::DisplayedPredecodeImage> displayedImages;
    displayedImages.push_back(std::move(primaryImage));
    if (secondaryImage.has_value()) {
        displayedImages.push_back(std::move(*secondaryImage));
    }

    return KiriView::ImagePredecodeCoordinator::Context {
        currentLocation,
        std::move(displayedImages),
        firstDisplayContext,
        pageIndex,
        {},
    };
}
}

class TestImagePredecodeCoordinator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scheduleCachesDisplayedImageAndPredecodesWindow();
    void scheduleCachesVisibleSpreadPagesAndSkipsSecondaryPredecode();
    void scheduleRejectsInvalidDisplayedContext();
    void archivePredecodeKeepsOpenedCollectionScopeContext();
    void regularPredecodeWindowKeepsOnePreviousAndTwoNextPages();
    void directoryCollectionStartsTwoBackgroundDecodes();
    void candidateListingFailureStartsEmptyFallbackWindow();
    void staleGenerationDecodeIsIgnored();
    void rapidNavigationDebouncesSkippedPagePredecode();
    void powerSaverMonitorSuppressesAndReschedulesPredecode();
    void cancelSuppressesPendingDecode();
};

void TestImagePredecodeCoordinator::scheduleCachesDisplayedImageAndPredecodesWindow()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl previousUrl = indexedImageUrl(0);
    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(previousUrl),
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    const QImage displayedImage = testImage();
    coordinator.schedule(predecodeContext(
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
            true,
            displayTestImagePayload(displayedImage, 0.5),
        },
        std::nullopt, KiriView::ImageFirstDisplayDecodeContext { QSize(640, 480) }));

    const std::optional<KiriView::PredecodedImage> displayed
        = coordinator.findPredecodedImage(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->location.imageUrl(), displayedUrl);
    QCOMPARE(displayed->displayImage.quality, KiriView::DisplayImageQuality::FirstDisplay);
    QCOMPARE(displayed->displayImage.displayPixelsPerSourcePixel, 0.5);

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(640, 480));
    dataLoader.finishFrontLoad(QByteArrayLiteral("next"));

    QTRY_VERIFY(coordinator.findPredecodedImage(nextUrl).has_value());
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, previousUrl);
}

void TestImagePredecodeCoordinator::scheduleCachesVisibleSpreadPagesAndSkipsSecondaryPredecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl primaryUrl = indexedImageUrl(0);
    const QUrl secondaryUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(primaryUrl),
            imageDocumentPageCandidate(secondaryUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    coordinator.schedule(predecodeContext(
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(primaryUrl),
            true,
            displayTestImagePayload(testImage(), 0.5),
        },
        std::make_optional(KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(secondaryUrl),
            true,
            displayTestImagePayload(testImage(), 0.75),
        })));

    const std::optional<KiriView::PredecodedImage> primary
        = coordinator.findPredecodedImage(primaryUrl);
    QVERIFY(primary.has_value());
    QCOMPARE(primary->displayImage.displayPixelsPerSourcePixel, 0.5);

    const std::optional<KiriView::PredecodedImage> secondary
        = coordinator.findPredecodedImage(secondaryUrl);
    QVERIFY(secondary.has_value());
    QCOMPARE(secondary->displayImage.displayPixelsPerSourcePixel, 0.75);

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
}

void TestImagePredecodeCoordinator::scheduleRejectsInvalidDisplayedContext()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {});

    QCOMPARE(dataLoader.loadCount(), std::size_t(0));
}

void TestImagePredecodeCoordinator::archivePredecodeKeepsOpenedCollectionScopeContext()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::OpenedCollectionScopeLocation> openedCollectionScope
        = KiriView::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(openedCollectionScope.has_value());
    const QUrl displayedUrl
        = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setOpenedCollectionCandidates(openedCollectionScope->rootUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    coordinator.schedule(predecodeContext(KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
            displayedUrl, *openedCollectionScope),
        false,
        displayTestImagePayload(testImage()),
    }));

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(
        dataLoader.frontLoad().openedCollectionScope.rootUrl(), openedCollectionScope->rootUrl());
    dataLoader.finishFrontLoad(QByteArrayLiteral("next"));

    QTRY_VERIFY(coordinator.findPredecodedImage(nextUrl).has_value());
    const std::optional<KiriView::PredecodedImage> predecoded
        = coordinator.findPredecodedImage(nextUrl);
    QVERIFY(predecoded.has_value());
    QCOMPARE(predecoded->location.openedCollectionScopeRootUrl(), openedCollectionScope->rootUrl());
}

void TestImagePredecodeCoordinator::regularPredecodeWindowKeepsOnePreviousAndTwoNextPages()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl displayedUrl = indexedImageUrl(5);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(), imageDocumentPageCandidates(15));

    coordinator.schedule(predecodeContext(KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        false,
        displayTestImagePayload(testImage()),
    }));

    const std::vector<QUrl> expectedLoadOrder {
        indexedImageUrl(6),
        indexedImageUrl(4),
        indexedImageUrl(7),
    };
    std::size_t expectedLoadCount = 0;
    for (const QUrl &expectedUrl : expectedLoadOrder) {
        ++expectedLoadCount;
        QTRY_VERIFY(dataLoader.loadCount() >= expectedLoadCount);
        QCOMPARE(dataLoader.backLoad().url, expectedUrl);
        QVERIFY(dataLoader.finishOldestActiveLoadForUrl(expectedUrl, QByteArrayLiteral("image")));
    }

    QTRY_COMPARE(dataLoader.loadCount(), expectedLoadOrder.size());
    QVERIFY(
        !dataLoader.finishOldestActiveLoadForUrl(indexedImageUrl(8), QByteArrayLiteral("image")));
}

void TestImagePredecodeCoordinator::directoryCollectionStartsTwoBackgroundDecodes()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const KiriView::OpenedCollectionScopeLocation directoryCollection
        = KiriView::OpenedCollectionScopeLocation::fromUrls(imagesDirectoryUrl(),
            imagesDirectoryUrl(), KiriView::OpenedCollectionScopeKind::Directory);
    const QUrl displayedUrl = indexedImageUrl(5);
    candidateProvider.setOpenedCollectionCandidates(
        directoryCollection.rootUrl(), imageDocumentPageCandidates(15));

    coordinator.schedule(predecodeContext(KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
            displayedUrl, directoryCollection),
        false,
        displayTestImagePayload(testImage()),
    }));

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.frontLoad().url, indexedImageUrl(6));
    QCOMPARE(dataLoader.backLoad().url, indexedImageUrl(4));
}

void TestImagePredecodeCoordinator::candidateListingFailureStartsEmptyFallbackWindow()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl displayedUrl = indexedImageUrl(5);
    candidateProvider.setDirectoryImageError(
        imagesDirectoryUrl(), QStringLiteral("candidate listing failed"));

    coordinator.schedule(predecodeContext(KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        true,
        displayTestImagePayload(testImage()),
    }));

    QTest::qWait(250);
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));
    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
}

void TestImagePredecodeCoordinator::staleGenerationDecodeIsIgnored()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    candidateProvider.setDirectoryImages(imagesDirectoryUrl(), imageDocumentPageCandidates(5));

    coordinator.schedule(predecodeContext(KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(indexedImageUrl(0)),
        false,
        displayTestImagePayload(testImage()),
    }));
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, indexedImageUrl(1));

    coordinator.schedule(predecodeContext(KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(indexedImageUrl(2)),
        false,
        displayTestImagePayload(testImage()),
    }));

    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("stale"));
    QTest::qWait(50);
    QVERIFY(!coordinator.findPredecodedImage(indexedImageUrl(1)).has_value());
}

void TestImagePredecodeCoordinator::rapidNavigationDebouncesSkippedPagePredecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    candidateProvider.setDirectoryImages(imagesDirectoryUrl(), imageDocumentPageCandidates(10));

    const auto schedulePage = [&coordinator](int pageIndex) {
        coordinator.schedule(predecodeContext(
            KiriView::DisplayedPredecodeImage {
                KiriView::DisplayedImageLocation::fromUrl(indexedImageUrl(pageIndex)),
                false,
                displayTestImagePayload(testImage()),
            },
            std::nullopt, {}, pageIndex));
    };

    schedulePage(0);
    schedulePage(1);
    schedulePage(2);

    QTest::qWait(250);
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, indexedImageUrl(3));
    QVERIFY(
        !dataLoader.finishOldestActiveLoadForUrl(indexedImageUrl(1), QByteArrayLiteral("image")));
}

void TestImagePredecodeCoordinator::powerSaverMonitorSuppressesAndReschedulesPredecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualPowerSaverMonitor *powerSaverMonitor = nullptr;
    KiriView::ImagePredecodeCoordinator coordinator = createCoordinator(
        this, candidateProvider, dataLoader, powerSaverProviderFor(powerSaverMonitor, true));
    QVERIFY(powerSaverMonitor != nullptr);
    QVERIFY(coordinator.powerSaverEnabled());

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    coordinator.schedule(predecodeContext(KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        true,
        displayTestImagePayload(testImage()),
    }));

    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
    QTest::qWait(250);
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    powerSaverMonitor->setPowerSaverEnabled(false);
    QVERIFY(!coordinator.powerSaverEnabled());
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);

    powerSaverMonitor->setPowerSaverEnabled(true);
    QVERIFY(coordinator.powerSaverEnabled());
    QVERIFY(dataLoader.frontLoad().canceled);
    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("stale"));
    QTest::qWait(50);
    QVERIFY(!coordinator.findPredecodedImage(nextUrl).has_value());
    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
}

void TestImagePredecodeCoordinator::cancelSuppressesPendingDecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    const QImage displayedImage = testImage();
    coordinator.schedule(predecodeContext(KiriView::DisplayedPredecodeImage {
        KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
        false,
        displayTestImagePayload(displayedImage),
    }));

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    coordinator.cancel();
    QVERIFY(dataLoader.frontLoad().canceled);

    dataLoader.finishFrontLoad(QByteArrayLiteral("next"));
    QTest::qWait(50);
    QVERIFY(!coordinator.findPredecodedImage(nextUrl).has_value());
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
}

QTEST_GUILESS_MAIN(TestImagePredecodeCoordinator)

#include "test_imagepredecodecoordinator.moc"
