// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentruntime.h"

#include "image_test_support.h"
#include "location/imagedocumentlocation.h"
#include "rendering/imagerendering.h"

#include <QObject>
#include <QTemporaryDir>
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
using kiriview::TestSupport::archivePageUrl;
using kiriview::TestSupport::comicBookContainerCandidate;
using kiriview::TestSupport::fileDeletionProviderFor;
using kiriview::TestSupport::imageDocumentPageCandidate;
using kiriview::TestSupport::imageDocumentRuntimeDependencyOverridesFor;
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::ManualFileDeletionProvider;
using kiriview::TestSupport::ManualImageDataLoader;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::staticImageDataDecoder;
using kiriview::TestSupport::staticImageDataDecoderRejectingBadData;
using kiriview::TestSupport::testImage;
using kiriview::TestSupport::testImageDecodeFailureString;
using kiriview::TestSupport::videoCandidate;

using FakeCandidateProvider = kiriview::TestSupport::FakeImageDocumentPageCandidateProvider;

class RuntimeOwner final : public QObject
{
public:
    RuntimeOwner(QObject* parent,
        kiriview::ImageDocumentRuntime::RenderContextProvider renderContextProvider,
        kiriview::ImageDocumentRuntime::ChangeCallback changeCallback,
        kiriview::ImageDocumentRuntimeDependencyOverrides dependencies,
        kiriview::ImageDocumentRuntime::FileDeletionFailedCallback fileDeletionFailedCallback = {},
        kiriview::ImageDocumentRuntime::UnsupportedOpenedCollectionVideoEnteredCallback
            unsupportedOpenedCollectionVideoEnteredCallback
        = {})
        : QObject(parent)
        , runtime(this, std::move(renderContextProvider), std::move(changeCallback),
              std::move(dependencies), std::move(fileDeletionFailedCallback),
              std::move(unsupportedOpenedCollectionVideoEnteredCallback))
    {
    }

    kiriview::ImageDocumentRuntime runtime;
};

class RuntimeHandle final
{
public:
    RuntimeHandle(QObject* parent,
        kiriview::ImageDocumentRuntime::RenderContextProvider renderContextProvider,
        kiriview::ImageDocumentRuntime::ChangeCallback changeCallback,
        kiriview::ImageDocumentRuntimeDependencyOverrides dependencies,
        kiriview::ImageDocumentRuntime::FileDeletionFailedCallback fileDeletionFailedCallback = {},
        kiriview::ImageDocumentRuntime::UnsupportedOpenedCollectionVideoEnteredCallback
            unsupportedOpenedCollectionVideoEnteredCallback
        = {})
        : m_owner(std::make_unique<RuntimeOwner>(parent, std::move(renderContextProvider),
              std::move(changeCallback), std::move(dependencies),
              std::move(fileDeletionFailedCallback),
              std::move(unsupportedOpenedCollectionVideoEnteredCallback)))
    {
    }

    kiriview::ImageDocumentRuntime* operator->() { return &m_owner->runtime; }
    const kiriview::ImageDocumentRuntime* operator->() const { return &m_owner->runtime; }
    kiriview::ImageDocumentRuntime& operator*() { return m_owner->runtime; }
    const kiriview::ImageDocumentRuntime& operator*() const { return m_owner->runtime; }

private:
    std::unique_ptr<RuntimeOwner> m_owner;
};

class NonCacheableTestImageTileSource final : public kiriview::ImageTileSource
{
public:
    explicit NonCacheableTestImageTileSource(QImage image)
        : m_image(std::move(image))
    {
    }

    QSize imageSize() const override { return m_image.size(); }

    std::optional<kiriview::DecodedTile> decodeTile(
        const kiriview::TileRequest&, QString*) const override
    {
        return std::nullopt;
    }

    QImage decodeBlockingDisplayImage(int, QString*) const override { return m_image; }

    qsizetype byteCost() const override { return std::numeric_limits<qsizetype>::max() / 2; }

private:
    QImage m_image;
};

kiriview::DecodedImageResult staticDecodedImageWithPreview(
    const QSize& sourceSize, const QSize& previewSize, qreal firstDisplayPixelsPerSourcePixel = 0.0)
{
    return kiriview::successfulDecodedImageResult(kiriview::StaticDecodedImage {
        staticDisplayTestImagePayload(testImage(sourceSize), testImage(previewSize),
            firstDisplayPixelsPerSourcePixel,
            sourceSize == previewSize ? kiriview::DisplayImageQuality::Exact
                                      : kiriview::DisplayImageQuality::FirstDisplay),
    });
}

kiriview::DecodedImageResult singleFrameDecodedImage(const QSize& size)
{
    QImage image = testImage(size);
    return kiriview::successfulDecodedImageResult(kiriview::StaticDecodedImage {
        kiriview::StaticDisplayImagePayload {
            QStringLiteral("test-image"),
            {},
            image.size(),
            image,
            kiriview::DisplayImageQuality::Exact,
            1.0,
            {},
            std::make_shared<NonCacheableTestImageTileSource>(std::move(image)),
        },
    });
}

kiriview::DecodedImageResult invalidAnimationDecodedImage()
{
    return kiriview::successfulDecodedImageResult(kiriview::ApngAnimationImage {
        QImage(),
        QByteArrayLiteral("not-an-animation"),
        {},
        QStringLiteral("invalid-animation"),
    });
}

RuntimeHandle createRuntime(QObject* parent, FakeCandidateProvider& candidateProvider,
    ManualImageDataLoader& dataLoader,
    kiriview::ImageDataDecoder dataDecoder = staticImageDataDecoder(testImage(2)),
    int maximumTextureSize = kiriview::fallbackTextureSizeMax, qreal devicePixelRatio = 1.0,
    kiriview::FileDeletionProvider fileDeletionProvider = {},
    kiriview::ImageDocumentRuntime::FileDeletionFailedCallback fileDeletionFailedCallback = {},
    kiriview::ImageDocumentRuntime::UnsupportedOpenedCollectionVideoEnteredCallback
        unsupportedOpenedCollectionVideoEnteredCallback
    = {})
{
    return RuntimeHandle(
        parent,
        [maximumTextureSize, devicePixelRatio]() {
            return kiriview::ImageDocumentRenderContext {
                devicePixelRatio,
                maximumTextureSize,
            };
        },
        kiriview::ImageDocumentRuntime::ChangeCallback {},
        imageDocumentRuntimeDependencyOverridesFor(
            candidateProvider, dataLoader, std::move(dataDecoder), std::move(fileDeletionProvider)),
        std::move(fileDeletionFailedCallback),
        std::move(unsupportedOpenedCollectionVideoEnteredCallback));
}

void finishLoad(ManualImageDataLoader& dataLoader)
{
    dataLoader.finishBackLoad(QByteArrayLiteral("ok"));
}

bool finishOldestActiveLoadForUrl(ManualImageDataLoader& dataLoader, const QUrl& url)
{
    return dataLoader.finishOldestActiveLoadForUrl(url, QByteArrayLiteral("ok"));
}

bool hasReadyDisplaySourceProjection(const kiriview::ImageDocumentRuntime& runtime,
    kiriview::DisplayedPageRole role = kiriview::DisplayedPageRole::Primary)
{
    const kiriview::ImageDisplaySourceProjection projection = runtime.displaySourceProjection(role);
    return projection.visible && projection.status == kiriview::ImageDisplaySourceStatus::Ready
        && !projection.providerUrl.isEmpty();
}

bool containsChange(
    const std::vector<kiriview::ImageDocumentChange>& changes, kiriview::ImageDocumentChange change)
{
    return std::find(changes.begin(), changes.end(), change) != changes.end();
}
}

class TestImageDocumentRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initialLoadSuccessUpdatesDocumentState();
    void openedCollectionScopeProjectionsFollowDisplayedImageScope();
    void openedCollectionVideoPlaceholderKeepsNavigation();
    void imageLoadsUsePhysicalViewportForFirstDisplayDecode();
    void renderContextProviderCanBeReplacedAfterConstruction();
    void maximumManualZoomChangesAfterViewportImageAndRenderContextUpdates();
    void ordinaryDirectImagePageNavigationDoesNotUseSiblingDirectory();
    void archiveCollectionPageNavigationPreservesManualZoom();
    void rotationChangesLogicalSizeAndPreservesManualZoom();
    void rotationRecomputesFitZoomForRotatedBounds();
    void rotationResetsOnImageDocumentPageNavigation();
    void rotationResetsAndStopsWhileTwoPageModeIsEnabled();
    void pendingAdjacentNavigationSkipsIntermediateLoad();
    void pendingPageSelectionSupersedesEarlierLoad();
    void pageSelectionStartsTrackedLoadThroughEffectExecutor();
    void pendingLoadFailureKeepsTargetPageNavigation();
    void siblingArchiveNavigationResetsManualZoom();
    void anchoredZoomAndPointQueriesUseRuntimeViewportGeometry();
    void viewportPanAndScanCommandsUpdateContentPosition();
    void nextDisplayedImageCanStartAtFinalScanPosition();
    void smallStaticImagePublishesProviderDisplay();
    void largeStaticImagePublishesProviderPreview();
    void staticImageStillSchedulesAdjacentPredecode();
    void replacementLoadFailureSelectsTargetError();
    void decodedReplacementFailureSelectsTargetError();
    void loadFailurePreservesTypedFailureMetadataAndClearsOnNextLoad();
    void decodedLoadFailurePreservesTypedFailureMetadata();
    void presentationFailurePreservesTypedFailureMetadata();
    void emptyOpenedCollectionFailurePreservesTypedFailureMetadata();
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
    void twoPageModePublishesAndClearsSecondaryDisplaySourceProjection();
    void twoPageModeClearsPreviousSpreadWhileTargetSpreadLoads();
    void twoPageModeLoadingNavigationUsesPendingPrimaryPage();
    void displaySourceProjectionPublishesProviderForStaticDecode();
};

void TestImageDocumentRuntime::initialLoadSuccessUpdatesDocumentState()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    QVERIFY(!runtime->zoomPercentKnown());
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);

    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Loading);
    QVERIFY(!runtime->zoomPercentKnown());
    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QVERIFY(runtime->zoomPercentKnown());
    QCOMPARE(runtime->sourceUrl(), imageUrl);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    QCOMPARE(runtime->imageSize(), QSize(2, 1));
    QCOMPARE(runtime->currentPageNumber(), 0);
    QCOMPARE(runtime->pageCount(), 0);
    QVERIFY(!runtime->containerNavigationAvailable());
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));
}

void TestImageDocumentRuntime::displaySourceProjectionPublishesProviderForStaticDecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));

    kiriview::ImageDisplaySourceProjection projection
        = runtime->displaySourceProjection(kiriview::DisplayedPageRole::Primary);
    QVERIFY(!projection.visible);
    QCOMPARE(projection.status, kiriview::ImageDisplaySourceStatus::Missing);

    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    projection = runtime->displaySourceProjection(kiriview::DisplayedPageRole::Primary);
    QVERIFY(projection.visible);
    QCOMPARE(projection.pageRole, kiriview::DisplayedPageRole::Primary);
    QCOMPARE(projection.status, kiriview::ImageDisplaySourceStatus::Ready);
    QVERIFY(!projection.providerUrl.isEmpty());
    QCOMPARE(projection.revision, quint64(1));
    QCOMPARE(projection.revisionToken, QStringLiteral("1"));
    QCOMPARE(projection.sourceIdentity, QStringLiteral("test-image"));
    QCOMPARE(projection.originalSize, QSize(2, 1));
    QCOMPARE(projection.rasterSize, QSize(2, 1));
    QCOMPARE(projection.selectedSourceScope.kind,
        kiriview::ImagePresentationScopeKey::Kind::DirectImage);
    QCOMPARE(projection.selectedSourceScope.url, imageUrl);
}

void TestImageDocumentRuntime::openedCollectionScopeProjectionsFollowDisplayedImageScope()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    QVERIFY(!runtime->ordinaryDirectMediaScopeActive());
    QVERIFY(!runtime->openedCollectionScopeActive());
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QVERIFY(runtime->ordinaryDirectMediaScopeActive());
    QVERIFY(!runtime->openedCollectionScopeActive());

    const QUrl comicArchiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> comicArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(comicArchiveUrl);
    QVERIFY(comicArchiveCollection.has_value());
    const QUrl comicArchivePage
        = archivePageUrl(comicArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(
        comicArchiveCollection->rootUrl(), { imageDocumentPageCandidate(comicArchivePage) });

    RuntimeHandle archiveRuntime = createRuntime(this, candidateProvider, dataLoader);
    archiveRuntime->setViewportSize(QSizeF(400.0, 300.0));
    archiveRuntime->setSourceUrl(comicArchiveUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, comicArchivePage));

    QTRY_COMPARE(archiveRuntime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(archiveRuntime->displayedUrl(), comicArchivePage);
    QVERIFY(!archiveRuntime->ordinaryDirectMediaScopeActive());
    QVERIFY(archiveRuntime->openedCollectionScopeActive());

    const QUrl generalArchiveUrl = localUrl(QStringLiteral("/books/book.zip"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> generalArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(generalArchiveUrl);
    QVERIFY(generalArchiveCollection.has_value());
    const QUrl generalArchivePage
        = archivePageUrl(generalArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(
        generalArchiveCollection->rootUrl(), { imageDocumentPageCandidate(generalArchivePage) });

    RuntimeHandle generalArchiveRuntime = createRuntime(this, candidateProvider, dataLoader);
    generalArchiveRuntime->setViewportSize(QSizeF(400.0, 300.0));
    generalArchiveRuntime->setSourceUrl(generalArchiveUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, generalArchivePage));

    QTRY_COMPARE(generalArchiveRuntime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(generalArchiveRuntime->displayedUrl(), generalArchivePage);
    QVERIFY(!generalArchiveRuntime->ordinaryDirectMediaScopeActive());
    QVERIFY(generalArchiveRuntime->openedCollectionScopeActive());

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QUrl directoryUrl = localUrl(directory.path());
    const std::optional<kiriview::OpenedCollectionScopeLocation> directoryCollection
        = kiriview::openedCollectionScopeLocationForDirectlyOpenedLocalUrl(directoryUrl);
    QVERIFY(directoryCollection.has_value());
    const QUrl directoryPage
        = archivePageUrl(directoryCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(
        directoryCollection->rootUrl(), { imageDocumentPageCandidate(directoryPage) });

    RuntimeHandle directoryRuntime = createRuntime(this, candidateProvider, dataLoader);
    directoryRuntime->setViewportSize(QSizeF(400.0, 300.0));
    directoryRuntime->setSourceUrl(directoryUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, directoryPage));

    QTRY_COMPARE(directoryRuntime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(directoryRuntime->displayedUrl(), directoryPage);
    QVERIFY(!directoryRuntime->ordinaryDirectMediaScopeActive());
    QVERIFY(directoryRuntime->openedCollectionScopeActive());

    const QUrl openedCollectionEntryUrl(QStringLiteral("zip:///books/book.zip!/page.png"));
    candidateProvider.setDirectoryImages(QUrl(QStringLiteral("zip:///books/book.zip!/")),
        { imageDocumentPageCandidate(openedCollectionEntryUrl) });

    RuntimeHandle archiveEntryRuntime = createRuntime(this, candidateProvider, dataLoader);
    archiveEntryRuntime->setViewportSize(QSizeF(400.0, 300.0));
    archiveEntryRuntime->setSourceUrl(openedCollectionEntryUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, openedCollectionEntryUrl));

    QTRY_COMPARE(archiveEntryRuntime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(archiveEntryRuntime->displayedUrl(), openedCollectionEntryUrl);
    QVERIFY(archiveEntryRuntime->ordinaryDirectMediaScopeActive());
    QVERIFY(!archiveEntryRuntime->openedCollectionScopeActive());
}

void TestImageDocumentRuntime::openedCollectionVideoPlaceholderKeepsNavigation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<QString> unsupportedVideoMessages;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl videoUrl = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.mp4"));
    const QUrl thirdPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            videoCandidate(videoUrl),
            imageDocumentPageCandidate(thirdPageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        staticImageDataDecoder(testImage(2)), kiriview::fallbackTextureSizeMax, 1.0, {}, {},
        [&unsupportedVideoMessages](
            const QString& message) { unsupportedVideoMessages.push_back(message); });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), firstPageUrl);
    QCOMPARE(runtime->currentPageNumber(), 1);
    QCOMPARE(runtime->pageCount(), 3);
    QVERIFY(!runtime->unsupportedOpenedCollectionVideo());
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));

    const std::size_t loadCountBeforeVideo = dataLoader.loadCount();
    runtime->openNextPage();

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), videoUrl);
    QCOMPARE(runtime->currentPageNumber(), 2);
    QCOMPARE(runtime->pageCount(), 3);
    QVERIFY(runtime->unsupportedOpenedCollectionVideo());
    QVERIFY(!hasReadyDisplaySourceProjection(*runtime));
    QCOMPARE(dataLoader.loadCount(), loadCountBeforeVideo);
    QCOMPARE(unsupportedVideoMessages.size(), std::size_t(1));
    QCOMPARE(unsupportedVideoMessages.front(),
        QString::fromUtf8(
            "KiriView can\342\200\231t play videos inside directly opened archives or "
            "directories yet."));

    runtime->openNextPage();

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeVideo + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, thirdPageUrl);
    QVERIFY(!runtime->unsupportedOpenedCollectionVideo());
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), thirdPageUrl);
    QTRY_COMPARE(runtime->currentPageNumber(), 3);
    QCOMPARE(runtime->pageCount(), 3);
    QVERIFY(!runtime->unsupportedOpenedCollectionVideo());
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));
}

