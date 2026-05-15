// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "image_test_support.h"
#include "imagecontainer.h"
#include "imagerendering.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::comicBookContainerCandidate;
using KiriView::TestSupport::fileOperationProviderFor;
using KiriView::TestSupport::imageAsyncDependenciesFor;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualFileOperationProvider;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticImageDataDecoderRejectingBadData;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;
using KiriView::TestSupport::testImageDecodeFailureString;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

KiriView::DecodedImageResult staticDecodedImageWithPreview(const QSize &sourceSize,
    const QSize &previewSize, KiriView::StaticImageDisplayHints displayHints = {})
{
    return KiriView::successfulDecodedImageResult(KiriView::StaticDecodedImage {
        staticTestImagePayload(testImage(sourceSize), testImage(previewSize), displayHints),
    });
}

KiriView::DecodedImageResult singleFrameDecodedImage(const QSize &size)
{
    return KiriView::successfulDecodedImageResult(KiriView::DecodedAnimationImage {
        { KiriView::AnimationFrame { testImage(size), 0 } },
        1,
    });
}

std::unique_ptr<KiriView::ImageDocumentController> createController(QObject *parent,
    FakeCandidateProvider &candidateProvider, ManualImageDataLoader &dataLoader,
    KiriView::ImageDataDecoder dataDecoder = staticImageDataDecoder(testImage(2)),
    int maximumTextureSize = KiriView::fallbackTextureSizeMax, qreal devicePixelRatio = 1.0,
    KiriView::FileOperationProvider fileOperations = {},
    KiriView::ImageDocumentController::FileDeletionFailedCallback fileDeletionFailedCallback = {})
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
        imageAsyncDependenciesFor(
            candidateProvider, dataLoader, std::move(dataDecoder), std::move(fileOperations)),
        std::move(fileDeletionFailedCallback));
}

void finishLoad(ManualImageDataLoader &dataLoader)
{
    dataLoader.finishBackLoad(QByteArrayLiteral("ok"));
}

bool finishOldestActiveLoadForUrl(ManualImageDataLoader &dataLoader, const QUrl &url)
{
    return dataLoader.finishOldestActiveLoadForUrl(url, QByteArrayLiteral("ok"));
}

std::size_t staticTileCount(const KiriView::ImageDocumentController &controller,
    KiriView::DisplayedPageRole role = KiriView::DisplayedPageRole::Primary)
{
    std::shared_ptr<KiriView::DisplayedImageSurface> surface = controller.imageSurface(role);
    auto *staticSurface = surface == nullptr ? nullptr : surface->staticTileSurface();
    return staticSurface == nullptr ? 0 : staticSurface->tiles().size();
}

bool containsChange(
    const std::vector<KiriView::ImageDocumentChange> &changes, KiriView::ImageDocumentChange change)
{
    return std::find(changes.begin(), changes.end(), change) != changes.end();
}
}

class TestImageDocumentController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialLoadSuccessUpdatesDocumentState();
    void imageLoadsUsePhysicalViewportForFirstDisplayDecode();
    void maximumManualZoomChangesAfterViewportImageAndRenderContextUpdates();
    void directoryImageNavigationResetsManualZoom();
    void archiveDocumentPageNavigationPreservesManualZoom();
    void pendingAdjacentNavigationSkipsIntermediateLoad();
    void pendingPageSelectionSupersedesEarlierLoad();
    void pendingLoadFailureRestoresDisplayedPageNavigation();
    void siblingArchiveNavigationResetsManualZoom();
    void smallStaticImageUsesFullImageSurface();
    void largeStaticImageUsesTiledSurface();
    void tiledStaticImageRefinesNewVisibleTilesAfterPan();
    void firstDisplayDefersTilesUntilZoomNeedsMoreDetail();
    void smallFullImageSurfaceStillSchedulesAdjacentPredecode();
    void replacementLoadFailureKeepsDisplayedImage();
    void decodedReplacementFailureSchedulesRecoveryPredecodeOnce();
    void emptyContainerNavigationClearsImageAndSelectsContainer();
    void deletingWithoutDisplayedImageDoesNotStartFileOperation();
    void fileDeletionFailureKeepsDisplayedImageAndReportsError();
    void fileDeletionCancelKeepsDisplayedImageWithoutError();
    void successfulFileDeletionOpensNextImageFallback();
    void successfulFileDeletionOpensPreviousImageFallback();
    void successfulFileDeletionWithoutFallbackClearsDocument();
    void successfulComicBookDeletionOpensNextSiblingArchive();
    void rightToLeftReadingIsOnlyAvailableForComicArchives();
    void rightToLeftReadingPersistsAcrossSiblingArchiveNavigation();
    void twoPageModeDisplaysCurrentAndNextComicArchivePages();
    void twoPageModeUsesRightToLeftPageOrder();
    void twoPageModeRightToLeftKeepsSinglePageNavigationSemantic();
    void twoPageModeWaitsForTargetSpreadBeforeRenderingNavigation();
    void twoPageModeLoadingNavigationUsesPendingPrimaryPage();
    void twoPageModeKeepsCoverAndWidePagesSingle();
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
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
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

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader,
            staticImageDataDecoder(testImage(2)), KiriView::fallbackTextureSizeMax, 2.0);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(800, 600));
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, nextImageUrl);
    QCOMPARE(dataLoader.backLoad().firstDisplay.physicalViewportSize, QSize(800, 600));
}

