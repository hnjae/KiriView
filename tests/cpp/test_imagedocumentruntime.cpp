// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntime.h"

#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "rendering/imagerendering.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::TestSupport::archivePageUrl;
using KiriView::TestSupport::comicBookContainerCandidate;
using KiriView::TestSupport::fileOperationProviderFor;
using KiriView::TestSupport::imageCandidate;
using KiriView::TestSupport::imageDocumentRuntimeDependencyOverridesFor;
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::ManualFileOperationProvider;
using KiriView::TestSupport::ManualImageDataLoader;
using KiriView::TestSupport::staticImageDataDecoder;
using KiriView::TestSupport::staticImageDataDecoderRejectingBadData;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;
using KiriView::TestSupport::testImageDecodeFailureString;

using FakeCandidateProvider = KiriView::TestSupport::FakeImageNavigationCandidateProvider;

class RuntimeOwner final : public QObject
{
public:
    RuntimeOwner(QObject *parent,
        KiriView::ImageDocumentRuntime::RenderContextProvider renderContextProvider,
        KiriView::ImageDocumentRuntime::ChangeCallback changeCallback,
        KiriView::ImageDocumentRuntimeDependencyOverrides dependencies,
        KiriView::ImageDocumentRuntime::FileDeletionFailedCallback fileDeletionFailedCallback = {})
        : QObject(parent)
        , runtime(this, std::move(renderContextProvider), std::move(changeCallback),
              std::move(dependencies), std::move(fileDeletionFailedCallback))
    {
    }

    KiriView::ImageDocumentRuntime runtime;
};

class RuntimeHandle final
{
public:
    RuntimeHandle(QObject *parent,
        KiriView::ImageDocumentRuntime::RenderContextProvider renderContextProvider,
        KiriView::ImageDocumentRuntime::ChangeCallback changeCallback,
        KiriView::ImageDocumentRuntimeDependencyOverrides dependencies,
        KiriView::ImageDocumentRuntime::FileDeletionFailedCallback fileDeletionFailedCallback = {})
        : m_owner(std::make_unique<RuntimeOwner>(parent, std::move(renderContextProvider),
              std::move(changeCallback), std::move(dependencies),
              std::move(fileDeletionFailedCallback)))
    {
    }

    KiriView::ImageDocumentRuntime *operator->() { return &m_owner->runtime; }
    const KiriView::ImageDocumentRuntime *operator->() const { return &m_owner->runtime; }
    KiriView::ImageDocumentRuntime &operator*() { return m_owner->runtime; }
    const KiriView::ImageDocumentRuntime &operator*() const { return m_owner->runtime; }

private:
    std::unique_ptr<RuntimeOwner> m_owner;
};

class NonCacheableTestImageTileSource final : public KiriView::ImageTileSource
{
public:
    explicit NonCacheableTestImageTileSource(QImage image)
        : m_image(std::move(image))
    {
    }

    QSize imageSize() const override { return m_image.size(); }

    std::optional<KiriView::DecodedTile> decodeTile(
        const KiriView::TileRequest &, QString *) const override
    {
        return std::nullopt;
    }

    QImage decodeBlockingDisplayImage(int, QString *) const override { return m_image; }

    qsizetype byteCost() const override { return std::numeric_limits<qsizetype>::max() / 2; }

private:
    QImage m_image;
};

KiriView::DecodedImageResult staticDecodedImageWithPreview(const QSize &sourceSize,
    const QSize &previewSize, KiriView::StaticImageDisplayHints displayHints = {})
{
    return KiriView::successfulDecodedImageResult(KiriView::StaticDecodedImage {
        staticTestImagePayload(testImage(sourceSize), testImage(previewSize), displayHints),
    });
}

KiriView::DecodedImageResult singleFrameDecodedImage(const QSize &size)
{
    QImage image = testImage(size);
    return KiriView::successfulDecodedImageResult(KiriView::StaticDecodedImage {
        KiriView::StaticImagePayload {
            std::make_shared<NonCacheableTestImageTileSource>(image),
            std::move(image),
            {},
        },
    });
}

