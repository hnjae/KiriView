// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentpredecodecontroller.h"
#include "document/imagedocumentstate.h"
#include "image_test_support.h"
#include "presentation/imagepresentationcontroller.h"
#include "rendering/imagerendering.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>
#include <memory>
#include <optional>
#include <utility>

namespace {
using KiriView::TestSupport::imageDecodeDependenciesFor;
using KiriView::TestSupport::imageDocumentPageCandidate;
using KiriView::TestSupport::imagesDirectoryUrl;
using KiriView::TestSupport::indexedImageUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::ManualPowerSaverMonitor;
using KiriView::TestSupport::powerSaverProviderFor;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageDocumentPageCandidateProvider;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

KiriView::ImageCacheBudgets testCacheBudgets()
{
    return KiriView::ImageCacheBudgets {
        testCacheByteBudget,
        testCacheByteBudget,
    };
}

KiriView::ImagePresentationController createPresentationController(QObject *parent)
{
    return KiriView::ImagePresentationController(
        parent,
        []() {
            return KiriView::ImageDocumentRenderContext {
                2.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        {}, testCacheBudgets());
}
}

class TestImageDocumentPredecodeController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void scheduleAdjacentImagePredecodeUsesPresentationSnapshot();
    void scheduleAdjacentImagePredecodeWithoutSnapshotCancelsActivePredecode();
    void powerSaverSuppressesBackgroundPredecodeAndReschedulesWhenDisabled();
};

void TestImageDocumentPredecodeController::scheduleAdjacentImagePredecodeUsesPresentationSnapshot()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageDocumentState state;
    KiriView::ImagePresentationController presentation = createPresentationController(this);
    KiriView::ImageDocumentPredecodeController controller(this, state, presentation,
        candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()), testCacheByteBudget);

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    presentation.setViewportSize(QSizeF(320.0, 240.0));
    presentation.setStaticImage(
        staticTestImagePayload(testImage(QSize(10, 8)), KiriView::StaticImageDisplayHints { 0.5 }),
        true);

    controller.scheduleAdjacentImagePredecode();

    const std::optional<KiriView::PredecodedImage> displayed
        = controller.findPredecodedImage(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->staticImage.displayHints.firstDisplayPixelsPerSourcePixel, 0.5);

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(640, 480));
}

void TestImageDocumentPredecodeController::
    scheduleAdjacentImagePredecodeWithoutSnapshotCancelsActivePredecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    KiriView::ImageDocumentState state;
    KiriView::ImagePresentationController presentation = createPresentationController(this);
    KiriView::ImageDocumentPredecodeController controller(this, state, presentation,
        candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()), testCacheByteBudget);

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    presentation.setStaticImage(staticTestImagePayload(testImage()), false);
    controller.scheduleAdjacentImagePredecode();
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));

    presentation.clearImage();
    controller.scheduleAdjacentImagePredecode();

    QVERIFY(dataLoader.frontLoad().canceled);
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QVERIFY(!controller.findPredecodedImage(nextUrl).has_value());
}

void TestImageDocumentPredecodeController::
    powerSaverSuppressesBackgroundPredecodeAndReschedulesWhenDisabled()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualPowerSaverMonitor *powerSaverMonitor = nullptr;
    KiriView::ImageDocumentState state;
    KiriView::ImagePresentationController presentation = createPresentationController(this);
    KiriView::ImageDocumentPredecodeController controller(this, state, presentation,
        candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()), testCacheByteBudget, {},
        powerSaverProviderFor(powerSaverMonitor, true));
    QVERIFY(powerSaverMonitor != nullptr);

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    state.setDisplayedImageLocation(KiriView::DisplayedImageLocation::fromUrl(displayedUrl));
    presentation.setStaticImage(staticTestImagePayload(testImage()), true);

    controller.scheduleAdjacentImagePredecode();

    QVERIFY(controller.findPredecodedImage(displayedUrl).has_value());
    QTest::qWait(250);
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    powerSaverMonitor->setPowerSaverEnabled(false);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);

    powerSaverMonitor->setPowerSaverEnabled(true);
    QVERIFY(dataLoader.frontLoad().canceled);
    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("stale"));
    QTest::qWait(50);
    QVERIFY(!controller.findPredecodedImage(nextUrl).has_value());
    QVERIFY(controller.findPredecodedImage(displayedUrl).has_value());
}

QTEST_GUILESS_MAIN(TestImageDocumentPredecodeController)

#include "test_imagedocumentpredecodecontroller.moc"