void TestImageDocumentController::
    maximumManualZoomChangesAfterViewportImageAndRenderContextUpdates()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<KiriView::ImageDocumentChange> changes;
    qreal devicePixelRatio = 1.0;
    const QUrl firstImageUrl = localUrl(QStringLiteral("/images/regular.png"));
    const QUrl secondImageUrl = localUrl(QStringLiteral("/images/wide.png"));

    auto dataDecoder = [](const QByteArray &, const KiriView::ImageDecodeRequest &request) {
        const QSize sourceSize = request.imageUrl().fileName() == QStringLiteral("wide.png")
            ? QSize(2000, 1000)
            : QSize(1000, 500);
        return staticDecodedImageWithPreview(sourceSize, sourceSize);
    };
    std::unique_ptr<KiriView::ImageDocumentController> controller
        = std::make_unique<KiriView::ImageDocumentController>(
            this,
            [&devicePixelRatio]() {
                return KiriView::ImageDocumentRenderContext {
                    devicePixelRatio,
                    KiriView::fallbackTextureSizeMax,
                };
            },
            [&changes](KiriView::ImageDocumentChange change) { changes.push_back(change); },
            imageAsyncDependenciesFor(candidateProvider, dataLoader, std::move(dataDecoder)));

    controller->setViewportSize(QSizeF(7000.0, 100.0));
    controller->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        controller->maximumManualZoomPercent(), 65536.0 * 100.0 / 1000.0));

    changes.clear();
    controller->setViewportSize(QSizeF(9000.0, 100.0));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::MaximumManualZoomPercent));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        controller->maximumManualZoomPercent(), 9000.0 * 8.0 * 100.0 / 1000.0));

    changes.clear();
    controller->setSourceUrl(secondImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), secondImageUrl);
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::MaximumManualZoomPercent));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        controller->maximumManualZoomPercent(), 9000.0 * 8.0 * 100.0 / 2000.0));

    changes.clear();
    devicePixelRatio = 2.0;
    controller->updateRenderContext();
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::MaximumManualZoomPercent));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        controller->maximumManualZoomPercent(), 9000.0 * 8.0 * 2.0 * 100.0 / 2000.0));
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

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    controller->openNextImage();

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondImageUrl);
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

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    controller->openNextImage();

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), secondPageUrl);
    QCOMPARE(controller->zoomMode(), KiriView::ImageZoomMode::Manual);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(controller->zoomPercent(), 150.0));
}

void TestImageDocumentController::pendingAdjacentNavigationSkipsIntermediateLoad()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl firstImageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondImageUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdImageUrl = localUrl(QStringLiteral("/images/03.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(firstImageUrl),
            imageCandidate(secondImageUrl),
            imageCandidate(thirdImageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(controller->currentPageNumber(), 2);

    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, thirdImageUrl);
    QCOMPARE(controller->currentPageNumber(), 3);
    QVERIFY(!finishOldestActiveLoadForUrl(dataLoader, secondImageUrl));

    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), thirdImageUrl);
    QCOMPARE(controller->sourceUrl(), thirdImageUrl);
    QCOMPARE(controller->currentPageNumber(), 3);
}