RuntimeHandle createRuntime(QObject *parent, FakeCandidateProvider &candidateProvider,
    ManualImageDataLoader &dataLoader,
    KiriView::ImageDataDecoder dataDecoder = staticImageDataDecoder(testImage(2)),
    int maximumTextureSize = KiriView::fallbackTextureSizeMax, qreal devicePixelRatio = 1.0,
    KiriView::FileOperationProvider fileOperations = {},
    KiriView::ImageDocumentRuntime::FileDeletionFailedCallback fileDeletionFailedCallback = {})
{
    return RuntimeHandle(
        parent,
        [maximumTextureSize, devicePixelRatio]() {
            return KiriView::ImageDocumentRenderContext {
                devicePixelRatio,
                maximumTextureSize,
            };
        },
        KiriView::ImageDocumentRuntime::ChangeCallback {},
        imageDocumentRuntimeDependencyOverridesFor(
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

std::size_t staticTileCount(const KiriView::ImageDocumentRuntime &runtime,
    KiriView::DisplayedPageRole role = KiriView::DisplayedPageRole::Primary)
{
    std::shared_ptr<KiriView::DisplayedImageSurface> surface = runtime.renderSnapshot(role).surface;
    auto *staticSurface = surface == nullptr ? nullptr : surface->staticTileSurface();
    return staticSurface == nullptr ? 0 : staticSurface->tiles().size();
}

bool hasRenderableSnapshot(const KiriView::ImageDocumentRuntime &runtime,
    KiriView::DisplayedPageRole role = KiriView::DisplayedPageRole::Primary)
{
    return runtime.renderSnapshot(role).isRenderable();
}

bool containsChange(
    const std::vector<KiriView::ImageDocumentChange> &changes, KiriView::ImageDocumentChange change)
{
    return std::find(changes.begin(), changes.end(), change) != changes.end();
}
}

class TestImageDocumentRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialLoadSuccessUpdatesDocumentState();
    void ordinaryDirectMediaScopeProjectionFollowsDisplayedImageScope();
    void imageLoadsUsePhysicalViewportForFirstDisplayDecode();
    void renderContextProviderCanBeReplacedAfterConstruction();
    void maximumManualZoomChangesAfterViewportImageAndRenderContextUpdates();
    void directoryImageNavigationResetsManualZoom();
    void archiveDocumentPageNavigationPreservesManualZoom();
    void rotationChangesLogicalSizeAndPreservesManualZoom();
    void rotationRecomputesFitZoomForRotatedBounds();
    void rotationResetsOnImageNavigation();
    void rotationResetsAndStopsWhileTwoPageModeIsEnabled();
    void pendingAdjacentNavigationSkipsIntermediateLoad();
    void pendingPageSelectionSupersedesEarlierLoad();
    void pageSelectionStartsTrackedLoadThroughEffectExecutor();
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
};

void TestImageDocumentRuntime::initialLoadSuccessUpdatesDocumentState()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    QVERIFY(!runtime->zoomPercentKnown());
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);

    QCOMPARE(runtime->status(), KiriView::ImageDocumentStatus::Loading);
    QVERIFY(!runtime->zoomPercentKnown());
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(runtime->zoomPercentKnown());
    QCOMPARE(runtime->sourceUrl(), imageUrl);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    QCOMPARE(runtime->imageSize(), QSize(2, 1));
    QCOMPARE(runtime->currentPageNumber(), 1);
    QCOMPARE(runtime->imageCount(), 1);
    QVERIFY(!runtime->containerNavigationAvailable());
    QVERIFY(runtime->renderSnapshot().isRenderable());
}

void TestImageDocumentRuntime::ordinaryDirectMediaScopeProjectionFollowsDisplayedImageScope()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    QVERIFY(!runtime->ordinaryDirectMediaScopeActive());
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(runtime->ordinaryDirectMediaScopeActive());

    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<KiriView::ArchiveDocumentLocation> archiveDocument
        = KiriView::archiveDocumentLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveDocument.has_value());
    const QUrl archivePage = archivePageUrl(archiveDocument->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setArchiveImages(archiveDocument->rootUrl(), { imageCandidate(archivePage) });

    RuntimeHandle archiveRuntime = createRuntime(this, candidateProvider, dataLoader);
    archiveRuntime->setViewportSize(QSizeF(400.0, 300.0));
    archiveRuntime->setSourceUrl(archiveUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, archivePage));

    QTRY_COMPARE(archiveRuntime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(archiveRuntime->displayedUrl(), archivePage);
    QVERIFY(!archiveRuntime->ordinaryDirectMediaScopeActive());
}

void TestImageDocumentRuntime::imageLoadsUsePhysicalViewportForFirstDisplayDecode()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        staticImageDataDecoder(testImage(2)), KiriView::fallbackTextureSizeMax, 2.0);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(800, 600));
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, nextImageUrl);
    QCOMPARE(dataLoader.backLoad().firstDisplay.physicalViewportSize, QSize(800, 600));
}

