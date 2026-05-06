// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagerendering.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::comicBookContainerCandidate;
using KiriView::TestSupport::dataLoaderFor;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticDecodedTestImage;
using KiriView::TestSupport::testImage;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

KiriView::DecodedImageResult decodeTestImageData(
    const QByteArray &, const KiriView::ImageDecodeRequest &)
{
    return KiriView::successfulDecodedImageResult(staticDecodedTestImage(testImage(2)));
}

KiriView::DecodedImageResult decodeBadDataAsFailure(
    const QByteArray &data, const KiriView::ImageDecodeRequest &request)
{
    if (data == QByteArrayLiteral("bad")) {
        return KiriView::DecodedImageFailure { QStringLiteral("decode failed") };
    }

    return decodeTestImageData(data, request);
}

KiriView::DecodedImageResult staticDecodedImageWithPreview(const QSize &sourceSize,
    const QSize &previewSize, KiriView::StaticImageDisplayHints displayHints = {})
{
    return KiriView::successfulDecodedImageResult(KiriView::StaticDecodedImage {
        KiriView::StaticImagePayload {
            std::make_shared<KiriView::TestSupport::TestImageTileSource>(testImage(sourceSize)),
            testImage(previewSize),
            displayHints,
        },
    });
}

KiriView::ImageAsyncDependencies dependenciesFor(FakeCandidateProvider &candidateProvider,
    ManualImageDataLoader &dataLoader,
    KiriView::ImageDecodeJob::DataDecoder dataDecoder = decodeTestImageData)
{
    return KiriView::ImageAsyncDependencies {
        candidateProvider.provider(),
        dataLoaderFor(dataLoader),
        std::move(dataDecoder),
    };
}

std::unique_ptr<KiriView::ImageDocumentController> createController(QObject *parent,
    FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader,
    KiriView::ImageDecodeJob::DataDecoder dataDecoder = decodeTestImageData,
    int maximumTextureSize = KiriView::fallbackTextureSizeMax, qreal devicePixelRatio = 1.0)
{
    return std::make_unique<KiriView::ImageDocumentController>(
        parent,
        [maximumTextureSize, devicePixelRatio]() {
            return KiriView::ImageDocumentRenderContext {
                devicePixelRatio,
                maximumTextureSize,
            };
        },
        KiriView::ImageDocumentController::ChangeCallback {},
        dependenciesFor(candidateProvider, dataLoader, std::move(dataDecoder)));
}

void finishLoad(ManualImageDataLoader &dataLoader)
{
    dataLoader.loads.back()->dataCallback(QByteArrayLiteral("ok"));
}

std::size_t staticTileCount(const KiriView::ImageDocumentController &controller)
{
    std::shared_ptr<KiriView::DisplayedImageSurface> surface = controller.imageSurface();
    auto *staticSurface
        = surface == nullptr ? nullptr : std::get_if<KiriView::StaticTileSurface>(surface.get());
    return staticSurface == nullptr ? 0 : staticSurface->tiles().size();
}
}

class TestImageDocumentController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialLoadSuccessUpdatesDocumentState();
    void imageLoadsUsePhysicalViewportForFirstDisplayDecode();
    void directoryImageNavigationResetsManualZoom();
    void archiveDocumentPageNavigationPreservesManualZoom();
    void siblingArchiveNavigationResetsManualZoom();
    void smallStaticImageUsesFullImageSurface();
    void largeStaticImageUsesTiledSurface();
    void tiledStaticImageRefinesNewVisibleTilesAfterPan();
    void firstDisplayDefersTilesUntilZoomNeedsMoreDetail();
    void smallFullImageSurfaceStillSchedulesAdjacentPredecode();
    void replacementLoadFailureKeepsDisplayedImage();
    void decodedReplacementFailureSchedulesRecoveryPredecodeOnce();
    void emptyContainerNavigationClearsImageAndSelectsContainer();
};

void TestImageDocumentController::initialLoadSuccessUpdatesDocumentState()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);

    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Loading);
    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->sourceUrl(), imageUrl);
    QCOMPARE(controller->displayedUrl(), imageUrl);
    QCOMPARE(controller->imageSize(), QSize(2, 1));
    QCOMPARE(controller->currentPageNumber(), 1);
    QCOMPARE(controller->imageCount(), 1);
    QVERIFY(!controller->containerNavigationAvailable());
    QVERIFY(!controller->image().isNull());
}