void TestImageDocumentController::pendingPageSelectionSupersedesEarlierLoad()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl firstImageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondImageUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl thirdImageUrl = localUrl(QStringLiteral("/images/03.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(firstImageUrl),
            imageCandidate(secondImageUrl),
            imageCandidate(thirdImageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(controller->currentPageNumber(), 2);

    controller->openImageAtPage(3);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdImageUrl);
    QCOMPARE(controller->displayedUrl(), firstImageUrl);
    QCOMPARE(controller->currentPageNumber(), 3);
    QVERIFY(!finishOldestActiveLoadForUrl(dataLoader, secondImageUrl));

    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), thirdImageUrl);
    QCOMPARE(controller->sourceUrl(), thirdImageUrl);
    QCOMPARE(controller->currentPageNumber(), 3);
}

void TestImageDocumentController::pendingLoadFailureRestoresDisplayedPageNavigation()
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

    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(controller->sourceUrl(), secondImageUrl);
    QCOMPARE(controller->displayedUrl(), firstImageUrl);
    QCOMPARE(controller->currentPageNumber(), 2);

    dataLoader.failBackLoad(QStringLiteral("missing"));

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->sourceUrl(), firstImageUrl);
    QCOMPARE(controller->displayedUrl(), firstImageUrl);
    QCOMPARE(controller->currentPageNumber(), 1);
    QCOMPARE(controller->errorString(), QStringLiteral("missing"));
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

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    controller->openNextContainer();

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondPageUrl);
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
    QVERIFY(surface->legacyFrameSurface() != nullptr);
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
    auto *staticSurface = surface->staticTileSurface();
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
    QVERIFY(surface->legacyFrameSurface() != nullptr);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, nextImageUrl);
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
    const std::size_t loadCountBeforeReplacement = dataLoader.loadCount();

    controller->setSourceUrl(missingUrl);
    QCOMPARE(dataLoader.loadCount(), loadCountBeforeReplacement + 1);
    QCOMPARE(dataLoader.backLoad().url, missingUrl);
    dataLoader.failBackLoad(QStringLiteral("missing"));

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

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(
        this, candidateProvider, dataLoader, staticImageDataDecoderRejectingBadData(testImage(2)));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, badUrl);
    const std::size_t loadCountBeforeReplacement = dataLoader.loadCount();

    controller->setSourceUrl(badUrl);
    QCOMPARE(dataLoader.loadCount(), loadCountBeforeReplacement + 1);
    QCOMPARE(dataLoader.backLoad().url, badUrl);
    dataLoader.finishBackLoad(QByteArrayLiteral("bad"));

    QTRY_COMPARE(controller->errorString(), testImageDecodeFailureString());
    QCOMPARE(controller->sourceUrl(), imageUrl);
    QCOMPARE(controller->displayedUrl(), imageUrl);
    QCOMPARE(dataLoader.loadCount(), loadCountBeforeReplacement + 2);
    QCOMPARE(dataLoader.backLoad().url, badUrl);
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

void TestImageDocumentController::deletingWithoutDisplayedImageDoesNotStartFileOperation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileOperationProvider fileOperations;

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(this,
        candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
        KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));

    controller->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(0));
    QVERIFY(!controller->fileDeletionInProgress());
}

void TestImageDocumentController::fileDeletionFailureKeepsDisplayedImageAndReportsError()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileOperationProvider fileOperations;
    std::vector<QString> deletionErrors;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(this,
        candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
        KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations),
        [&deletionErrors](const QString &errorString) { deletionErrors.push_back(errorString); });
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, imageUrl);
    QCOMPARE(fileOperations.backOperation().request.mode, KiriView::FileDeletionMode::MoveToTrash);
    QVERIFY(controller->fileDeletionInProgress());

    fileOperations.finishBackOperation(
        KiriView::FileDeletionResult::Failed, QStringLiteral("permission denied"));

    QVERIFY(!controller->fileDeletionInProgress());
    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->sourceUrl(), imageUrl);
    QCOMPARE(controller->displayedUrl(), imageUrl);
    QCOMPARE(deletionErrors.size(), std::size_t(1));
    QCOMPARE(deletionErrors.front(), QStringLiteral("permission denied"));
    QVERIFY(!controller->image().isNull());
}

void TestImageDocumentController::fileDeletionCancelKeepsDisplayedImageWithoutError()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileOperationProvider fileOperations;
    std::vector<QString> deletionErrors;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(this,
        candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
        KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations),
        [&deletionErrors](const QString &errorString) { deletionErrors.push_back(errorString); });
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->deleteDisplayedFile(KiriView::FileDeletionMode::DeletePermanently);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, imageUrl);
    QCOMPARE(
        fileOperations.backOperation().request.mode, KiriView::FileDeletionMode::DeletePermanently);

    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Canceled);

    QVERIFY(!controller->fileDeletionInProgress());
    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->sourceUrl(), imageUrl);
    QCOMPARE(controller->displayedUrl(), imageUrl);
    QVERIFY(deletionErrors.empty());
    QVERIFY(!controller->image().isNull());
}

