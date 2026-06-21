// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decoding/kiriimagedecoder.h"
#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "predecode/imagepredecodecoordinator.h"

#include <QFile>
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
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::imageDecodeDependenciesFor;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::imagesDirectoryUrl;
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::ManualPowerSaverMonitor;
using kiriview::TestSupport::ManualTimerScheduler;
using kiriview::TestSupport::powerSaverProviderFor;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::staticImageDataDecoder;
using kiriview::TestSupport::testImage;

using FakeCandidateProvider = kiriview::TestSupport::FakeImageDocumentPageCandidateProvider;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

kiriview::PowerSaverProvider noOpPowerSaverProvider()
{
    return kiriview::PowerSaverProvider {
        [](QObject *, kiriview::PowerSaverChangedCallback) {
            return std::unique_ptr<kiriview::PowerSaverStateMonitor>();
        },
    };
}

kiriview::ImageWorkerScheduler immediateWorkerScheduler()
{
    return kiriview::ImageWorkerScheduler([](QObject *, kiriview::ImageWorkerOperation work,
                                              kiriview::ImageWorkerCompletion completion) {
        work();
        completion();
    });
}

kiriview::ImageDataDecoder defaultImageDataDecoder()
{
    return [](const QByteArray &data, const kiriview::ImageDecodeRequest &request) {
        return kiriview::decodeImageData(data, request);
    };
}

kiriview::ImagePredecodeCoordinator createCoordinator(QObject *parent,
    FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader,
    kiriview::PowerSaverProvider powerSaverProvider, kiriview::TimerScheduler timerScheduler = {},
    kiriview::PredecodeThreadCountProvider threadCountProvider = {})
{
    kiriview::ImageDecodeDependencies dependencies
        = imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder());
    dependencies.workerScheduler = immediateWorkerScheduler();
    return kiriview::ImagePredecodeCoordinator(parent, candidateProvider.provider(),
        std::move(dependencies), std::move(powerSaverProvider), testCacheByteBudget,
        std::move(timerScheduler), std::move(threadCountProvider));
}

kiriview::ImagePredecodeCoordinator createCoordinator(QObject *parent,
    FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader,
    kiriview::ImageDataDecoder dataDecoder, kiriview::TimerScheduler timerScheduler = {},
    kiriview::PredecodeThreadCountProvider threadCountProvider = {})
{
    kiriview::ImageDecodeDependencies dependencies
        = imageDecodeDependenciesFor(dataLoader, std::move(dataDecoder));
    dependencies.workerScheduler = immediateWorkerScheduler();
    return kiriview::ImagePredecodeCoordinator(parent, candidateProvider.provider(),
        std::move(dependencies), noOpPowerSaverProvider(), testCacheByteBudget,
        std::move(timerScheduler), std::move(threadCountProvider));
}

kiriview::ImagePredecodeCoordinator createCoordinator(
    QObject *parent, FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    return createCoordinator(parent, candidateProvider, dataLoader, noOpPowerSaverProvider());
}

std::vector<kiriview::ImageDocumentPageCandidate> imageDocumentPageCandidates(int count)
{
    std::vector<kiriview::ImageDocumentPageCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        candidates.push_back(imageDocumentPageCandidate(indexedImageUrl(index)));
    }
    return candidates;
}

kiriview::StaticDisplayImagePayload displayTestImagePayload(
    const QImage &image, qreal firstDisplayPixelsPerSourcePixel = 0.0)
{
    const kiriview::DisplayImageQuality quality = firstDisplayPixelsPerSourcePixel > 0.0
        ? kiriview::DisplayImageQuality::FirstDisplay
        : kiriview::DisplayImageQuality::Exact;
    return staticDisplayTestImagePayload(image, image, firstDisplayPixelsPerSourcePixel, quality);
}