void TestImageDocumentController::imageLoadsUsePhysicalViewportForFirstDisplayDecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl nextImageUrl = localUrl(QStringLiteral("/images/02.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
            imageCandidate(nextImageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(this,
        candidateProvider, dataLoader, decodeTestImageData, KiriView::fallbackTextureSizeMax, 2.0);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);

    QCOMPARE(dataLoader.loads.size(), std::size_t(1));
    QCOMPARE(dataLoader.loads.front()->firstDisplay.physicalViewportSize, QSize(800, 600));
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QTRY_COMPARE(dataLoader.loads.size(), std::size_t(2));
    QCOMPARE(dataLoader.loads.back()->url, nextImageUrl);
    QCOMPARE(dataLoader.loads.back()->firstDisplay.physicalViewportSize, QSize(800, 600));
}

void TestImageDocumentController::directoryImageNavigationResetsManualZoom()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl firstImageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondImageUrl = localUrl(QStringLiteral("/images/02.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(firstImageUrl),
            imageCandidate(secondImageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    controller->setZoomPercent(150.0);
    QCOMPARE(controller->zoomMode(), KiriView::ImageZoomMode::Manual);

    const std::size_t loadCountBeforeNavigation = dataLoader.loads.size();
    controller->openNextImage();

    QTRY_COMPARE(dataLoader.loads.size(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.loads.back()->url, secondImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), secondImageUrl);
    QCOMPARE(controller->zoomMode(), KiriView::ImageZoomMode::Fit);
}

void TestImageDocumentController::archiveDocumentPageNavigationPreservesManualZoom()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(firstPageUrl),
            imageCandidate(secondPageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->displayedUrl(), firstPageUrl);
    controller->setZoomPercent(150.0);
    QCOMPARE(controller->zoomMode(), KiriView::ImageZoomMode::Manual);

    const std::size_t loadCountBeforeNavigation = dataLoader.loads.size();
    controller->openNextImage();

    QTRY_COMPARE(dataLoader.loads.size(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.loads.back()->url, secondPageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), secondPageUrl);
    QCOMPARE(controller->zoomMode(), KiriView::ImageZoomMode::Manual);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(controller->zoomPercent(), 150.0));
}

void TestImageDocumentController::siblingArchiveNavigationResetsManualZoom()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl firstArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl secondArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> firstArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(firstArchiveUrl);
    const std::optional<KiriView::ArchiveDocumentLocation> secondArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(secondArchiveUrl);
    QVERIFY(firstArchiveDocument.has_value());
    QVERIFY(secondArchiveDocument.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(firstArchiveDocument->rootUrl(),
        {
            imageCandidate(firstPageUrl),
        });
    candidateProvider.setArchiveImages(secondArchiveDocument->rootUrl(),
        {
            imageCandidate(secondPageUrl),
        });
    candidateProvider.setContainerCandidates(localUrl(QStringLiteral("/books/")),
        {
            comicBookContainerCandidate(firstArchiveUrl),
            comicBookContainerCandidate(secondArchiveUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(firstArchiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->displayedUrl(), firstPageUrl);
    controller->setZoomPercent(150.0);
    QCOMPARE(controller->zoomMode(), KiriView::ImageZoomMode::Manual);

    const std::size_t loadCountBeforeNavigation = dataLoader.loads.size();
    controller->openNextContainer();

    QTRY_COMPARE(dataLoader.loads.size(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.loads.back()->url, secondPageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), secondPageUrl);
    QCOMPARE(controller->zoomMode(), KiriView::ImageZoomMode::Fit);
}

void TestImageDocumentController::smallStaticImageUsesFullImageSurface()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/small.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(QSize(1024, 1), QSize(1024, 1));
        },
        KiriView::fallbackTextureSizeMax);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    std::shared_ptr<KiriView::DisplayedImageSurface> surface = controller->imageSurface();
    QVERIFY(surface != nullptr);
    QVERIFY(std::holds_alternative<KiriView::LegacyFrameSurface>(*surface));
    QCOMPARE(controller->imageSize(), QSize(1024, 1));
}

void TestImageDocumentController::largeStaticImageUsesTiledSurface()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/large.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(
                QSize(KiriView::imageBlockingDisplayLongEdgeMax + 1, 1),
                QSize(KiriView::imageBlockingDisplayLongEdgeMax, 1));
        },
        KiriView::fallbackTextureSizeMax);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    std::shared_ptr<KiriView::DisplayedImageSurface> surface = controller->imageSurface();
    QVERIFY(surface != nullptr);
    auto *staticSurface = std::get_if<KiriView::StaticTileSurface>(surface.get());
    QVERIFY(staticSurface != nullptr);
    QCOMPARE(staticSurface->imageSize(), QSize(KiriView::imageBlockingDisplayLongEdgeMax + 1, 1));
    QCOMPARE(staticSurface->preview().size(), QSize(KiriView::imageBlockingDisplayLongEdgeMax, 1));
}