void TestImageDocumentController::successfulFileDeletionOpensNextImageFallback()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileOperationProvider fileOperations;
    const QUrl previousUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/03.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(previousUrl),
            imageCandidate(nextUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(this,
        candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
        KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(currentUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(controller->sourceUrl(), nextUrl);
    QCOMPARE(controller->displayedUrl(), QUrl());
    QCOMPARE(dataLoader.backLoad().url, nextUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->displayedUrl(), nextUrl);
}

void TestImageDocumentController::successfulFileDeletionOpensPreviousImageFallback()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileOperationProvider fileOperations;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl previousUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl currentUrl = localUrl(QStringLiteral("/images/03.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(firstUrl),
            imageCandidate(previousUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(this,
        candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
        KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(currentUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(controller->sourceUrl(), previousUrl);
    QCOMPARE(dataLoader.backLoad().url, previousUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->displayedUrl(), previousUrl);
}

void TestImageDocumentController::successfulFileDeletionWithoutFallbackClearsDocument()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileOperationProvider fileOperations;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")), {});

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(this,
        candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
        KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Null);
    QCOMPARE(controller->sourceUrl(), QUrl());
    QCOMPARE(controller->displayedUrl(), QUrl());
    QCOMPARE(controller->imageSize(), QSize());
    QVERIFY(controller->image().isNull());
}

void TestImageDocumentController::successfulComicBookDeletionOpensNextSiblingArchive()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileOperationProvider fileOperations;
    const QUrl currentArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl nextArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> currentArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(currentArchiveUrl);
    const std::optional<KiriView::ArchiveDocumentLocation> nextArchiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(nextArchiveUrl);
    QVERIFY(currentArchiveDocument.has_value());
    QVERIFY(nextArchiveDocument.has_value());
    const QUrl currentImageUrl
        = archivePageUrl(currentArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextImageUrl
        = archivePageUrl(nextArchiveDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(currentArchiveDocument->rootUrl(),
        {
            imageCandidate(currentImageUrl),
        });
    candidateProvider.setContainerCandidates(localUrl(QStringLiteral("/books/")),
        {
            comicBookContainerCandidate(nextArchiveUrl),
        });
    candidateProvider.setArchiveImages(nextArchiveDocument->rootUrl(),
        {
            imageCandidate(nextImageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller = createController(this,
        candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
        KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(currentArchiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);

    controller->deleteDisplayedFile(KiriView::FileDeletionMode::DeletePermanently);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, currentArchiveUrl);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(controller->sourceUrl(), nextImageUrl);
    QCOMPARE(dataLoader.backLoad().url, nextImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->displayedUrl(), nextImageUrl);
    QVERIFY(controller->containerNavigationAvailable());
}

void TestImageDocumentController::rightToLeftReadingIsOnlyAvailableForComicArchives()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl archivePageUrl = KiriView::TestSupport::archivePageUrl(
        archiveDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(archivePageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(!controller->rightToLeftReadingEnabled());
    QVERIFY(!controller->rightToLeftReadingAvailable());

    controller->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), archivePageUrl);
    QVERIFY(controller->rightToLeftReadingAvailable());
    QVERIFY(!controller->rightToLeftReadingEnabled());
}

void TestImageDocumentController::rightToLeftReadingPersistsAcrossSiblingArchiveNavigation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl firstArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl secondArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const QUrl ordinaryImageUrl = localUrl(QStringLiteral("/images/01.png"));
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
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(ordinaryImageUrl),
        });

    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader);
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(firstArchiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), firstPageUrl);
    controller->setRightToLeftReadingEnabled(true);
    QVERIFY(controller->rightToLeftReadingEnabled());

    controller->openNextContainer();
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), secondPageUrl);
    QVERIFY(controller->rightToLeftReadingAvailable());
    QVERIFY(controller->rightToLeftReadingEnabled());

    controller->setSourceUrl(ordinaryImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), ordinaryImageUrl);
    QVERIFY(!controller->rightToLeftReadingAvailable());
    QVERIFY(!controller->rightToLeftReadingEnabled());
}