void TestImageDocumentRuntime::renderContextProviderCanBeReplacedAfterConstruction()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<KiriView::ImageDocumentChange> changes;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/regular.png"));

    RuntimeHandle runtime(
        this,
        []() {
            return KiriView::ImageDocumentRenderContext {
                1.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        [&changes](const std::vector<KiriView::ImageDocumentChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.begin(), publishedChanges.end());
        },
        imageDocumentRuntimeDependencyOverridesFor(candidateProvider, dataLoader,
            [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
                return staticDecodedImageWithPreview(QSize(1000, 500), QSize(1000, 500));
            }));

    runtime->setViewportSize(QSizeF(7000.0, 100.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 65536.0 * 100.0 / 1000.0));

    changes.clear();
    runtime->setRenderContextProvider([]() {
        return KiriView::ImageDocumentRenderContext {
            2.0,
            KiriView::fallbackTextureSizeMax,
        };
    });

    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::MaximumManualZoomPercent));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 65536.0 * 2.0 * 100.0 / 1000.0));
}

void TestImageDocumentRuntime::maximumManualZoomChangesAfterViewportImageAndRenderContextUpdates()
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
    RuntimeHandle runtime(
        this,
        [&devicePixelRatio]() {
            return KiriView::ImageDocumentRenderContext {
                devicePixelRatio,
                KiriView::fallbackTextureSizeMax,
            };
        },
        [&changes](const std::vector<KiriView::ImageDocumentChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.begin(), publishedChanges.end());
        },
        imageDocumentRuntimeDependencyOverridesFor(
            candidateProvider, dataLoader, std::move(dataDecoder)));

    runtime->setViewportSize(QSizeF(7000.0, 100.0));
    runtime->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 65536.0 * 100.0 / 1000.0));

    changes.clear();
    runtime->setViewportSize(QSizeF(9000.0, 100.0));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::MaximumManualZoomPercent));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 9000.0 * 8.0 * 100.0 / 1000.0));

    changes.clear();
    devicePixelRatio = 2.0;
    runtime->updateRenderContext();
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::MaximumManualZoomPercent));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 9000.0 * 8.0 * 2.0 * 100.0 / 1000.0));
}

void TestImageDocumentRuntime::directoryImageNavigationResetsManualZoom()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    runtime->setZoomPercent(150.0);
    QCOMPARE(runtime->zoomMode(), KiriView::ImageZoomMode::Manual);

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openNextImage();

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondImageUrl);
    QCOMPARE(runtime->zoomMode(), KiriView::ImageZoomMode::Fit);
}

void TestImageDocumentRuntime::archiveDocumentPageNavigationPreservesManualZoom()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), firstPageUrl);
    runtime->setZoomPercent(150.0);
    QCOMPARE(runtime->zoomMode(), KiriView::ImageZoomMode::Manual);

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openNextImage();

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondPageUrl);
    QCOMPARE(runtime->zoomMode(), KiriView::ImageZoomMode::Manual);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(runtime->zoomPercent(), 150.0));
}

void TestImageDocumentRuntime::rotationChangesLogicalSizeAndPreservesManualZoom()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<KiriView::ImageDocumentChange> changes;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/portrait.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    RuntimeHandle runtime(
        this,
        []() {
            return KiriView::ImageDocumentRenderContext {
                1.0,
                KiriView::fallbackTextureSizeMax,
            };
        },
        [&changes](const std::vector<KiriView::ImageDocumentChange> &publishedChanges) {
            changes.insert(changes.end(), publishedChanges.begin(), publishedChanges.end());
        },
        imageDocumentRuntimeDependencyOverridesFor(candidateProvider, dataLoader,
            [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
                return staticDecodedImageWithPreview(QSize(100, 200), QSize(100, 200));
            }));
    runtime->setViewportSize(QSizeF(500.0, 500.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    runtime->setZoomPercent(100.0);
    changes.clear();

    runtime->rotateClockwise();

    QCOMPARE(runtime->rotationDegrees(), 90);
    QCOMPARE(runtime->imageSize(), QSize(200, 100));
    QCOMPARE(runtime->primaryImageSize(), QSize(200, 100));
    QCOMPARE(runtime->displaySize(), QSizeF(200.0, 100.0));
    QCOMPARE(runtime->zoomMode(), KiriView::ImageZoomMode::Manual);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(runtime->zoomPercent(), 100.0));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Rotation));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::ImageSize));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::DisplaySize));
    QVERIFY(containsChange(changes, KiriView::ImageDocumentChange::Repaint));

    runtime->rotateCounterclockwise();

    QCOMPARE(runtime->rotationDegrees(), 0);
    QCOMPARE(runtime->imageSize(), QSize(100, 200));
    QCOMPARE(runtime->displaySize(), QSizeF(100.0, 200.0));
}