void TestImageDocumentController::tiledStaticImageRefinesNewVisibleTilesAfterPan()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/large.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(
                QSize(KiriView::imageBlockingDisplayLongEdgeMax + 1, 512),
                QSize(KiriView::imageBlockingDisplayLongEdgeMax, 512));
        },
        KiriView::fallbackTextureSizeMax);
    controller->setViewportSize(QSizeF(512.0, 512.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    controller->setZoomPercent(100.0);
    controller->setVisibleItemRect(QRectF(0.0, 0.0, 512.0, 512.0));
    QTRY_VERIFY(staticTileCount(*controller) > std::size_t(0));
    const std::size_t initialTileCount = staticTileCount(*controller);

    controller->setVisibleItemRect(QRectF(1536.0, 0.0, 512.0, 512.0));

    QTRY_VERIFY(staticTileCount(*controller) > initialTileCount);
}

void TestImageDocumentController::firstDisplayDefersTilesUntilZoomNeedsMoreDetail()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/large.jpg"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(
                QSize(2048, 2048), QSize(512, 512), KiriView::StaticImageDisplayHints { 0.25 });
        },
        KiriView::fallbackTextureSizeMax);
    controller->setViewportSize(QSizeF(512.0, 512.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    controller->setVisibleItemRect(QRectF(0.0, 0.0, 512.0, 512.0));
    QTest::qWait(50);
    QCOMPARE(staticTileCount(*controller), std::size_t(0));

    controller->setZoomPercent(100.0);

    QTRY_VERIFY(staticTileCount(*controller) > std::size_t(0));
}

void TestImageDocumentController::smallFullImageSurfaceStillSchedulesAdjacentPredecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl nextImageUrl = localUrl(QStringLiteral("/images/02.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
            imageCandidate(nextImageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(QSize(1024, 1), QSize(1024, 1));
        },
        KiriView::fallbackTextureSizeMax);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    std::shared_ptr<KiriView::DisplayedImageSurface> surface = controller->imageSurface();
    QVERIFY(surface != nullptr);
    QVERIFY(std::holds_alternative<KiriView::LegacyFrameSurface>(*surface));
    QTRY_COMPARE(dataLoader.loads.size(), std::size_t(2));
    QCOMPARE(dataLoader.loads.back()->url, nextImageUrl);
}

void TestImageDocumentController::replacementLoadFailureKeepsDisplayedImage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl missingUrl = localUrl(QStringLiteral("/images/missing.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
            imageCandidate(missingUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    const quint64 displayedRevision = controller->imageRevision();
    const std::size_t loadCountBeforeReplacement = dataLoader.loads.size();

    controller->setSourceUrl(missingUrl);
    QCOMPARE(dataLoader.loads.size(), loadCountBeforeReplacement + 1);
    QCOMPARE(dataLoader.loads.back()->url, missingUrl);
    dataLoader.loads.back()->errorCallback(QStringLiteral("missing"));

    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->sourceUrl(), imageUrl);
    QCOMPARE(controller->displayedUrl(), imageUrl);
    QCOMPARE(controller->errorString(), QStringLiteral("missing"));
    QCOMPARE(controller->imageSize(), QSize(2, 1));
    QCOMPARE(controller->imageRevision(), displayedRevision);
    QVERIFY(!controller->image().isNull());
}

void TestImageDocumentController::decodedReplacementFailureSchedulesRecoveryPredecodeOnce()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl badUrl = localUrl(QStringLiteral("/images/02.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
            imageCandidate(badUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader, decodeBadDataAsFailure);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QTRY_COMPARE(dataLoader.loads.size(), std::size_t(2));
    QCOMPARE(dataLoader.loads.back()->url, badUrl);
    const std::size_t loadCountBeforeReplacement = dataLoader.loads.size();

    controller->setSourceUrl(badUrl);
    QCOMPARE(dataLoader.loads.size(), loadCountBeforeReplacement + 1);
    QCOMPARE(dataLoader.loads.back()->url, badUrl);
    dataLoader.loads.back()->dataCallback(QByteArrayLiteral("bad"));

    QTRY_COMPARE(controller->errorString(), QStringLiteral("decode failed"));
    QCOMPARE(controller->sourceUrl(), imageUrl);
    QCOMPARE(controller->displayedUrl(), imageUrl);
    QCOMPARE(dataLoader.loads.size(), loadCountBeforeReplacement + 2);
    QCOMPARE(dataLoader.loads.back()->url, badUrl);
}

void TestImageDocumentController::emptyContainerNavigationClearsImageAndSelectsContainer()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> currentArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(currentContainerUrl);
    const std::optional<KiriView::ArchiveDocumentLocation> targetArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(currentArchiveDocument.has_value());
    QVERIFY(targetArchiveDocument.has_value());
    const QUrl currentImageUrl
        = archivePageUrl(currentArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(currentArchiveDocument->rootUrl(),
        {
            imageCandidate(currentImageUrl),
        });
    candidateProvider.setContainerCandidates(localUrl(QStringLiteral("/books/")),
        {
            comicBookContainerCandidate(currentContainerUrl),
            comicBookContainerCandidate(targetContainerUrl),
        });
    candidateProvider.setArchiveImages(targetArchiveDocument->rootUrl(), {});

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(currentContainerUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->openNextContainer();

    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Error);
    QCOMPARE(controller->sourceUrl(), targetContainerUrl);
    QCOMPARE(controller->displayedUrl(), QUrl());
    QVERIFY(controller->containerNavigationAvailable());
    QVERIFY(controller->image().isNull());
    QCOMPARE(controller->imageSize(), QSize());
    QVERIFY(!controller->errorString().isEmpty());
}

QTEST_GUILESS_MAIN(TestImageDocumentController)

#include "test_imagedocumentcontroller.moc"