void TestImageDocumentController::twoPageModeDisplaysCurrentAndNextComicArchivePages()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(firstPageUrl),
            imageCandidate(secondPageUrl),
            imageCandidate(thirdPageUrl),
        });

    auto decoder = [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
        return singleFrameDecodedImage(QSize(100, 200));
    };
    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader, std::move(decoder));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(controller->displayedUrl(), firstPageUrl);
    QVERIFY(controller->twoPageModeAvailable());

    controller->setTwoPageModeEnabled(true);
    QVERIFY(controller->twoPageModeEnabled());
    QVERIFY(!controller->secondaryPageVisible());
    QCOMPARE(controller->imageSize(), QSize(100, 200));

    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->displayedUrl(), secondPageUrl);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);

    QTRY_VERIFY(controller->secondaryPageVisible());
    QCOMPARE(controller->currentPageNumber(), 2);
    QCOMPARE(controller->currentLastPageNumber(), 3);
    QCOMPARE(controller->primaryImageSize(), QSize(100, 200));
    QCOMPARE(controller->secondaryImageSize(), QSize(100, 200));
    QCOMPARE(controller->imageSize(), QSize(200, 200));
}

void TestImageDocumentController::twoPageModeUsesRightToLeftPageOrder()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(firstPageUrl),
            imageCandidate(secondPageUrl),
            imageCandidate(thirdPageUrl),
        });

    auto decoder = [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
        return staticDecodedImageWithPreview(QSize(100, 200), QSize(100, 200));
    };
    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader, std::move(decoder), 64);
    controller->setViewportSize(QSizeF(100.0, 200.0));
    controller->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), firstPageUrl);
    controller->setTwoPageModeEnabled(true);
    controller->setRightToLeftReadingEnabled(true);
    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, thirdPageUrl));

    QTRY_VERIFY(controller->secondaryPageVisible());
    QCOMPARE(controller->currentPageNumber(), 2);
    QCOMPARE(controller->currentLastPageNumber(), 3);
    controller->setZoomPercent(100.0);
    controller->setVisibleItemRect(QRectF(0.0, 0.0, 100.0, 200.0));

    QTRY_VERIFY(staticTileCount(*controller, KiriView::DisplayedPageRole::Secondary) > 0);
    QCOMPARE(staticTileCount(*controller, KiriView::DisplayedPageRole::Primary), std::size_t(0));
}

void TestImageDocumentController::twoPageModeRightToLeftKeepsSinglePageNavigationSemantic()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    const QUrl fourthPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("04.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(firstPageUrl),
            imageCandidate(secondPageUrl),
            imageCandidate(thirdPageUrl),
            imageCandidate(fourthPageUrl),
        });

    auto decoder = [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
        return singleFrameDecodedImage(QSize(100, 200));
    };
    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader, std::move(decoder));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), firstPageUrl);
    controller->setTwoPageModeEnabled(true);
    controller->setRightToLeftReadingEnabled(true);
    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, thirdPageUrl));
    QTRY_COMPARE(controller->currentPageNumber(), 2);
    QTRY_COMPARE(controller->currentLastPageNumber(), 3);

    std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    controller->openNextSinglePage();
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->displayedUrl(), thirdPageUrl);
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(2));
    QTRY_COMPARE(dataLoader.backLoad().url, fourthPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, fourthPageUrl));
    QTRY_COMPARE(controller->currentPageNumber(), 3);
    QTRY_COMPARE(controller->currentLastPageNumber(), 4);

    loadCountBeforeNavigation = dataLoader.loadCount();
    controller->openPreviousSinglePage();
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->displayedUrl(), secondPageUrl);
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(2));
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, thirdPageUrl));
    QTRY_COMPARE(controller->currentPageNumber(), 2);
    QTRY_COMPARE(controller->currentLastPageNumber(), 3);
}