void TestImageDocumentRuntime::rotationRecomputesFitZoomForRotatedBounds()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/portrait.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(QSize(100, 200), QSize(100, 200));
        });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->zoomMode(), KiriView::ImageZoomMode::Fit);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(runtime->zoomPercent(), 150.0));
    QCOMPARE(runtime->displaySize(), QSizeF(150.0, 300.0));

    runtime->rotateClockwise();

    QCOMPARE(runtime->rotationDegrees(), 90);
    QCOMPARE(runtime->zoomMode(), KiriView::ImageZoomMode::Fit);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(runtime->zoomPercent(), 200.0));
    QCOMPARE(runtime->displaySize(), QSizeF(400.0, 200.0));
}

void TestImageDocumentRuntime::rotationResetsOnImageNavigation()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(QSize(100, 200), QSize(100, 200));
        });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    runtime->rotateClockwise();
    QCOMPARE(runtime->rotationDegrees(), 90);
    QCOMPARE(runtime->imageSize(), QSize(200, 100));

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondImageUrl);
    QCOMPARE(runtime->rotationDegrees(), 0);
    QCOMPARE(runtime->imageSize(), QSize(100, 200));
}

void TestImageDocumentRuntime::rotationResetsAndStopsWhileTwoPageModeIsEnabled()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(QSize(100, 200), QSize(100, 200));
        });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    runtime->rotateClockwise();
    QCOMPARE(runtime->rotationDegrees(), 90);

    runtime->setTwoPageModeEnabled(true);

    QCOMPARE(runtime->rotationDegrees(), 0);
    QVERIFY(runtime->twoPageModeEnabled());
    QVERIFY(runtime->twoPageModeAvailable());
    runtime->rotateClockwise();
    QCOMPARE(runtime->rotationDegrees(), 0);
}

void TestImageDocumentRuntime::pendingAdjacentNavigationSkipsIntermediateLoad()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 2);

    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, thirdImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 3);
    QVERIFY(!finishOldestActiveLoadForUrl(dataLoader, secondImageUrl));

    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), thirdImageUrl);
    QCOMPARE(runtime->sourceUrl(), thirdImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 3);
}

void TestImageDocumentRuntime::pendingPageSelectionSupersedesEarlierLoad()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 2);

    runtime->openImageAtPage(3);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdImageUrl);
    QCOMPARE(runtime->displayedUrl(), firstImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 3);
    QVERIFY(!finishOldestActiveLoadForUrl(dataLoader, secondImageUrl));

    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), thirdImageUrl);
    QCOMPARE(runtime->sourceUrl(), thirdImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 3);
}

void TestImageDocumentRuntime::pageSelectionStartsTrackedLoadThroughEffectExecutor()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    const std::size_t loadCountBeforeSelection = dataLoader.loadCount();
    runtime->openImageAtPage(2);

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeSelection + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(runtime->sourceUrl(), secondImageUrl);
    QCOMPARE(runtime->displayedUrl(), firstImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 2);

    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondImageUrl);
    QCOMPARE(runtime->sourceUrl(), secondImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 2);
}

void TestImageDocumentRuntime::pendingLoadFailureRestoresDisplayedPageNavigation()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(runtime->sourceUrl(), secondImageUrl);
    QCOMPARE(runtime->displayedUrl(), firstImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 2);

    dataLoader.failBackLoad(QStringLiteral("missing"));

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->sourceUrl(), firstImageUrl);
    QCOMPARE(runtime->displayedUrl(), firstImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 1);
    QCOMPARE(runtime->errorString(), QStringLiteral("missing"));
}

void TestImageDocumentRuntime::siblingArchiveNavigationResetsManualZoom()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(firstArchiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), firstPageUrl);
    runtime->setZoomPercent(150.0);
    QCOMPARE(runtime->zoomMode(), KiriView::ImageZoomMode::Manual);

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openNextContainer();

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondPageUrl);
    QCOMPARE(runtime->zoomMode(), KiriView::ImageZoomMode::Fit);
}

