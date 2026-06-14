// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentpredecodecontroller.h"
#include "document/imagedocumentstate.h"
#include "image_test_support.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagepresentationruntime.h"
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
using kiriview::TestSupport::imageDecodeDependenciesFor;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::imagesDirectoryUrl;
using kiriview::TestSupport::indexedImageUrl;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::ManualPowerSaverMonitor;
using kiriview::TestSupport::ManualTimerScheduler;
using kiriview::TestSupport::powerSaverProviderFor;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::staticImageDataDecoder;
using kiriview::TestSupport::testImage;

using FakeCandidateProvider = kiriview::TestSupport::FakeImageDocumentPageCandidateProvider;

constexpr qsizetype testCacheByteBudget = 1024 * 1024;

kiriview::ImageCacheBudgets testCacheBudgets()
{
    return kiriview::ImageCacheBudgets {
        testCacheByteBudget,
        testCacheByteBudget,
    };
}

kiriview::ImageDocumentRenderContext renderContext()
{
    return kiriview::ImageDocumentRenderContext {
        2.0,
        kiriview::fallbackTextureSizeMax,
    };
}

kiriview::ImagePageSurfaceController createPageSurfaceController(QObject *parent)
{
    return kiriview::ImagePageSurfaceController(parent, {}, testCacheBudgets());
}

kiriview::ImagePresentationRuntime createPresentationRuntime()
{
    return kiriview::ImagePresentationRuntime(renderContext);
}

kiriview::ImageWorkerScheduler immediateWorkerScheduler()
{
    return kiriview::ImageWorkerScheduler([](QObject *, kiriview::ImageWorkerOperation work,
                                              kiriview::ImageWorkerCompletion completion) {
        work();
        completion();
    });
}

kiriview::StaticDisplayImagePayload displayTestImagePayload(
    const QImage &image, qreal firstDisplayPixelsPerSourcePixel = 0.0)
{
    const kiriview::DisplayImageQuality quality = firstDisplayPixelsPerSourcePixel > 0.0
        ? kiriview::DisplayImageQuality::FirstDisplay
        : kiriview::DisplayImageQuality::Exact;
    return staticDisplayTestImagePayload(image, image, firstDisplayPixelsPerSourcePixel, quality);
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
    kiriview::ImageDocumentState state;
    kiriview::ImagePageSurfaceController pageSurface = createPageSurfaceController(this);
    kiriview::ImagePresentationRuntime presentationRuntime = createPresentationRuntime();
    kiriview::ImageDocumentPredecodeController controller(this, state, pageSurface,
        presentationRuntime, candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()), testCacheByteBudget);

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(displayedUrl));
    presentationRuntime.setViewportSize(QSizeF(320.0, 240.0));
    pageSurface.setStaticDisplayImage(
        displayTestImagePayload(testImage(QSize(10, 8)), 0.5), true, renderContext());

    controller.scheduleAdjacentImagePredecode();

    const std::optional<kiriview::PredecodedImage> displayed
        = controller.findPredecodedImage(displayedUrl);
    QVERIFY(displayed.has_value());
    QCOMPARE(displayed->displayImage.quality, kiriview::DisplayImageQuality::FirstDisplay);
    QCOMPARE(displayed->displayImage.displayPixelsPerSourcePixel, 0.5);

    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(640, 480));
}

void TestImageDocumentPredecodeController::
    scheduleAdjacentImagePredecodeWithoutSnapshotCancelsActivePredecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    kiriview::ImageDocumentState state;
    kiriview::ImagePageSurfaceController pageSurface = createPageSurfaceController(this);
    kiriview::ImagePresentationRuntime presentationRuntime = createPresentationRuntime();
    kiriview::ImageDocumentPredecodeController controller(this, state, pageSurface,
        presentationRuntime, candidateProvider.provider(),
        imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder()), testCacheByteBudget);

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(displayedUrl));
    pageSurface.setStaticDisplayImage(displayTestImagePayload(testImage()), false, renderContext());
    controller.scheduleAdjacentImagePredecode();
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));

    pageSurface.clearImage();
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
    ManualTimerScheduler timerScheduler;
    kiriview::ImageDocumentState state;
    kiriview::ImagePageSurfaceController pageSurface = createPageSurfaceController(this);
    kiriview::ImagePresentationRuntime presentationRuntime = createPresentationRuntime();
    kiriview::ImageDecodeDependencies decodeDependencies
        = imageDecodeDependenciesFor(dataLoader, staticImageDataDecoder());
    decodeDependencies.workerScheduler = immediateWorkerScheduler();
    kiriview::ImageDocumentPredecodeController controller(this, state, pageSurface,
        presentationRuntime, candidateProvider.provider(), std::move(decodeDependencies),
        testCacheByteBudget, {}, powerSaverProviderFor(powerSaverMonitor, true), true,
        timerScheduler.scheduler(), []() { return 4; });
    QVERIFY(powerSaverMonitor != nullptr);

    const QUrl displayedUrl = indexedImageUrl(1);
    const QUrl nextUrl = indexedImageUrl(2);
    candidateProvider.setDirectoryImages(imagesDirectoryUrl(),
        {
            imageDocumentPageCandidate(displayedUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    state.setDisplayedImageLocation(kiriview::DisplayedImageLocation::fromUrl(displayedUrl));
    pageSurface.setStaticDisplayImage(displayTestImagePayload(testImage()), true, renderContext());

    timerScheduler.advanceTo(1000);
    controller.scheduleAdjacentImagePredecode();

    QVERIFY(controller.findPredecodedImage(displayedUrl).has_value());
    QCOMPARE(dataLoader.loadCount(), std::size_t(0));

    timerScheduler.advanceTo(1200);
    powerSaverMonitor->setPowerSaverEnabled(false);
    QVERIFY(timerScheduler.timerAt(0).active());
    timerScheduler.timerAt(0).fire();
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().url, nextUrl);

    powerSaverMonitor->setPowerSaverEnabled(true);
    QVERIFY(dataLoader.frontLoad().canceled);
    dataLoader.deliverFrontLoadDataIgnoringCancellation(QByteArrayLiteral("stale"));
    QVERIFY(!controller.findPredecodedImage(nextUrl).has_value());
    QVERIFY(controller.findPredecodedImage(displayedUrl).has_value());
}

QTEST_GUILESS_MAIN(TestImageDocumentPredecodeController)

#include "test_imagedocumentpredecodecontroller.moc"