void TestImageDocumentController::twoPageModeWaitsForTargetSpreadBeforeRenderingNavigation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    const QUrl fourthPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("04.png"));
    const QUrl fifthPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("05.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(firstPageUrl),
            imageCandidate(secondPageUrl),
            imageCandidate(thirdPageUrl),
            imageCandidate(fourthPageUrl),
            imageCandidate(fifthPageUrl),
        });

    auto decoder = [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
        return singleFrameDecodedImage(QSize(100, 200));
    };
    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader, std::move(decoder));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->displayedUrl(), firstPageUrl);

    controller->setTwoPageModeEnabled(true);
    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);

    QTRY_VERIFY(controller->secondaryPageVisible());
    QCOMPARE(controller->displayedUrl(), secondPageUrl);
    QCOMPARE(controller->currentLastPageNumber(), 3);
    QVERIFY(controller->imageSurface() != nullptr);
    QVERIFY(controller->imageSurface(KiriView::DisplayedPageRole::Secondary) != nullptr);

    controller->openNextImage();

    QTRY_COMPARE(dataLoader.backLoad().url, fourthPageUrl);
    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Loading);
    QVERIFY(controller->loading());
    QVERIFY(controller->imageSurface() == nullptr);
    QVERIFY(controller->imageSurface(KiriView::DisplayedPageRole::Secondary) == nullptr);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), fourthPageUrl);
    QTRY_COMPARE(dataLoader.backLoad().url, fifthPageUrl);
    QCOMPARE(controller->status(), KiriView::ImageDocumentStatus::Loading);
    QVERIFY(controller->loading());
    QVERIFY(controller->imageSurface() == nullptr);
    QVERIFY(controller->imageSurface(KiriView::DisplayedPageRole::Secondary) == nullptr);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    QTRY_VERIFY(controller->secondaryPageVisible());
    QCOMPARE(controller->displayedUrl(), fourthPageUrl);
    QCOMPARE(controller->currentLastPageNumber(), 5);
    QVERIFY(controller->imageSurface() != nullptr);
    QVERIFY(controller->imageSurface(KiriView::DisplayedPageRole::Secondary) != nullptr);
}

void TestImageDocumentController::twoPageModeLoadingNavigationUsesPendingPrimaryPage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl firstPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    const QUrl fourthPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("04.png"));
    const QUrl fifthPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("05.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(firstPageUrl),
            imageCandidate(secondPageUrl),
            imageCandidate(thirdPageUrl),
            imageCandidate(fourthPageUrl),
            imageCandidate(fifthPageUrl),
        });

    auto decoder = [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
        return singleFrameDecodedImage(QSize(100, 200));
    };
    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader, std::move(decoder));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->displayedUrl(), firstPageUrl);

    controller->setTwoPageModeEnabled(true);
    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(controller->currentPageNumber(), 2);
    QTRY_COMPARE(controller->currentLastPageNumber(), 3);
    QVERIFY(controller->secondaryPageVisible());

    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, fourthPageUrl);
    QCOMPARE(controller->currentPageNumber(), 4);

    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, fifthPageUrl);
    QCOMPARE(controller->currentPageNumber(), 5);
    QVERIFY(!finishOldestActiveLoadForUrl(dataLoader, fourthPageUrl));

    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), fifthPageUrl);
    QCOMPARE(controller->currentPageNumber(), 5);
    QVERIFY(!controller->secondaryPageVisible());
}

void TestImageDocumentController::twoPageModeKeepsCoverAndWidePagesSingle()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl coverUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    const QUrl widePageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("02.png"));
    const QUrl portraitPageUrl
        = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("03.png"));
    const QUrl fourthPageUrl = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("04.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(),
        {
            imageCandidate(coverUrl),
            imageCandidate(widePageUrl),
            imageCandidate(portraitPageUrl),
            imageCandidate(fourthPageUrl),
        });

    auto decoder = [widePageUrl](const QByteArray &, const KiriView::ImageDecodeRequest &request) {
        return singleFrameDecodedImage(
            request.imageUrl() == widePageUrl ? QSize(300, 100) : QSize(100, 200));
    };
    std::unique_ptr<KiriView::ImageDocumentController> controller
        = createController(this, candidateProvider, dataLoader, std::move(decoder));
    controller->setViewportSize(QSizeF(400.0, 300.0));
    controller->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->status(), KiriView::ImageDocumentStatus::Ready);
    controller->setTwoPageModeEnabled(true);
    QVERIFY(!controller->secondaryPageVisible());
    QCOMPARE(controller->displayedUrl(), coverUrl);

    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, widePageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), widePageUrl);
    QVERIFY(!controller->secondaryPageVisible());
    QCOMPARE(controller->imageSize(), QSize(300, 100));

    controller->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, portraitPageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), portraitPageUrl);
    QTRY_COMPARE(dataLoader.backLoad().url, fourthPageUrl);
    finishLoad(dataLoader);

    QTRY_VERIFY(controller->secondaryPageVisible());
    QCOMPARE(controller->imageSize(), QSize(200, 200));

    controller->openPreviousImage();
    QTRY_COMPARE(dataLoader.backLoad().url, widePageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(controller->displayedUrl(), widePageUrl);
}

QTEST_GUILESS_MAIN(TestImageDocumentController)

#include "test_imagedocumentcontroller.moc"