void TestImageDocumentRuntime::smallStaticImageUsesFullImageSurface()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/small.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(QSize(1024, 1), QSize(1024, 1));
        },
        KiriView::fallbackTextureSizeMax);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    std::shared_ptr<KiriView::DisplayedImageSurface> surface = runtime->renderSnapshot().surface;
    QVERIFY(surface != nullptr);
    QVERIFY(surface->legacyFrameSurface() != nullptr);
    QCOMPARE(runtime->imageSize(), QSize(1024, 1));
}

void TestImageDocumentRuntime::largeStaticImageUsesTiledSurface()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/large.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(
                QSize(KiriView::imageBlockingDisplayLongEdgeMax + 1, 1),
                QSize(KiriView::imageBlockingDisplayLongEdgeMax, 1));
        },
        KiriView::fallbackTextureSizeMax);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    std::shared_ptr<KiriView::DisplayedImageSurface> surface = runtime->renderSnapshot().surface;
    QVERIFY(surface != nullptr);
    auto *staticSurface = surface->staticTileSurface();
    QVERIFY(staticSurface != nullptr);
    QCOMPARE(staticSurface->imageSize(), QSize(KiriView::imageBlockingDisplayLongEdgeMax + 1, 1));
    QCOMPARE(staticSurface->preview().size(), QSize(KiriView::imageBlockingDisplayLongEdgeMax, 1));
}

void TestImageDocumentRuntime::tiledStaticImageRefinesNewVisibleTilesAfterPan()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/large.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(
                QSize(KiriView::imageBlockingDisplayLongEdgeMax + 1, 512),
                QSize(KiriView::imageBlockingDisplayLongEdgeMax, 512));
        },
        KiriView::fallbackTextureSizeMax);
    runtime->setViewportSize(QSizeF(512.0, 512.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    runtime->setZoomPercent(100.0);
    runtime->setVisibleItemRect(QRectF(0.0, 0.0, 512.0, 512.0));
    QTRY_VERIFY(staticTileCount(*runtime) > std::size_t(0));
    const std::size_t initialTileCount = staticTileCount(*runtime);

    runtime->setVisibleItemRect(QRectF(1536.0, 0.0, 512.0, 512.0));

    QTRY_VERIFY(staticTileCount(*runtime) > initialTileCount);
}

void TestImageDocumentRuntime::firstDisplayDefersTilesUntilZoomNeedsMoreDetail()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/large.jpg"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(
                QSize(2048, 2048), QSize(512, 512), KiriView::StaticImageDisplayHints { 0.25 });
        },
        KiriView::fallbackTextureSizeMax);
    runtime->setViewportSize(QSizeF(512.0, 512.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    runtime->setVisibleItemRect(QRectF(0.0, 0.0, 512.0, 512.0));
    QTest::qWait(50);
    QCOMPARE(staticTileCount(*runtime), std::size_t(0));

    runtime->setZoomPercent(100.0);

    QTRY_VERIFY(staticTileCount(*runtime) > std::size_t(0));
}

void TestImageDocumentRuntime::smallFullImageSurfaceStillSchedulesAdjacentPredecode()
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

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader,
        [](const QByteArray &, const KiriView::ImageDecodeRequest &) {
            return staticDecodedImageWithPreview(QSize(1024, 1), QSize(1024, 1));
        },
        KiriView::fallbackTextureSizeMax);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    std::shared_ptr<KiriView::DisplayedImageSurface> surface = runtime->renderSnapshot().surface;
    QVERIFY(surface != nullptr);
    QVERIFY(surface->legacyFrameSurface() != nullptr);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, nextImageUrl);
}

void TestImageDocumentRuntime::replacementLoadFailureKeepsDisplayedImage()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    const quint64 displayedRevision = runtime->renderSnapshot().revision;
    const std::size_t loadCountBeforeReplacement = dataLoader.loadCount();

    runtime->setSourceUrl(missingUrl);
    QCOMPARE(dataLoader.loadCount(), loadCountBeforeReplacement + 1);
    QCOMPARE(dataLoader.backLoad().url, missingUrl);
    dataLoader.failBackLoad(QStringLiteral("missing"));

    QCOMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->sourceUrl(), imageUrl);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    QCOMPARE(runtime->errorString(), QStringLiteral("missing"));
    QCOMPARE(runtime->imageSize(), QSize(2, 1));
    QCOMPARE(runtime->renderSnapshot().revision, displayedRevision);
    QVERIFY(runtime->renderSnapshot().isRenderable());
}