void TestImageDocumentRuntime::imageLoadsUsePhysicalViewportForFirstDisplayDecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl nextImageUrl = localUrl(QStringLiteral("/images/02.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
            imageDocumentPageCandidate(nextImageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        staticImageDataDecoder(testImage(2)), kiriview::fallbackTextureSizeMax, 2.0);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);

    QCOMPARE(dataLoader.loadCount(), std::size_t(1));
    QCOMPARE(dataLoader.frontLoad().firstDisplay.physicalViewportSize, QSize(800, 600));
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, nextImageUrl);
    QCOMPARE(dataLoader.backLoad().firstDisplay.physicalViewportSize, QSize(800, 600));
}

void TestImageDocumentRuntime::renderContextProviderCanBeReplacedAfterConstruction()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<kiriview::ImageDocumentChange> changes;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/regular.png"));

    RuntimeHandle runtime(
        this,
        []() {
            return kiriview::ImageDocumentRenderContext {
                1.0,
                kiriview::fallbackTextureSizeMax,
            };
        },
        [&changes](const std::vector<kiriview::ImageDocumentChange>& publishedChanges) {
            changes.insert(changes.end(), publishedChanges.begin(), publishedChanges.end());
        },
        imageDocumentRuntimeDependencyOverridesFor(candidateProvider, dataLoader,
            [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
                return staticDecodedImageWithPreview(QSize(1000, 500), QSize(1000, 500));
            }));

    runtime->setViewportSize(QSizeF(7000.0, 100.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 65536.0 * 100.0 / 1000.0));

    changes.clear();
    runtime->setRenderContextProvider([]() {
        return kiriview::ImageDocumentRenderContext {
            2.0,
            kiriview::fallbackTextureSizeMax,
        };
    });

    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::MaximumManualZoomPercent));
    QVERIFY(kiriview::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 65536.0 * 2.0 * 100.0 / 1000.0));
}

void TestImageDocumentRuntime::maximumManualZoomChangesAfterViewportImageAndRenderContextUpdates()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<kiriview::ImageDocumentChange> changes;
    qreal devicePixelRatio = 1.0;
    const QUrl firstImageUrl = localUrl(QStringLiteral("/images/regular.png"));
    const QUrl secondImageUrl = localUrl(QStringLiteral("/images/wide.png"));

    auto dataDecoder = [](const QByteArray&, const kiriview::ImageDecodeRequest& request) {
        const QSize sourceSize = request.imageUrl().fileName() == QStringLiteral("wide.png")
            ? QSize(2000, 1000)
            : QSize(1000, 500);
        return staticDecodedImageWithPreview(sourceSize, sourceSize);
    };
    RuntimeHandle runtime(
        this,
        [&devicePixelRatio]() {
            return kiriview::ImageDocumentRenderContext {
                devicePixelRatio,
                kiriview::fallbackTextureSizeMax,
            };
        },
        [&changes](const std::vector<kiriview::ImageDocumentChange>& publishedChanges) {
            changes.insert(changes.end(), publishedChanges.begin(), publishedChanges.end());
        },
        imageDocumentRuntimeDependencyOverridesFor(
            candidateProvider, dataLoader, std::move(dataDecoder)));

    runtime->setViewportSize(QSizeF(7000.0, 100.0));
    runtime->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 65536.0 * 100.0 / 1000.0));

    changes.clear();
    runtime->setViewportSize(QSizeF(9000.0, 100.0));
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::MaximumManualZoomPercent));
    QVERIFY(kiriview::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 9000.0 * 8.0 * 100.0 / 1000.0));

    changes.clear();
    devicePixelRatio = 2.0;
    runtime->updateRenderContext();
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::MaximumManualZoomPercent));
    QVERIFY(kiriview::imageZoomApproximatelyEqual(
        runtime->maximumManualZoomPercent(), 9000.0 * 8.0 * 2.0 * 100.0 / 1000.0));
}

void TestImageDocumentRuntime::ordinaryDirectImagePageNavigationDoesNotUseSiblingDirectory()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl firstImageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl secondImageUrl = localUrl(QStringLiteral("/images/02.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(firstImageUrl),
            imageDocumentPageCandidate(secondImageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(firstImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    runtime->requestManualZoomPercent(150.0);
    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Manual);

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openNextPage();

    QCOMPARE(dataLoader.loadCount(), loadCountBeforeNavigation);
    QCOMPARE(runtime->displayedUrl(), firstImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 0);
    QCOMPARE(runtime->pageCount(), 0);
    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Manual);
}

void TestImageDocumentRuntime::archiveCollectionPageNavigationPreservesManualZoom()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), firstPageUrl);
    runtime->requestManualZoomPercent(150.0);
    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Manual);

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openNextPage();

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondPageUrl);
    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Manual);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(runtime->zoomPercent(), 150.0));
}