kiriview::ImagePredecodeCoordinator::Context predecodeContext(
    kiriview::DisplayedPredecodeImage primaryImage,
    std::optional<kiriview::DisplayedPredecodeImage> secondaryImage = std::nullopt,
    kiriview::ImageFirstDisplayDecodeContext firstDisplayContext = {}, int pageIndex = -1)
{
    const kiriview::DisplayedImageLocation currentLocation = primaryImage.location;
    std::vector<kiriview::DisplayedPredecodeImage> displayedImages;
    displayedImages.push_back(std::move(primaryImage));
    if (secondaryImage.has_value()) {
        displayedImages.push_back(std::move(*secondaryImage));
    }

    return kiriview::ImagePredecodeCoordinator::Context {
        currentLocation,
        std::move(displayedImages),
        firstDisplayContext,
        pageIndex,
        {},
    };
}

QString fixturePath(const QString &fileName)
{
    return QStringLiteral(KIRIVIEW_TEST_SOURCE_DIR "/../fixtures/") + fileName;
}

QByteArray fixtureData(const QString &fileName)
{
    QFile file(fixturePath(fileName));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
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
    void regularPredecodeWindowKeepsTwoPreviousAndTwoNextPages();
    void directoryCollectionStartsTwoBackgroundDecodes();
    void candidateListingFailureStartsEmptyFallbackWindow();
    void archiveThreadCountProviderControlsParallelLoadLimit();
    void animatedBackgroundDecodeIsNotCachedAsStaticPredecodedImage_data();
    void animatedBackgroundDecodeIsNotCachedAsStaticPredecodedImage();
    void staleGenerationDecodeIsIgnored();
    void rapidNavigationDebouncesSkippedPagePredecode();
    void powerSaverMonitorSuppressesAndReschedulesPredecode();
    void cancelSuppressesPendingDecode();
};

void TestImagePredecodeCoordinator::scheduleCachesDisplayedImageAndPredecodesWindow()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    kiriview::ImagePredecodeCoordinator coordinator
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
        kiriview::DisplayedPredecodeImage {
            kiriview::DisplayedImageLocation::fromUrl(displayedUrl),
            true,
            displayTestImagePayload(displayedImage, 0.5),
        },
        std::nullopt, kiriview::ImageFirstDisplayDecodeContext { QSize(640, 480) }));

    const std::optional<kiriview::PredecodedImage> displayed
        = coordinator.findPredecodedImage(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->location.imageUrl(), displayedUrl);
    QCOMPARE(displayed->displayImage.quality, kiriview::DisplayImageQuality::FirstDisplay);
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
    kiriview::ImagePredecodeCoordinator coordinator
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
        kiriview::DisplayedPredecodeImage {
            kiriview::DisplayedImageLocation::fromUrl(primaryUrl),
            true,
            displayTestImagePayload(testImage(), 0.5),
        },
        std::make_optional(kiriview::DisplayedPredecodeImage {
            kiriview::DisplayedImageLocation::fromUrl(secondaryUrl),
            true,
            displayTestImagePayload(testImage(), 0.75),
        })));

    const std::optional<kiriview::PredecodedImage> primary
        = coordinator.findPredecodedImage(primaryUrl);
    QVERIFY(primary.has_value());
    QCOMPARE(primary->displayImage.displayPixelsPerSourcePixel, 0.5);

    const std::optional<kiriview::PredecodedImage> secondary
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
    kiriview::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    coordinator.schedule(kiriview::ImagePredecodeCoordinator::Context {});

    QCOMPARE(dataLoader.loadCount(), std::size_t(0));
}

void TestImagePredecodeCoordinator::archivePredecodeKeepsOpenedCollectionScopeContext()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    kiriview::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> openedCollectionScope
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(openedCollectionScope.has_value());
    const QUrl displayedUrl
        = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setOpenedCollectionCandidates(openedCollectionScope->rootUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
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
    const std::optional<kiriview::PredecodedImage> predecoded
        = coordinator.findPredecodedImage(nextUrl);
    QVERIFY(predecoded.has_value());
    QCOMPARE(predecoded->location.openedCollectionScopeRootUrl(), openedCollectionScope->rootUrl());
}