void TestImageDocumentRuntime::decodedReplacementFailureSchedulesRecoveryPredecodeOnce()
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

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader, staticImageDataDecoderRejectingBadData(testImage(2)));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, badUrl);
    const std::size_t loadCountBeforeReplacement = dataLoader.loadCount();

    runtime->setSourceUrl(badUrl);
    QCOMPARE(dataLoader.loadCount(), loadCountBeforeReplacement + 1);
    QCOMPARE(dataLoader.backLoad().url, badUrl);
    dataLoader.finishBackLoad(QByteArrayLiteral("bad"));

    QTRY_COMPARE(runtime->errorString(), testImageDecodeFailureString());
    QCOMPARE(runtime->sourceUrl(), imageUrl);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeReplacement + 2);
    QCOMPARE(dataLoader.backLoad().url, badUrl);
}

void TestImageDocumentRuntime::emptyContainerNavigationClearsImageAndSelectsContainer()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(currentContainerUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->openNextContainer();

    QCOMPARE(runtime->status(), KiriView::ImageDocumentStatus::Error);
    QCOMPARE(runtime->sourceUrl(), targetContainerUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QVERIFY(runtime->containerNavigationAvailable());
    QVERIFY(!runtime->renderSnapshot().isRenderable());
    QCOMPARE(runtime->imageSize(), QSize());
    QVERIFY(!runtime->errorString().isEmpty());
}

void TestImageDocumentRuntime::deletingWithoutDisplayedImageDoesNotStartFileOperation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileOperationProvider fileOperations;

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));

    runtime->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(0));
    QVERIFY(!runtime->fileDeletionInProgress());
}

void TestImageDocumentRuntime::fileDeletionFailureKeepsDisplayedImageAndReportsError()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        staticImageDataDecoder(testImage(2)), KiriView::fallbackTextureSizeMax, 1.0,
        fileOperationProviderFor(fileOperations),
        [&deletionErrors](const QString &errorString) { deletionErrors.push_back(errorString); });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, imageUrl);
    QCOMPARE(fileOperations.backOperation().request.mode, KiriView::FileDeletionMode::MoveToTrash);
    QVERIFY(runtime->fileDeletionInProgress());

    fileOperations.finishBackOperation(
        KiriView::FileDeletionResult::Failed, QStringLiteral("permission denied"));

    QVERIFY(!runtime->fileDeletionInProgress());
    QCOMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->sourceUrl(), imageUrl);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    QCOMPARE(deletionErrors.size(), std::size_t(1));
    QCOMPARE(deletionErrors.front(), QStringLiteral("permission denied"));
    QVERIFY(runtime->renderSnapshot().isRenderable());
}

void TestImageDocumentRuntime::fileDeletionCancelKeepsDisplayedImageWithoutError()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        staticImageDataDecoder(testImage(2)), KiriView::fallbackTextureSizeMax, 1.0,
        fileOperationProviderFor(fileOperations),
        [&deletionErrors](const QString &errorString) { deletionErrors.push_back(errorString); });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(KiriView::FileDeletionMode::DeletePermanently);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, imageUrl);
    QCOMPARE(
        fileOperations.backOperation().request.mode, KiriView::FileDeletionMode::DeletePermanently);

    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Canceled);

    QVERIFY(!runtime->fileDeletionInProgress());
    QCOMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->sourceUrl(), imageUrl);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    QVERIFY(deletionErrors.empty());
    QVERIFY(runtime->renderSnapshot().isRenderable());
}

void TestImageDocumentRuntime::successfulFileDeletionOpensNextImageFallback()
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

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(currentUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(runtime->sourceUrl(), nextUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QCOMPARE(dataLoader.backLoad().url, nextUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), nextUrl);
}

void TestImageDocumentRuntime::successfulFileDeletionOpensPreviousImageFallback()
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

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(currentUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(runtime->sourceUrl(), previousUrl);
    QCOMPARE(dataLoader.backLoad().url, previousUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), previousUrl);
}

void TestImageDocumentRuntime::successfulFileDeletionWithoutFallbackClearsDocument()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileOperationProvider fileOperations;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")), {});

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(KiriView::FileDeletionMode::MoveToTrash);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(runtime->status(), KiriView::ImageDocumentStatus::Null);
    QCOMPARE(runtime->sourceUrl(), QUrl());
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QCOMPARE(runtime->imageSize(), QSize());
    QVERIFY(!runtime->renderSnapshot().isRenderable());
}