void TestImageDocumentRuntime::rotationChangesLogicalSizeAndPreservesManualZoom()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<kiriview::ImageDocumentChange> changes;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/portrait.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime(
        this,
        []() {
            return kiriview::ImageDocumentRenderContext {
                1.0,
                kiriview::fallbackTextureSizeMax,
            };
        },
        [&changes](const std::vector<kiriview::ImageDocumentChange>& publishedChanges) {
            changes.insert(changes.end(), publishedChanges.begin(), publishedChanges.end());
        },
        imageDocumentRuntimeDependencyOverridesFor(candidateProvider, dataLoader,
            [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
                return staticDecodedImageWithPreview(QSize(100, 200), QSize(100, 200));
            }));
    runtime->setViewportSize(QSizeF(500.0, 500.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    runtime->requestManualZoomPercent(100.0);
    changes.clear();

    runtime->rotateClockwise();

    QCOMPARE(runtime->rotationDegrees(), 90);
    QCOMPARE(runtime->imageSize(), QSize(200, 100));
    QCOMPARE(runtime->primaryImageSize(), QSize(200, 100));
    QCOMPARE(runtime->displaySize(), QSizeF(200.0, 100.0));
    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Manual);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(runtime->zoomPercent(), 100.0));
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::Rotation));
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::ImageSize));
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::DisplaySize));
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::DisplaySource));

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
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
            return staticDecodedImageWithPreview(QSize(100, 200), QSize(100, 200));
        });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Fit);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(runtime->zoomPercent(), 150.0));
    QCOMPARE(runtime->displaySize(), QSizeF(150.0, 300.0));

    runtime->rotateClockwise();

    QCOMPARE(runtime->rotationDegrees(), 90);
    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Fit);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(runtime->zoomPercent(), 200.0));
    QCOMPARE(runtime->displaySize(), QSizeF(400.0, 200.0));
}

void TestImageDocumentRuntime::rotationResetsOnImageDocumentPageNavigation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
            return staticDecodedImageWithPreview(QSize(100, 200), QSize(100, 200));
        });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    runtime->rotateClockwise();
    QCOMPARE(runtime->rotationDegrees(), 90);
    QCOMPARE(runtime->imageSize(), QSize(200, 100));

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondPageUrl);
    QCOMPARE(runtime->rotationDegrees(), 0);
    QCOMPARE(runtime->imageSize(), QSize(100, 200));
}

void TestImageDocumentRuntime::rotationResetsAndStopsWhileTwoPageModeIsEnabled()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
            return staticDecodedImageWithPreview(QSize(100, 200), QSize(100, 200));
        });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
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
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/pending-adjacent.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstImageUrl),
            imageDocumentPageCandidate(secondImageUrl),
            imageDocumentPageCandidate(thirdImageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 2);

    runtime->openNextPage();
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
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/pending-selection.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstImageUrl),
            imageDocumentPageCandidate(secondImageUrl),
            imageDocumentPageCandidate(thirdImageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 2);

    runtime->openImageAtPage(3);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdImageUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
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
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/page-selection.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstImageUrl),
            imageDocumentPageCandidate(secondImageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    const std::size_t loadCountBeforeSelection = dataLoader.loadCount();
    runtime->openImageAtPage(2);

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeSelection + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(runtime->sourceUrl(), secondImageUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QCOMPARE(runtime->currentPageNumber(), 2);

    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondImageUrl);
    QCOMPARE(runtime->sourceUrl(), secondImageUrl);
    QCOMPARE(runtime->currentPageNumber(), 2);
}

void TestImageDocumentRuntime::pendingLoadFailureKeepsTargetPageNavigation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/pending-failure.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondImageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstImageUrl),
            imageDocumentPageCandidate(secondImageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondImageUrl);
    QCOMPARE(runtime->sourceUrl(), secondImageUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QCOMPARE(runtime->currentPageNumber(), 2);

    dataLoader.failBackLoad(QStringLiteral("missing"));

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Error);
    QCOMPARE(runtime->sourceUrl(), secondImageUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QCOMPARE(runtime->currentPageNumber(), 2);
    QCOMPARE(runtime->errorString(), QStringLiteral("missing"));
}

void TestImageDocumentRuntime::siblingArchiveNavigationResetsManualZoom()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl firstArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl secondArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> firstArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(firstArchiveUrl);
    const std::optional<kiriview::OpenedCollectionScopeLocation> secondArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(secondArchiveUrl);
    QVERIFY(firstArchiveCollection.has_value());
    QVERIFY(secondArchiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(firstArchiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
        });
    candidateProvider.setOpenedCollectionCandidates(secondArchiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(secondPageUrl),
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

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), firstPageUrl);
    runtime->requestManualZoomPercent(150.0);
    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Manual);

    const std::size_t loadCountBeforeNavigation = dataLoader.loadCount();
    runtime->openNextContainer();

    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(1));
    QCOMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), secondPageUrl);
    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Fit);
}

void TestImageDocumentRuntime::nextDisplayedImageCanStartAtFinalScanPosition()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(800, 800)));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), firstPageUrl);
    runtime->requestManualZoomPercent(100.0);
    runtime->requestViewportContentPosition(QPointF());
    runtime->requestNextDisplayedImageStartToFinalScanPosition();

    runtime->openNextSinglePage();
    QVERIFY(dataLoader.finishOldestActiveLoadForUrl(secondPageUrl, QByteArrayLiteral("ok")));
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), secondPageUrl);

    QVERIFY(runtime->requestDisplayedImageInitialContentPosition() > 0);

    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Manual);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(runtime->zoomPercent(), 100.0));
    QCOMPARE(runtime->viewportContentPosition(), QPointF(400.0, 500.0));
}

void TestImageDocumentRuntime::anchoredZoomAndPointQueriesUseRuntimeViewportGeometry()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(800, 800)));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    QCOMPARE(runtime->displaySize(), QSizeF(300.0, 300.0));
    QCOMPARE(runtime->viewportContentPosition(), QPointF());

    QVERIFY(!runtime->viewportPointInsideImage(QPointF(10.0, 10.0)));
    QVERIFY(runtime->viewportPointInsideImage(QPointF(60.0, 10.0)));
    QCOMPARE(runtime->nearestImageViewportPoint(QPointF(10.0, 10.0)), QPointF(50.0, 10.0));

    QVERIFY(runtime->requestManualZoomPercentAtCenter(100.0) > 0);

    QCOMPARE(runtime->zoomMode(), kiriview::ImageZoomMode::Manual);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(runtime->zoomPercent(), 100.0));
    QCOMPARE(runtime->displaySize(), QSizeF(800.0, 800.0));
    QCOMPARE(runtime->viewportContentPosition(), QPointF(200.0, 250.0));
}

void TestImageDocumentRuntime::viewportPanAndScanCommandsUpdateContentPosition()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(800, 800)));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    runtime->requestManualZoomPercent(100.0);
    QVERIFY(runtime->viewportPannable());

    QVERIFY(runtime->requestViewportPanToInitialScanPosition() > 0);
    QCOMPARE(runtime->viewportContentPosition(), QPointF());

    QVERIFY(runtime->requestViewportScanForward() > 0);
    QCOMPARE(runtime->viewportContentPosition(), QPointF(350.0, 0.0));

    QVERIFY(runtime->requestViewportScanBackward() > 0);
    QCOMPARE(runtime->viewportContentPosition(), QPointF());

    QVERIFY(runtime->requestViewportPanBy(QPointF(125.0, 75.0)) > 0);
    QCOMPARE(runtime->viewportContentPosition(), QPointF(125.0, 75.0));

    QVERIFY(runtime->requestViewportPanToFinalScanPosition() > 0);
    QCOMPARE(runtime->viewportContentPosition(), QPointF(400.0, 500.0));
}