void TestImagePredecodeCoordinator::regularPredecodeWindowKeepsTwoPreviousAndTwoNextPages()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    kiriview::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl displayedUrl = indexedImageUrl(5);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(), imageDocumentPageCandidates(15));

    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(displayedUrl),
        false,
        displayTestImagePayload(testImage()),
    }));

    const std::vector<QUrl> expectedLoadOrder {
        indexedImageUrl(6),
        indexedImageUrl(4),
        indexedImageUrl(7),
        indexedImageUrl(3),
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
    kiriview::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const kiriview::OpenedCollectionScopeLocation directoryCollection
        = kiriview::OpenedCollectionScopeLocation::fromUrls(imagesDirectoryUrl(),
            imagesDirectoryUrl(), kiriview::OpenedCollectionScopeKind::Directory);
    const QUrl displayedUrl = indexedImageUrl(5);
    candidateProvider.setOpenedCollectionCandidates(
        directoryCollection.rootUrl(), imageDocumentPageCandidates(15));

    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
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
    ManualTimerScheduler timerScheduler;
    kiriview::ImagePredecodeCoordinator coordinator = createCoordinator(
        this, candidateProvider, dataLoader, noOpPowerSaverProvider(), timerScheduler.scheduler());

    const QUrl displayedUrl = indexedImageUrl(5);
    candidateProvider.setDirectoryImageError(
        imagesDirectoryUrl(), QStringLiteral("candidate listing failed"));

    timerScheduler.advanceTo(1000);
    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(displayedUrl),
        true,
        displayTestImagePayload(testImage()),
    }));

    timerScheduler.timerAt(0).fire();

    QCOMPARE(dataLoader.loadCount(), std::size_t(0));
    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
}

void TestImagePredecodeCoordinator::archiveThreadCountProviderControlsParallelLoadLimit()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualTimerScheduler timerScheduler;
    kiriview::ImagePredecodeCoordinator coordinator = createCoordinator(this, candidateProvider,
        dataLoader, noOpPowerSaverProvider(), timerScheduler.scheduler(), []() { return 8; });

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> openedCollectionScope
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(openedCollectionScope.has_value());
    const QUrl displayedUrl
        = archivePageUrl(openedCollectionScope->rootUrl(), QStringLiteral("05.png"));
    std::vector<kiriview::ImageDocumentPageCandidate> candidates;
    for (int index = 0; index < 15; ++index) {
        candidates.push_back(
            imageDocumentPageCandidate(archivePageUrl(openedCollectionScope->rootUrl(),
                QStringLiteral("%1.png").arg(index, 2, 10, QLatin1Char('0')))));
    }
    candidateProvider.setOpenedCollectionCandidates(
        openedCollectionScope->rootUrl(), std::move(candidates));

    timerScheduler.advanceTo(1000);
    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
            displayedUrl, *openedCollectionScope),
        false,
        displayTestImagePayload(testImage()),
    }));

    timerScheduler.timerAt(0).fire();

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(4));
}

void TestImagePredecodeCoordinator::
    animatedBackgroundDecodeIsNotCachedAsStaticPredecodedImage_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::newRow("apng") << QStringLiteral("animated-smoke.apng");
    QTest::newRow("webp") << QStringLiteral("animated-smoke.webp");
    QTest::newRow("jxl") << QStringLiteral("animated-smoke.jxl");
    QTest::newRow("heif-sequence") << QStringLiteral("heif-sequence-alpha.heics");
}

void TestImagePredecodeCoordinator::animatedBackgroundDecodeIsNotCachedAsStaticPredecodedImage()
{
    QFETCH(QString, fileName);

    const QByteArray imageData = fixtureData(fileName);
    QVERIFY2(!imageData.isEmpty(), qPrintable(fixturePath(fileName)));

    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    kiriview::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader, defaultImageDataDecoder());

    const QUrl displayedUrl = indexedImageUrl(0);
    const QUrl animatedUrl = indexedImageUrl(1);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(animatedUrl),
        });

    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(displayedUrl),
        false,
        displayTestImagePayload(testImage()),
    }));

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, animatedUrl);
    dataLoader.finishFrontLoad(imageData);

    QVERIFY(!coordinator.findPredecodedImage(animatedUrl).has_value());
}