void TestImageDocumentRuntime::successfulComicBookDeletionOpensNextSiblingArchive()
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

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            KiriView::fallbackTextureSizeMax, 1.0, fileOperationProviderFor(fileOperations));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(currentArchiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(KiriView::FileDeletionMode::DeletePermanently);

    QCOMPARE(fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fileOperations.backOperation().request.targetUrl, currentArchiveUrl);
    fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(runtime->sourceUrl(), nextImageUrl);
    QCOMPARE(dataLoader.backLoad().url, nextImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), nextImageUrl);
    QVERIFY(runtime->containerNavigationAvailable());
}

void TestImageDocumentRuntime::rightToLeftReadingIsOnlyAvailableForComicArchives()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QVERIFY(!runtime->rightToLeftReadingEnabled());
    QVERIFY(!runtime->rightToLeftReadingAvailable());

    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), archivePageUrl);
    QVERIFY(runtime->rightToLeftReadingAvailable());
    QVERIFY(!runtime->rightToLeftReadingEnabled());
}

void TestImageDocumentRuntime::rightToLeftReadingPersistsAcrossSiblingArchiveNavigation()
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

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(firstArchiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), firstPageUrl);
    runtime->setRightToLeftReadingEnabled(true);
    QVERIFY(runtime->rightToLeftReadingEnabled());

    runtime->openNextContainer();
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondPageUrl);
    QVERIFY(runtime->rightToLeftReadingAvailable());
    QVERIFY(runtime->rightToLeftReadingEnabled());

    runtime->setSourceUrl(ordinaryImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), ordinaryImageUrl);
    QVERIFY(!runtime->rightToLeftReadingAvailable());
    QVERIFY(!runtime->rightToLeftReadingEnabled());
}

void TestImageDocumentRuntime::twoPageModeDisplaysCurrentAndNextComicArchivePages()
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
    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader, std::move(decoder));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), firstPageUrl);
    QVERIFY(runtime->twoPageModeAvailable());

    runtime->rotateClockwise();
    const KiriView::DisplayedImageRenderSnapshot rotatedSinglePageSnapshot
        = runtime->renderSnapshot();
    QVERIFY(rotatedSinglePageSnapshot.isRenderable());
    QCOMPARE(rotatedSinglePageSnapshot.imageSize, QSize(200, 100));
    QCOMPARE(rotatedSinglePageSnapshot.rotationDegrees, 90);

    runtime->setTwoPageModeEnabled(true);
    QVERIFY(runtime->twoPageModeEnabled());
    QVERIFY(!runtime->secondaryPageVisible());
    QCOMPARE(runtime->imageSize(), QSize(100, 200));

    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->displayedUrl(), secondPageUrl);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);

    QTRY_VERIFY(runtime->secondaryPageVisible());
    QCOMPARE(runtime->currentPageNumber(), 2);
    QCOMPARE(runtime->currentLastPageNumber(), 3);
    QCOMPARE(runtime->primaryImageSize(), QSize(100, 200));
    QCOMPARE(runtime->secondaryImageSize(), QSize(100, 200));
    QCOMPARE(runtime->imageSize(), QSize(200, 200));

    const KiriView::DisplayedImageRenderSnapshot primarySnapshot = runtime->renderSnapshot();
    const KiriView::DisplayedImageRenderSnapshot secondarySnapshot
        = runtime->renderSnapshot(KiriView::DisplayedPageRole::Secondary);
    QVERIFY(primarySnapshot.isRenderable());
    QVERIFY(secondarySnapshot.isRenderable());
    QCOMPARE(primarySnapshot.imageSize, QSize(100, 200));
    QCOMPARE(secondarySnapshot.imageSize, QSize(100, 200));
    QCOMPARE(primarySnapshot.rotationDegrees, 0);
    QCOMPARE(secondarySnapshot.rotationDegrees, 0);
}

void TestImageDocumentRuntime::twoPageModeUsesRightToLeftPageOrder()
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
    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, std::move(decoder), 64);
    runtime->setViewportSize(QSizeF(100.0, 200.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), firstPageUrl);
    runtime->setTwoPageModeEnabled(true);
    runtime->setRightToLeftReadingEnabled(true);
    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, thirdPageUrl));

    QTRY_VERIFY(runtime->secondaryPageVisible());
    QCOMPARE(runtime->currentPageNumber(), 2);
    QCOMPARE(runtime->currentLastPageNumber(), 3);
    runtime->setZoomPercent(100.0);
    runtime->setVisibleItemRect(QRectF(0.0, 0.0, 100.0, 200.0));

    QTRY_VERIFY(staticTileCount(*runtime, KiriView::DisplayedPageRole::Secondary) > 0);
    QCOMPARE(staticTileCount(*runtime, KiriView::DisplayedPageRole::Primary), std::size_t(0));
}