void TestImageDocumentRuntime::smallStaticImagePublishesProviderDisplay()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/small.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader,
        [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
            return staticDecodedImageWithPreview(QSize(1024, 1), QSize(1024, 1));
        },
        kiriview::fallbackTextureSizeMax);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    const kiriview::ImageDisplaySourceProjection projection = runtime->displaySourceProjection();
    QVERIFY(projection.visible);
    QCOMPARE(projection.status, kiriview::ImageDisplaySourceStatus::Ready);
    QCOMPARE(projection.originalSize, QSize(1024, 1));
    QCOMPARE(projection.rasterSize, QSize(1024, 1));
    QCOMPARE(runtime->imageSize(), QSize(1024, 1));
}

void TestImageDocumentRuntime::largeStaticImagePublishesProviderPreview()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/large.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader,
        [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
            return staticDecodedImageWithPreview(
                QSize(kiriview::imageBlockingDisplayLongEdgeMax + 1, 1),
                QSize(kiriview::imageBlockingDisplayLongEdgeMax, 1));
        },
        kiriview::fallbackTextureSizeMax);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    const kiriview::ImageDisplaySourceProjection projection = runtime->displaySourceProjection();
    QVERIFY(projection.visible);
    QCOMPARE(projection.status, kiriview::ImageDisplaySourceStatus::Ready);
    QCOMPARE(projection.originalSize, QSize(kiriview::imageBlockingDisplayLongEdgeMax + 1, 1));
    QCOMPARE(projection.rasterSize, QSize(kiriview::imageBlockingDisplayLongEdgeMax, 1));
}

void TestImageDocumentRuntime::staticImageStillSchedulesAdjacentPredecode()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl nextImageUrl = localUrl(QStringLiteral("/images/02.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
            imageDocumentPageCandidate(nextImageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader,
        [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
            return staticDecodedImageWithPreview(QSize(1024, 1), QSize(1024, 1));
        },
        kiriview::fallbackTextureSizeMax);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, nextImageUrl);
}

void TestImageDocumentRuntime::replacementLoadFailureSelectsTargetError()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl missingUrl = localUrl(QStringLiteral("/images/missing.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
            imageDocumentPageCandidate(missingUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    kiriview::ImageDisplaySourceProjection projection
        = runtime->displaySourceProjection(kiriview::DisplayedPageRole::Primary);
    QVERIFY(projection.visible);
    QCOMPARE(projection.status, kiriview::ImageDisplaySourceStatus::Ready);
    QVERIFY(!projection.providerUrl.isEmpty());
    const std::size_t loadCountBeforeReplacement = dataLoader.loadCount();

    runtime->setSourceUrl(missingUrl);
    QCOMPARE(dataLoader.loadCount(), loadCountBeforeReplacement + 1);
    QCOMPARE(dataLoader.backLoad().url, missingUrl);
    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Loading);
    QCOMPARE(runtime->sourceUrl(), missingUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QVERIFY(!hasReadyDisplaySourceProjection(*runtime));
    projection = runtime->displaySourceProjection(kiriview::DisplayedPageRole::Primary);
    QVERIFY(!projection.visible);
    QCOMPARE(projection.status, kiriview::ImageDisplaySourceStatus::Missing);
    QVERIFY(projection.providerUrl.isEmpty());
    dataLoader.failBackLoad(QStringLiteral("missing"));

    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Error);
    QCOMPARE(runtime->sourceUrl(), missingUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QCOMPARE(runtime->errorString(), QStringLiteral("missing"));
    QVERIFY(runtime->imageSize().isEmpty());
    QVERIFY(!hasReadyDisplaySourceProjection(*runtime));
}

void TestImageDocumentRuntime::decodedReplacementFailureSelectsTargetError()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl badUrl = localUrl(QStringLiteral("/images/02.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
            imageDocumentPageCandidate(badUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader, staticImageDataDecoderRejectingBadData(testImage(2)));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QTRY_COMPARE(dataLoader.loadCount(), std::size_t(2));
    QCOMPARE(dataLoader.backLoad().url, badUrl);
    const std::size_t loadCountBeforeReplacement = dataLoader.loadCount();

    runtime->setSourceUrl(badUrl);
    QCOMPARE(dataLoader.loadCount(), loadCountBeforeReplacement + 1);
    QCOMPARE(dataLoader.backLoad().url, badUrl);
    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Loading);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    dataLoader.finishBackLoad(QByteArrayLiteral("bad"));

    QTRY_COMPARE(runtime->errorString(), testImageDecodeFailureString());
    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Error);
    QCOMPARE(runtime->sourceUrl(), badUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeReplacement + 1);
}

void TestImageDocumentRuntime::loadFailurePreservesTypedFailureMetadataAndClearsOnNextLoad()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/missing.png"));
    const QUrl nextImageUrl = localUrl(QStringLiteral("/images/next.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
            imageDocumentPageCandidate(nextImageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    dataLoader.failBackLoad(QStringLiteral("KIO data load failed"));

    QTRY_VERIFY(runtime->loadFailure().has_value());
    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Error);
    QCOMPARE(runtime->loadFailure()->sourceUrl, imageUrl);
    QVERIFY(runtime->loadFailure()->sessionId > 0);
    QVERIFY(runtime->loadFailure()->kind == kiriview::ImageLoadFailureKind::DataLoad);
    QCOMPARE(runtime->loadFailure()->userMessage, QStringLiteral("KIO data load failed"));
    QCOMPARE(runtime->loadFailure()->diagnosticDetail, QStringLiteral("KIO data load failed"));
    QVERIFY(runtime->loadFailure()->severity == kiriview::ImageLoadFailureSeverity::Error);
    QVERIFY(!runtime->loadFailure()->retryable);
    QCOMPARE(runtime->errorString(), runtime->loadFailure()->userMessage);

    runtime->setSourceUrl(nextImageUrl);

    QVERIFY(!runtime->loadFailure().has_value());
    QCOMPARE(runtime->errorString(), QString());
    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Loading);
}

void TestImageDocumentRuntime::decodedLoadFailurePreservesTypedFailureMetadata()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/bad.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(
        this, candidateProvider, dataLoader, staticImageDataDecoderRejectingBadData(testImage(2)));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    dataLoader.finishBackLoad(QByteArrayLiteral("bad"));

    QTRY_VERIFY(runtime->loadFailure().has_value());
    QCOMPARE(runtime->loadFailure()->sourceUrl, imageUrl);
    QVERIFY(runtime->loadFailure()->sessionId > 0);
    QVERIFY(runtime->loadFailure()->kind == kiriview::ImageLoadFailureKind::Decode);
    QCOMPARE(runtime->loadFailure()->userMessage, testImageDecodeFailureString());
    QCOMPARE(runtime->loadFailure()->diagnosticDetail, testImageDecodeFailureString());
    QVERIFY(runtime->loadFailure()->severity == kiriview::ImageLoadFailureSeverity::Error);
    QVERIFY(!runtime->loadFailure()->retryable);
    QCOMPARE(runtime->errorString(), runtime->loadFailure()->userMessage);
}

