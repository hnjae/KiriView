// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "image_test_support.h"
#include "navigation/imagecontainer.h"
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
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::imagesDirectoryUrl;
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::ManualPowerSaverMonitor;
using KiriView::TestSupport::powerSaverProviderFor;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

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
        std::move(powerSaverProvider));
}

KiriView::ImagePredecodeCoordinator createCoordinator(
    QObject *parent, FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader)
{
    return createCoordinator(parent, candidateProvider, dataLoader, noOpPowerSaverProvider());
}

std::vector<KiriView::ImageNavigationCandidate> imageCandidates(int count)
{
    std::vector<KiriView::ImageNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        candidates.push_back(imageCandidate(indexedImageUrl(index)));
    }
    return candidates;
}
}

class TestImagePredecodeCoordinator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scheduleCachesDisplayedImageAndPredecodesWindow();
    void scheduleCachesVisibleSpreadPagesAndSkipsSecondaryPredecode();
    void scheduleRejectsInvalidDisplayedContext();
    void archivePredecodeKeepsArchiveDocumentContext();
    void regularPredecodeWindowKeepsOnePreviousAndTwoNextPages();
    void directoryDocumentStartsTwoBackgroundDecodes();
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
            imageCandidate(previousUrl),
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    const QImage displayedImage = testImage();
    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
            true,
            staticTestImagePayload(displayedImage, KiriView::StaticImageDisplayHints { 0.5 }),
        },
        std::nullopt,
        KiriView::ImageFirstDisplayDecodeContext { QSize(640, 480) },
    });

    const std::optional<KiriView::PredecodedImage> displayed
        = coordinator.findPredecodedImage(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->location.imageUrl(), displayedUrl);
    QCOMPARE(displayed->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.5);

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
            imageCandidate(primaryUrl),
            imageCandidate(secondaryUrl),
            imageCandidate(nextUrl),
        });

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(primaryUrl),
            true,
            staticTestImagePayload(testImage(), KiriView::StaticImageDisplayHints { 0.5 }),
        },
        std::make_optional(KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(secondaryUrl),
            true,
            staticTestImagePayload(testImage(), KiriView::StaticImageDisplayHints { 0.75 }),
        }),
    });

    const std::optional<KiriView::PredecodedImage> primary
        = coordinator.findPredecodedImage(primaryUrl);
    QVERIFY(primary.has_value());
    QCOMPARE(primary->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.5);

    const std::optional<KiriView::PredecodedImage> secondary
        = coordinator.findPredecodedImage(secondaryUrl);
    QVERIFY(secondary.has_value());
    QCOMPARE(secondary->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.75);

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

void TestImagePredecodeCoordinator::archivePredecodeKeepsArchiveDocumentContext()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl displayedUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromArchiveDocument(displayedUrl, *archiveDocument),
            false,
            staticTestImagePayload(testImage()),
        },
    });

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.frontLoad().archiveDocument.rootUrl(), archiveDocument->rootUrl());
    dataLoader.finishFrontLoad(QByteArrayLiteral("next"));

    QTRY_VERIFY(coordinator.findPredecodedImage(nextUrl).has_value());
    const std::optional<KiriView::PredecodedImage> predecoded
        = coordinator.findPredecodedImage(nextUrl);
    QVERIFY(predecoded.has_value());
    QCOMPARE(predecoded->location.archiveDocumentRootUrl(), archiveDocument->rootUrl());
}

void TestImagePredecodeCoordinator::regularPredecodeWindowKeepsOnePreviousAndTwoNextPages()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const QUrl displayedUrl = indexedImageUrl(5);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(), imageCandidates(15));

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
            false,
            staticTestImagePayload(testImage()),
        },
    });

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

void TestImagePredecodeCoordinator::directoryDocumentStartsTwoBackgroundDecodes()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    const KiriView::ArchiveDocumentLocation directoryDocument
        = KiriView::ArchiveDocumentLocation::fromUrls(
            imagesDirectoryUrl(), imagesDirectoryUrl(), KiriView::ArchiveDocumentKind::Directory);
    const QUrl displayedUrl = indexedImageUrl(5);
    candidateProvider.setArchiveImages(directoryDocument.rootUrl(), imageCandidates(15));

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromArchiveDocument(displayedUrl, directoryDocument),
            false,
            staticTestImagePayload(testImage()),
        },
    });

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.frontLoad().url, indexedImageUrl(6));
    QCOMPARE(dataLoader.backLoad().url, indexedImageUrl(4));
}

void TestImagePredecodeCoordinator::staleGenerationDecodeIsIgnored()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImagePredecodeCoordinator coordinator
        = createCoordinator(this, candidateProvider, dataLoader);

    candidateProvider.setDirectoryImages(imagesDirectoryUrl(), imageCandidates(5));

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(indexedImageUrl(0)),
            false,
            staticTestImagePayload(testImage()),
        },
    });
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, indexedImageUrl(1));

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(indexedImageUrl(2)),
            false,
            staticTestImagePayload(testImage()),
        },
    });

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

    candidateProvider.setDirectoryImages(imagesDirectoryUrl(), imageCandidates(10));

    const auto schedulePage = [&coordinator](int pageIndex) {
        coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
            KiriView::DisplayedPredecodeImage {
                KiriView::DisplayedImageLocation::fromUrl(indexedImageUrl(pageIndex)),
                false,
                staticTestImagePayload(testImage()),
            },
            std::nullopt,
            {},
            pageIndex,
        });
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
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
            true,
            staticTestImagePayload(testImage()),
        },
    });

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
            imageCandidate(displayedUrl),
            imageCandidate(nextUrl),
        });

    const QImage displayedImage = testImage();
    coordinator.schedule(KiriView::ImagePredecodeCoordinator::Context {
        KiriView::DisplayedPredecodeImage {
            KiriView::DisplayedImageLocation::fromUrl(displayedUrl),
            false,
            staticTestImagePayload(displayedImage),
        },
    });

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