void TestImageDocumentRuntime::twoPageModeRightToLeftKeepsSinglePageNavigationSemantic()
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
    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader, std::move(decoder));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), firstPageUrl);
    runtime->setTwoPageModeEnabled(true);
    runtime->setRightToLeftReadingEnabled(true);
    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, thirdPageUrl));
    QTRY_COMPARE(runtime->currentPageNumber(), 2);
    QTRY_COMPARE(runtime->currentLastPageNumber(), 3);

    std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openNextSinglePage();
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->displayedUrl(), thirdPageUrl);
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(2));
    QTRY_COMPARE(dataLoader.backLoad().url, fourthPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, fourthPageUrl));
    QTRY_COMPARE(runtime->currentPageNumber(), 3);
    QTRY_COMPARE(runtime->currentLastPageNumber(), 4);

    loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openPreviousSinglePage();
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->displayedUrl(), secondPageUrl);
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(2));
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, thirdPageUrl));
    QTRY_COMPARE(runtime->currentPageNumber(), 2);
    QTRY_COMPARE(runtime->currentLastPageNumber(), 3);
}

void TestImageDocumentRuntime::twoPageModeWaitsForTargetSpreadBeforeRenderingNavigation()
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
    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader, std::move(decoder));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->displayedUrl(), firstPageUrl);

    runtime->setTwoPageModeEnabled(true);
    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);

    QTRY_VERIFY(runtime->secondaryPageVisible());
    QCOMPARE(runtime->displayedUrl(), secondPageUrl);
    QCOMPARE(runtime->currentLastPageNumber(), 3);
    QVERIFY(hasRenderableSnapshot(*runtime));
    QVERIFY(hasRenderableSnapshot(*runtime, KiriView::DisplayedPageRole::Secondary));

    runtime->openNextImage();

    QTRY_COMPARE(dataLoader.backLoad().url, fourthPageUrl);
    QCOMPARE(runtime->status(), KiriView::ImageDocumentStatus::Loading);
    QVERIFY(runtime->loading());
    QVERIFY(!hasRenderableSnapshot(*runtime));
    QVERIFY(!hasRenderableSnapshot(*runtime, KiriView::DisplayedPageRole::Secondary));
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), fourthPageUrl);
    QTRY_COMPARE(dataLoader.backLoad().url, fifthPageUrl);
    QCOMPARE(runtime->status(), KiriView::ImageDocumentStatus::Loading);
    QVERIFY(runtime->loading());
    QVERIFY(!hasRenderableSnapshot(*runtime));
    QVERIFY(!hasRenderableSnapshot(*runtime, KiriView::DisplayedPageRole::Secondary));
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), KiriView::ImageDocumentStatus::Ready);
    QTRY_VERIFY(runtime->secondaryPageVisible());
    QCOMPARE(runtime->displayedUrl(), fourthPageUrl);
    QCOMPARE(runtime->currentLastPageNumber(), 5);
    QVERIFY(hasRenderableSnapshot(*runtime));
    QVERIFY(hasRenderableSnapshot(*runtime, KiriView::DisplayedPageRole::Secondary));
}

void TestImageDocumentRuntime::twoPageModeLoadingNavigationUsesPendingPrimaryPage()
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
    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader, std::move(decoder));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->displayedUrl(), firstPageUrl);

    runtime->setTwoPageModeEnabled(true);
    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->currentPageNumber(), 2);
    QTRY_COMPARE(runtime->currentLastPageNumber(), 3);
    QVERIFY(runtime->secondaryPageVisible());

    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, fourthPageUrl);
    QCOMPARE(runtime->currentPageNumber(), 4);

    runtime->openNextImage();
    QTRY_COMPARE(dataLoader.backLoad().url, fifthPageUrl);
    QCOMPARE(runtime->currentPageNumber(), 5);
    QVERIFY(!finishOldestActiveLoadForUrl(dataLoader, fourthPageUrl));

    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), fifthPageUrl);
    QCOMPARE(runtime->currentPageNumber(), 5);
    QVERIFY(!runtime->secondaryPageVisible());
}

QTEST_GUILESS_MAIN(TestImageDocumentRuntime)

#include "test_imagedocumentruntime.moc"