void TestImageDocumentRuntime::presentationFailurePreservesTypedFailureMetadata()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/invalid-animation.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
            return invalidAnimationDecodedImage();
        });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_VERIFY(runtime->loadFailure().has_value());
    QCOMPARE(runtime->loadFailure()->sourceUrl, imageUrl);
    QVERIFY(runtime->loadFailure()->sessionId > 0);
    QVERIFY(runtime->loadFailure()->kind == kiriview::ImageLoadFailureKind::Presentation);
    QCOMPARE(runtime->loadFailure()->userMessage,
        QStringLiteral("Could not decode the selected image animation."));
    QCOMPARE(runtime->loadFailure()->diagnosticDetail, runtime->loadFailure()->userMessage);
    QVERIFY(runtime->loadFailure()->severity == kiriview::ImageLoadFailureSeverity::Error);
    QVERIFY(!runtime->loadFailure()->retryable);
    QCOMPARE(runtime->errorString(), runtime->loadFailure()->userMessage);
}

void TestImageDocumentRuntime::emptyOpenedCollectionFailurePreservesTypedFailureMetadata()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/empty.cbz"));
    const std::optional<QUrl> archiveRootUrl = kiriview::comicBookArchiveRootUrl(archiveUrl);
    QVERIFY(archiveRootUrl.has_value());
    candidateProvider.setOpenedCollectionCandidates(*archiveRootUrl, {});

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);

    QVERIFY(runtime->loadFailure().has_value());
    QCOMPARE(runtime->loadFailure()->sourceUrl, archiveUrl);
    QVERIFY(runtime->loadFailure()->sessionId > 0);
    QVERIFY(runtime->loadFailure()->kind == kiriview::ImageLoadFailureKind::EmptyOpenedCollection);
    QCOMPARE(runtime->loadFailure()->userMessage,
        QStringLiteral("The selected collection does not contain any supported media."));
    QCOMPARE(runtime->loadFailure()->diagnosticDetail, QString());
    QVERIFY(runtime->loadFailure()->severity == kiriview::ImageLoadFailureSeverity::Error);
    QVERIFY(!runtime->loadFailure()->retryable);
    QCOMPARE(runtime->errorString(), runtime->loadFailure()->userMessage);
}

void TestImageDocumentRuntime::emptyContainerNavigationClearsImageAndSelectsContainer()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl currentContainerUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl targetContainerUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> currentArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(currentContainerUrl);
    const std::optional<kiriview::OpenedCollectionScopeLocation> targetArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(targetContainerUrl);
    QVERIFY(currentArchiveCollection.has_value());
    QVERIFY(targetArchiveCollection.has_value());
    const QUrl currentImageUrl
        = archivePageUrl(currentArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(currentArchiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(currentImageUrl),
        });
    candidateProvider.setContainerCandidates(localUrl(QStringLiteral("/books/")),
        {
            comicBookContainerCandidate(currentContainerUrl),
            comicBookContainerCandidate(targetContainerUrl),
        });
    candidateProvider.setOpenedCollectionCandidates(targetArchiveCollection->rootUrl(), {});

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(currentContainerUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->openNextContainer();

    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Error);
    QCOMPARE(runtime->sourceUrl(), targetContainerUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QVERIFY(runtime->containerNavigationAvailable());
    QVERIFY(!hasReadyDisplaySourceProjection(*runtime));
    QCOMPARE(runtime->imageSize(), QSize());
    QVERIFY(!runtime->errorString().isEmpty());
}

void TestImageDocumentRuntime::deletingWithoutDisplayedImageDoesNotStartFileOperation()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileDeletionProvider fileDeletionProvider;

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            kiriview::fallbackTextureSizeMax, 1.0, fileDeletionProviderFor(fileDeletionProvider));

    runtime->deleteDisplayedFile(kiriview::FileDeletionMode::MoveToTrash);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(0));
    QVERIFY(!runtime->fileDeletionInProgress());
}

void TestImageDocumentRuntime::fileDeletionFailureKeepsDisplayedImageAndReportsError()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileDeletionProvider fileDeletionProvider;
    std::vector<QString> deletionErrors;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        staticImageDataDecoder(testImage(2)), kiriview::fallbackTextureSizeMax, 1.0,
        fileDeletionProviderFor(fileDeletionProvider),
        [&deletionErrors](const QString& errorString) { deletionErrors.push_back(errorString); });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(kiriview::FileDeletionMode::MoveToTrash);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fileDeletionProvider.backOperation().request.targetUrl, imageUrl);
    QCOMPARE(
        fileDeletionProvider.backOperation().request.mode, kiriview::FileDeletionMode::MoveToTrash);
    QVERIFY(runtime->fileDeletionInProgress());

    fileDeletionProvider.finishBackOperation(
        kiriview::FileDeletionResult::Failed, QStringLiteral("permission denied"));

    QVERIFY(!runtime->fileDeletionInProgress());
    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->sourceUrl(), imageUrl);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    QCOMPARE(deletionErrors.size(), std::size_t(1));
    QCOMPARE(deletionErrors.front(), QStringLiteral("permission denied"));
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));
}

void TestImageDocumentRuntime::fileDeletionCancelKeepsDisplayedImageWithoutError()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileDeletionProvider fileDeletionProvider;
    std::vector<QString> deletionErrors;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader,
        staticImageDataDecoder(testImage(2)), kiriview::fallbackTextureSizeMax, 1.0,
        fileDeletionProviderFor(fileDeletionProvider),
        [&deletionErrors](const QString& errorString) { deletionErrors.push_back(errorString); });
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(kiriview::FileDeletionMode::DeletePermanently);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fileDeletionProvider.backOperation().request.targetUrl, imageUrl);
    QCOMPARE(fileDeletionProvider.backOperation().request.mode,
        kiriview::FileDeletionMode::DeletePermanently);

    fileDeletionProvider.finishBackOperation(kiriview::FileDeletionResult::Canceled);

    QVERIFY(!runtime->fileDeletionInProgress());
    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->sourceUrl(), imageUrl);
    QCOMPARE(runtime->displayedUrl(), imageUrl);
    QVERIFY(deletionErrors.empty());
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));
}

void TestImageDocumentRuntime::successfulFileDeletionOpensNextImageFallback()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileDeletionProvider fileDeletionProvider;
    const QUrl previousUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl currentUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl nextUrl = localUrl(QStringLiteral("/images/03.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(previousUrl),
            imageDocumentPageCandidate(nextUrl),
        });

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            kiriview::fallbackTextureSizeMax, 1.0, fileDeletionProviderFor(fileDeletionProvider));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(currentUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(kiriview::FileDeletionMode::MoveToTrash);
    fileDeletionProvider.finishBackOperation(kiriview::FileDeletionResult::Succeeded);

    QCOMPARE(runtime->sourceUrl(), nextUrl);
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QCOMPARE(dataLoader.backLoad().url, nextUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), nextUrl);
}

void TestImageDocumentRuntime::successfulFileDeletionOpensPreviousImageFallback()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileDeletionProvider fileDeletionProvider;
    const QUrl firstUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl previousUrl = localUrl(QStringLiteral("/images/02.png"));
    const QUrl currentUrl = localUrl(QStringLiteral("/images/03.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(firstUrl),
            imageDocumentPageCandidate(previousUrl),
        });

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            kiriview::fallbackTextureSizeMax, 1.0, fileDeletionProviderFor(fileDeletionProvider));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(currentUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(kiriview::FileDeletionMode::MoveToTrash);
    fileDeletionProvider.finishBackOperation(kiriview::FileDeletionResult::Succeeded);

    QCOMPARE(runtime->sourceUrl(), previousUrl);
    QCOMPARE(dataLoader.backLoad().url, previousUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), previousUrl);
}