void TestImagePredecodeCoordinator::staleGenerationDecodeIsIgnored()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    kiriview::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    candidateProvider.setDirectoryImages(imagesDirectoryUrl(), imageDocumentPageCandidates(5));

    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(indexedImageUrl(0)),
        false,
        displayTestImagePayload(testImage()),
    }));
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, indexedImageUrl(1));

    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(indexedImageUrl(2)),
        false,
        displayTestImagePayload(testImage()),
    }));

    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("stale"));
    QVERIFY(!coordinator.findPredecodedImage(indexedImageUrl(1)).has_value());
}

void TestImagePredecodeCoordinator::rapidNavigationDebouncesSkippedPagePredecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualTimerScheduler timerScheduler;
    kiriview::ImagePredecodeCoordinator coordinator = createCoordinator(
        this, candidateProvider, dataLoader, noOpPowerSaverProvider(), timerScheduler.scheduler());

    candidateProvider.setDirectoryImages(imagesDirectoryUrl(), imageDocumentPageCandidates(10));

    const auto schedulePage = [&coordinator](int pageIndex) {
        coordinator.schedule(predecodeContext(
            kiriview::DisplayedPredecodeImage {
                kiriview::DisplayedImageLocation::fromUrl(indexedImageUrl(pageIndex)),
                false,
                displayTestImagePayload(testImage()),
            },
            std::nullopt, {}, pageIndex));
    };

    timerScheduler.advanceTo(1000);
    schedulePage(0);
    timerScheduler.advanceTo(1100);
    schedulePage(1);
    timerScheduler.advanceTo(1200);
    schedulePage(2);

    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    timerScheduler.timerAt(0).fire();
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));
    timerScheduler.timerAt(1).fire();

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
    ManualTimerScheduler timerScheduler;
    kiriview::ImagePredecodeCoordinator coordinator = createCoordinator(this, candidateProvider,
        dataLoader, powerSaverProviderFor(powerSaverMonitor, true), timerScheduler.scheduler());
    QVERIFY(powerSaverMonitor != nullptr);
    QVERIFY(coordinator.powerSaverEnabled());

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    timerScheduler.advanceTo(1000);
    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(displayedUrl),
        true,
        displayTestImagePayload(testImage()),
    }));

    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    timerScheduler.advanceTo(1200);
    powerSaverMonitor->setPowerSaverEnabled(false);
    QVERIFY(!coordinator.powerSaverEnabled());
    QVERIFY(timerScheduler.timerAt(0).active());
    timerScheduler.timerAt(0).fire();
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);

    powerSaverMonitor->setPowerSaverEnabled(true);
    QVERIFY(coordinator.powerSaverEnabled());
    QVERIFY(dataLoader.frontLoad().canceled);
    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("stale"));
    QVERIFY(!coordinator.findPredecodedImage(nextUrl).has_value());
    QVERIFY(coordinator.findPredecodedImage(displayedUrl).has_value());
}

void TestImagePredecodeCoordinator::cancelSuppressesPendingDecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    kiriview::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    const QImage displayedImage = testImage();
    coordinator.schedule(predecodeContext(kiriview::DisplayedPredecodeImage {
        kiriview::DisplayedImageLocation::fromUrl(displayedUrl),
        false,
        displayTestImagePayload(displayedImage),
    }));

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    coordinator.cancel();
    QVERIFY(dataLoader.frontLoad().canceled);

    dataLoader.finishFrontLoad(QByteArrayLiteral("next"));
    QVERIFY(!coordinator.findPredecodedImage(nextUrl).has_value());
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
}

QTEST_GUILESS_MAIN(TestImagePredecodeCoordinator)

#include "test_imagepredecodecoordinator.moc"