void TestImageDocumentRuntime::successfulFileDeletionWithoutFallbackClearsDocument()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileDeletionProvider fileDeletionProvider;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")), {});

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            kiriview::fallbackTextureSizeMax, 1.0, fileDeletionProviderFor(fileDeletionProvider));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(kiriview::FileDeletionMode::MoveToTrash);
    fileDeletionProvider.finishBackOperation(kiriview::FileDeletionResult::Succeeded);

    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Null);
    QCOMPARE(runtime->sourceUrl(), QUrl());
    QCOMPARE(runtime->displayedUrl(), QUrl());
    QCOMPARE(runtime->imageSize(), QSize());
    QVERIFY(!hasReadyDisplaySourceProjection(*runtime));
}

void TestImageDocumentRuntime::successfulComicBookDeletionOpensNextSiblingArchive()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    ManualFileDeletionProvider fileDeletionProvider;
    const QUrl currentArchiveUrl = localUrl(QStringLiteral("/books/a.cbz"));
    const QUrl nextArchiveUrl = localUrl(QStringLiteral("/books/b.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> currentArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(currentArchiveUrl);
    const std::optional<kiriview::OpenedCollectionScopeLocation> nextArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(nextArchiveUrl);
    QVERIFY(currentArchiveCollection.has_value());
    QVERIFY(nextArchiveCollection.has_value());
    const QUrl currentImageUrl
        = archivePageUrl(currentArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl nextImageUrl
        = archivePageUrl(nextArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(currentArchiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(currentImageUrl),
        });
    candidateProvider.setContainerCandidates(localUrl(QStringLiteral("/books/")),
        {
            comicBookContainerCandidate(nextArchiveUrl),
        });
    candidateProvider.setOpenedCollectionCandidates(nextArchiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(nextImageUrl),
        });

    RuntimeHandle runtime
        = createRuntime(this, candidateProvider, dataLoader, staticImageDataDecoder(testImage(2)),
            kiriview::fallbackTextureSizeMax, 1.0, fileDeletionProviderFor(fileDeletionProvider));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(currentArchiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);

    runtime->deleteDisplayedFile(kiriview::FileDeletionMode::DeletePermanently);

    QCOMPARE(fileDeletionProvider.operationCount(), std::size_t(1));
    QCOMPARE(fileDeletionProvider.backOperation().request.targetUrl, currentArchiveUrl);
    fileDeletionProvider.finishBackOperation(kiriview::FileDeletionResult::Succeeded);

    QCOMPARE(runtime->sourceUrl(), nextImageUrl);
    QCOMPARE(dataLoader.backLoad().url, nextImageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), nextImageUrl);
    QVERIFY(runtime->containerNavigationAvailable());
}

void TestImageDocumentRuntime::rightToLeftReadingIsOnlyAvailableForComicArchives()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl imageUrl = localUrl(QStringLiteral("/images/01.png"));
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl archivePageUrl = kiriview::TestSupport::archivePageUrl(
        archiveCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(imageUrl),
        });
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(archivePageUrl),
        });

    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader);
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(imageUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
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
    const std::optional<kiriview::OpenedCollectionScopeLocation> firstArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(firstArchiveUrl);
    const std::optional<kiriview::OpenedCollectionScopeLocation> secondArchiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(secondArchiveUrl);
    QVERIFY(firstArchiveCollection.has_value());
    QVERIFY(secondArchiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(firstArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(secondArchiveCollection->rootUrl(), QStringLiteral("01.png"));
    candidateProvider.setOpenedCollectionCandidates(firstArchiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
        });
    candidateProvider.setOpenedCollectionCandidates(secondArchiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(secondPageUrl),
        });
    candidateProvider.setContainerCandidates(localUrl(QStringLiteral("/books/")),
        {
            comicBookContainerCandidate(firstArchiveUrl),
            comicBookContainerCandidate(secondArchiveUrl),
        });
    candidateProvider.setDirectoryImages(localUrl(QStringLiteral("/images/")),
        {
            imageDocumentPageCandidate(ordinaryImageUrl),
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
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
            imageDocumentPageCandidate(thirdPageUrl),
        });

    auto decoder = [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
        return singleFrameDecodedImage(QSize(100, 200));
    };
    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader, std::move(decoder));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QCOMPARE(runtime->displayedUrl(), firstPageUrl);
    QVERIFY(runtime->twoPageModeAvailable());

    runtime->rotateClockwise();
    const kiriview::ImageDisplaySourceProjection rotatedSinglePageProjection
        = runtime->displaySourceProjection();
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));
    QCOMPARE(runtime->imageSize(), QSize(200, 100));
    QCOMPARE(rotatedSinglePageProjection.rotationDegrees, 90);

    runtime->setTwoPageModeEnabled(true);
    QVERIFY(runtime->twoPageModeEnabled());
    QVERIFY(!runtime->secondaryPageVisible());
    QCOMPARE(runtime->imageSize(), QSize(100, 200));

    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->displayedUrl(), QUrl());
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);

    QTRY_VERIFY(runtime->secondaryPageVisible());
    QCOMPARE(runtime->currentPageNumber(), 2);
    QCOMPARE(runtime->currentLastPageNumber(), 3);
    QCOMPARE(runtime->primaryImageSize(), QSize(100, 200));
    QCOMPARE(runtime->secondaryImageSize(), QSize(100, 200));
    QCOMPARE(runtime->imageSize(), QSize(200, 200));

    const kiriview::ImageDisplaySourceProjection primaryProjection
        = runtime->displaySourceProjection();
    const kiriview::ImageDisplaySourceProjection secondaryProjection
        = runtime->displaySourceProjection(kiriview::DisplayedPageRole::Secondary);
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));
    QVERIFY(hasReadyDisplaySourceProjection(*runtime, kiriview::DisplayedPageRole::Secondary));
    QCOMPARE(primaryProjection.originalSize, QSize(100, 200));
    QCOMPARE(secondaryProjection.originalSize, QSize(100, 200));
    QCOMPARE(primaryProjection.rotationDegrees, 0);
    QCOMPARE(secondaryProjection.rotationDegrees, 0);
}

void TestImageDocumentRuntime::twoPageModeUsesRightToLeftPageOrder()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
            imageDocumentPageCandidate(thirdPageUrl),
        });

    auto decoder = [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
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
    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, thirdPageUrl));

    QTRY_VERIFY(runtime->secondaryPageVisible());
    QCOMPARE(runtime->currentPageNumber(), 2);
    QCOMPARE(runtime->currentLastPageNumber(), 3);
    runtime->requestManualZoomPercent(100.0);
    runtime->requestViewportContentPosition(QPointF(0.0, 0.0));

    QTRY_VERIFY(hasReadyDisplaySourceProjection(*runtime));
    QVERIFY(hasReadyDisplaySourceProjection(*runtime, kiriview::DisplayedPageRole::Secondary));
}

void TestImageDocumentRuntime::twoPageModeRightToLeftKeepsSinglePageNavigationSemantic()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    const QUrl fourthPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("04.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
            imageDocumentPageCandidate(thirdPageUrl),
            imageDocumentPageCandidate(fourthPageUrl),
        });

    auto decoder = [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
        return singleFrameDecodedImage(QSize(100, 200));
    };
    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader, std::move(decoder));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), firstPageUrl);
    runtime->setTwoPageModeEnabled(true);
    runtime->setRightToLeftReadingEnabled(true);
    runtime->openNextPage();
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
    QTRY_COMPARE(runtime->displayedUrl(), QUrl());
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
    QTRY_COMPARE(runtime->displayedUrl(), QUrl());
    QTRY_COMPARE(dataLoader.loadCount(), loadCountBeforeNavigation + std::size_t(2));
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    QVERIFY(finishOldestActiveLoadForUrl(dataLoader, thirdPageUrl));
    QTRY_COMPARE(runtime->currentPageNumber(), 2);
    QTRY_COMPARE(runtime->currentLastPageNumber(), 3);
}

void TestImageDocumentRuntime::twoPageModePublishesAndClearsSecondaryDisplaySourceProjection()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    std::vector<kiriview::ImageDocumentChange> changes;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
            imageDocumentPageCandidate(thirdPageUrl),
        });

    auto decoder = [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
        return singleFrameDecodedImage(QSize(100, 200));
    };
    RuntimeHandle runtime(
        this,
        []() {
            return kiriview::ImageDocumentRenderContext {
                1.0,
                kiriview::fallbackTextureSizeMax,
            };
        },
        [&changes](const std::vector<kiriview::ImageDocumentChange>& publishedChanges) {
            changes.insert(changes.end(), publishedChanges.begin(), publishedChanges.end());
        },
        imageDocumentRuntimeDependencyOverridesFor(
            candidateProvider, dataLoader, std::move(decoder)));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->displayedUrl(), firstPageUrl);

    runtime->setTwoPageModeEnabled(true);
    changes.clear();
    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);

    QTRY_VERIFY(runtime->secondaryPageVisible());
    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::DisplaySource));
    const kiriview::ImageDisplaySourceProjection primary
        = runtime->displaySourceProjection(kiriview::DisplayedPageRole::Primary);
    const kiriview::ImageDisplaySourceProjection secondary
        = runtime->displaySourceProjection(kiriview::DisplayedPageRole::Secondary);
    QVERIFY(primary.visible);
    QVERIFY(secondary.visible);
    QCOMPARE(secondary.pageRole, kiriview::DisplayedPageRole::Secondary);
    QCOMPARE(secondary.status, kiriview::ImageDisplaySourceStatus::Ready);
    QVERIFY(!secondary.providerUrl.isEmpty());
    QVERIFY(primary.providerUrl != secondary.providerUrl);
    QCOMPARE(secondary.revision, quint64(1));
    QCOMPARE(secondary.sourceIdentity, QStringLiteral("test-image"));
    QCOMPARE(secondary.selectedSourceScope.kind,
        kiriview::ImagePresentationScopeKey::Kind::OpenedCollection);

    changes.clear();
    runtime->setTwoPageModeEnabled(false);

    QVERIFY(containsChange(changes, kiriview::ImageDocumentChange::DisplaySource));
    const kiriview::ImageDisplaySourceProjection clearedSecondary
        = runtime->displaySourceProjection(kiriview::DisplayedPageRole::Secondary);
    QVERIFY(!clearedSecondary.visible);
    QCOMPARE(clearedSecondary.status, kiriview::ImageDisplaySourceStatus::Missing);
    QVERIFY(clearedSecondary.providerUrl.isEmpty());
}

void TestImageDocumentRuntime::twoPageModeClearsPreviousSpreadWhileTargetSpreadLoads()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    const QUrl fourthPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("04.png"));
    const QUrl fifthPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("05.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
            imageDocumentPageCandidate(thirdPageUrl),
            imageDocumentPageCandidate(fourthPageUrl),
            imageDocumentPageCandidate(fifthPageUrl),
        });

    auto decoder = [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
        return singleFrameDecodedImage(QSize(100, 200));
    };
    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader, std::move(decoder));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->displayedUrl(), firstPageUrl);

    runtime->setTwoPageModeEnabled(true);
    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);

    QTRY_VERIFY(runtime->secondaryPageVisible());
    QCOMPARE(runtime->displayedUrl(), secondPageUrl);
    QCOMPARE(runtime->currentLastPageNumber(), 3);
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));
    QVERIFY(hasReadyDisplaySourceProjection(*runtime, kiriview::DisplayedPageRole::Secondary));

    runtime->openNextPage();

    QTRY_COMPARE(dataLoader.backLoad().url, fourthPageUrl);
    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Loading);
    QVERIFY(runtime->loading());
    QVERIFY(!hasReadyDisplaySourceProjection(*runtime));
    QVERIFY(!hasReadyDisplaySourceProjection(*runtime, kiriview::DisplayedPageRole::Secondary));
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->displayedUrl(), QUrl());
    QTRY_COMPARE(dataLoader.backLoad().url, fifthPageUrl);
    QCOMPARE(runtime->status(), kiriview::ImageDocumentStatus::Loading);
    QVERIFY(runtime->loading());
    QVERIFY(!hasReadyDisplaySourceProjection(*runtime));
    QVERIFY(!hasReadyDisplaySourceProjection(*runtime, kiriview::DisplayedPageRole::Secondary));
    finishLoad(dataLoader);

    QTRY_COMPARE(runtime->status(), kiriview::ImageDocumentStatus::Ready);
    QTRY_VERIFY(runtime->secondaryPageVisible());
    QCOMPARE(runtime->displayedUrl(), fourthPageUrl);
    QCOMPARE(runtime->currentLastPageNumber(), 5);
    QVERIFY(hasReadyDisplaySourceProjection(*runtime));
    QVERIFY(hasReadyDisplaySourceProjection(*runtime, kiriview::DisplayedPageRole::Secondary));
}

void TestImageDocumentRuntime::twoPageModeLoadingNavigationUsesPendingPrimaryPage()
{
    FakeCandidateProvider candidateProvider;
    ManualImageDataLoader dataLoader;
    const QUrl archiveUrl = localUrl(QStringLiteral("/books/book.cbz"));
    const std::optional<kiriview::OpenedCollectionScopeLocation> archiveCollection
        = kiriview::openedCollectionScopeLocationForLocalArchiveUrl(archiveUrl);
    QVERIFY(archiveCollection.has_value());
    const QUrl firstPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("01.png"));
    const QUrl secondPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("02.png"));
    const QUrl thirdPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("03.png"));
    const QUrl fourthPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("04.png"));
    const QUrl fifthPageUrl
        = archivePageUrl(archiveCollection->rootUrl(), QStringLiteral("05.png"));
    candidateProvider.setOpenedCollectionCandidates(archiveCollection->rootUrl(),
        {
            imageDocumentPageCandidate(firstPageUrl),
            imageDocumentPageCandidate(secondPageUrl),
            imageDocumentPageCandidate(thirdPageUrl),
            imageDocumentPageCandidate(fourthPageUrl),
            imageDocumentPageCandidate(fifthPageUrl),
        });

    auto decoder = [](const QByteArray&, const kiriview::ImageDecodeRequest&) {
        return singleFrameDecodedImage(QSize(100, 200));
    };
    RuntimeHandle runtime = createRuntime(this, candidateProvider, dataLoader, std::move(decoder));
    runtime->setViewportSize(QSizeF(400.0, 300.0));
    runtime->setSourceUrl(archiveUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->displayedUrl(), firstPageUrl);

    runtime->setTwoPageModeEnabled(true);
    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, secondPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(dataLoader.backLoad().url, thirdPageUrl);
    finishLoad(dataLoader);
    QTRY_COMPARE(runtime->currentPageNumber(), 2);
    QTRY_COMPARE(runtime->currentLastPageNumber(), 3);
    QVERIFY(runtime->secondaryPageVisible());

    runtime->openNextPage();
    QTRY_COMPARE(dataLoader.backLoad().url, fourthPageUrl);
    QCOMPARE(runtime->currentPageNumber(), 4);

    runtime->openNextPage();
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
