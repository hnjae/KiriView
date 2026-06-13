// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadpresentationcontroller.h"

#include "image_test_support.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagepresentationruntime.h"
#include "presentation/imagepresentationstate.h"
#include "rendering/imagerendering.h"

#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>
#include <map>
#include <optional>
#include <vector>

namespace {
using kiriview::TestSupport::localUrl;
using kiriview::TestSupport::staticDisplayTestImagePayload;
using kiriview::TestSupport::testImage;

kiriview::ImageDocumentRenderContext renderContext()
{
    return kiriview::ImageDocumentRenderContext {
        1.0,
        kiriview::fallbackTextureSizeMax,
    };
}

kiriview::ImageCacheBudgets testCacheBudgets()
{
    return kiriview::ImageCacheBudgets {
        1024 * 1024,
        512 * 1024,
    };
}

kiriview::OpenedCollectionScopeLocation openedCollectionScope()
{
    return kiriview::OpenedCollectionScopeLocation::fromUrls(
        localUrl(QStringLiteral("/books/book.cbz")), localUrl(QStringLiteral("/books/")),
        kiriview::OpenedCollectionScopeKind::ComicBookArchive);
}

kiriview::DisplayedImageLocation displayedLocation(const QUrl &url)
{
    return kiriview::DisplayedImageLocation::fromOpenedCollectionScope(
        url, openedCollectionScope());
}

kiriview::ImageDocumentPageNavigationSnapshot navigationSnapshot(
    const std::vector<QUrl> &urls, int currentPageNumber)
{
    return kiriview::ImageDocumentPageNavigationSnapshot {
        kiriview::PageNavigationState {
            urls,
            currentPageNumber - 1,
        },
    };
}

bool projectionReady(const kiriview::ImageDisplaySourceProjection &projection)
{
    return projection.visible && projection.status == kiriview::ImageDisplaySourceStatus::Ready
        && !projection.providerUrl.isEmpty();
}

class SpreadPresentationFixture
{
public:
    SpreadPresentationFixture()
        : primaryPageSurface(&context, {}, testCacheBudgets())
        , presentationRuntime(renderContext)
        , controller(&context, renderContext, state, primaryPageSurface, presentationRuntime,
              kiriview::ImageSpreadPresentationController::Callbacks {
                  {},
                  [this](const QUrl &url) { return findPredecodedImage(url); },
                  [this]() { return snapshot; },
                  {},
              },
              {}, {}, testCacheBudgets())
    {
        controller.setViewportSize(QSizeF(800.0, 600.0));
    }

    void displayPrimaryPage(const QUrl &url, const QSize &imageSize, int pageNumber)
    {
        snapshot = navigationSnapshot(pageUrls, pageNumber);
        state.setDisplayedImageLocation(displayedLocation(url));
        primaryPageSurface.setStaticDisplayImage(
            staticDisplayTestImagePayload(testImage(imageSize)), false, renderContext());
        controller.commitPrimaryPageSlot(state.displayedImageLocation());
    }

    std::optional<kiriview::PredecodedImage> findPredecodedImage(const QUrl &url) const
    {
        const auto imageSize = predecodedImageSizes.find(url);
        if (imageSize == predecodedImageSizes.cend()) {
            return std::nullopt;
        }

        return kiriview::PredecodedImage {
            staticDisplayTestImagePayload(testImage(imageSize->second)),
            displayedLocation(url),
        };
    }

    QObject context;
    kiriview::ImageDocumentState state;
    kiriview::ImagePageSurfaceController primaryPageSurface;
    kiriview::ImagePresentationRuntime presentationRuntime;
    std::vector<QUrl> pageUrls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
        localUrl(QStringLiteral("/books/004.png")),
        localUrl(QStringLiteral("/books/005.png")),
        localUrl(QStringLiteral("/books/006.png")),
    };
    std::map<QUrl, QSize> predecodedImageSizes;
    kiriview::ImageDocumentPageNavigationSnapshot snapshot;
    kiriview::ImageSpreadPresentationController controller;
};
}

class TestImageSpreadPresentationController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pageWidthCacheBelongsToSpreadNavigationOwner();
    void spreadVisibleRectOwnsPageVisibleRectProjection();
    void stillImageLoadOutcomeUpdatesDisplayProjection();
    void primaryDisplaySourceChangeRefreshesCommittedProjection();
    void spreadZoomDoesNotMutatePageZoomOwners();
    void spreadZoomRestoresToSinglePageWithoutMutatingPageZoomOwner();
    void transitionPhaseKeepsPreviousActiveUntilPlaceholder();
};

void TestImageSpreadPresentationController::pageWidthCacheBelongsToSpreadNavigationOwner()
{
    SpreadPresentationFixture fixture;

    fixture.displayPrimaryPage(fixture.pageUrls.at(3), QSize(1200, 800), 4);
    fixture.controller.setTwoPageModeEnabled(true);
    QVERIFY(fixture.controller.twoPageModeActive());
    QVERIFY(!fixture.controller.secondaryPageVisible());

    fixture.predecodedImageSizes[fixture.pageUrls.at(5)] = QSize(800, 1200);
    fixture.displayPrimaryPage(fixture.pageUrls.at(4), QSize(800, 1200), 5);
    fixture.controller.refreshSecondaryPage();
    QVERIFY(fixture.controller.secondaryPageVisible());

    const kiriview::ImageSpreadPageNavigationTarget target
        = fixture.controller.imageDocumentPageNavigationTarget(
            kiriview::NavigationDirection::Previous);

    QVERIFY(target.handledBySpread);
    QCOMPARE(target.pageNumber, 4);
}

void TestImageSpreadPresentationController::spreadVisibleRectOwnsPageVisibleRectProjection()
{
    SpreadPresentationFixture fixture;

    fixture.displayPrimaryPage(fixture.pageUrls.at(4), QSize(800, 1200), 5);
    fixture.predecodedImageSizes[fixture.pageUrls.at(5)] = QSize(800, 1200);
    fixture.controller.setTwoPageModeEnabled(true);
    fixture.controller.refreshSecondaryPage();
    QVERIFY(fixture.controller.secondaryPageVisible());

    fixture.controller.requestManualZoomPercent(100.0);
    fixture.controller.requestViewportContentPosition(QPointF(800.0, 0.0));

    QCOMPARE(fixture.controller.visibleItemRect(), QRectF(800.0, 0.0, 800.0, 600.0));
    QVERIFY(fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Primary)
            .visibleItemRect.isEmpty());
    QCOMPARE(fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Secondary)
                 .visibleItemRect,
        QRectF(0.0, 0.0, 800.0, 600.0));

    fixture.controller.setRightToLeftReadingEnabled(true);

    QCOMPARE(fixture.controller.visibleItemRect(), QRectF(800.0, 0.0, 800.0, 600.0));
    QCOMPARE(fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Primary)
                 .visibleItemRect,
        QRectF(0.0, 0.0, 800.0, 600.0));
    QVERIFY(fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Secondary)
            .visibleItemRect.isEmpty());
}

void TestImageSpreadPresentationController::stillImageLoadOutcomeUpdatesDisplayProjection()
{
    SpreadPresentationFixture fixture;

    fixture.displayPrimaryPage(fixture.pageUrls.at(0), QSize(800, 1200), 1);

    kiriview::ImageDisplaySourceProjection projection
        = fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Primary);
    QVERIFY(projectionReady(projection));
    QVERIFY(projection.loadAcknowledgmentRequired);

    fixture.controller.acknowledgeDisplayImageLoad(kiriview::DisplayedPageRole::Primary,
        projection.providerUrl, projection.revision, projection.sourceIdentity,
        kiriview::ImageDisplayLoadOutcome::Loaded);

    projection = fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Primary);
    QVERIFY(projectionReady(projection));
    QVERIFY(!projection.loadAcknowledgmentRequired);
}

void TestImageSpreadPresentationController::primaryDisplaySourceChangeRefreshesCommittedProjection()
{
    SpreadPresentationFixture fixture;

    fixture.displayPrimaryPage(fixture.pageUrls.at(0), QSize(800, 1200), 1);

    const kiriview::ImageDisplaySourceProjection firstProjection
        = fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Primary);
    QVERIFY(projectionReady(firstProjection));

    fixture.primaryPageSurface.setAnimationFrame(
        testImage(QSize(800, 1200)), QStringLiteral("animated-frame"));
    const kiriview::ImagePresentationPageSlotSnapshot frameSurfaceSnapshot
        = fixture.primaryPageSurface.snapshot();
    QVERIFY(frameSurfaceSnapshot.displaySource.revision != firstProjection.revision);
    QCOMPARE(frameSurfaceSnapshot.displaySource.sourceIdentity, QStringLiteral("animated-frame"));

    fixture.controller.handleDocumentChange(kiriview::ImageDocumentChange::DisplaySource);

    const kiriview::ImageDisplaySourceProjection frameProjection
        = fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Primary);
    QVERIFY(projectionReady(frameProjection));
    QCOMPARE(frameProjection.revision, frameSurfaceSnapshot.displaySource.revision);
    QCOMPARE(frameProjection.sourceIdentity, frameSurfaceSnapshot.displaySource.sourceIdentity);
}

void TestImageSpreadPresentationController::spreadZoomDoesNotMutatePageZoomOwners()
{
    SpreadPresentationFixture fixture;

    fixture.displayPrimaryPage(fixture.pageUrls.at(4), QSize(800, 1200), 5);
    fixture.predecodedImageSizes[fixture.pageUrls.at(5)] = QSize(800, 1200);
    fixture.controller.setTwoPageModeEnabled(true);
    fixture.controller.refreshSecondaryPage();
    QVERIFY(fixture.controller.secondaryPageVisible());

    fixture.controller.requestManualZoomPercent(125.0);

    QCOMPARE(fixture.controller.zoomMode(), kiriview::ImageZoomMode::Manual);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(fixture.controller.zoomPercent(), 125.0));
}

void TestImageSpreadPresentationController::
    spreadZoomRestoresToSinglePageWithoutMutatingPageZoomOwner()
{
    SpreadPresentationFixture fixture;

    fixture.displayPrimaryPage(fixture.pageUrls.at(4), QSize(800, 1200), 5);
    fixture.predecodedImageSizes[fixture.pageUrls.at(5)] = QSize(800, 1200);
    fixture.controller.setTwoPageModeEnabled(true);
    fixture.controller.refreshSecondaryPage();
    QVERIFY(fixture.controller.secondaryPageVisible());

    fixture.controller.requestManualZoomPercent(150.0);
    fixture.controller.setTwoPageModeEnabled(false);

    QVERIFY(!fixture.controller.twoPageModeActive());
    QCOMPARE(fixture.controller.zoomMode(), kiriview::ImageZoomMode::Manual);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(fixture.controller.zoomPercent(), 150.0));
}

void TestImageSpreadPresentationController::transitionPhaseKeepsPreviousActiveUntilPlaceholder()
{
    SpreadPresentationFixture fixture;

    fixture.displayPrimaryPage(fixture.pageUrls.at(1), QSize(800, 1200), 2);
    fixture.predecodedImageSizes[fixture.pageUrls.at(2)] = QSize(800, 1200);
    fixture.controller.setTwoPageModeEnabled(true);
    fixture.controller.refreshSecondaryPage();
    QVERIFY(fixture.controller.secondaryPageVisible());
    QCOMPARE(fixture.controller.presentationTransitionState(),
        kiriview::ImagePresentationTransitionState::CommittedActive);

    fixture.controller.beginTransition();

    QCOMPARE(fixture.controller.presentationTransitionState(),
        kiriview::ImagePresentationTransitionState::PreviousActive);
    QVERIFY(projectionReady(
        fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Primary)));
    QVERIFY(projectionReady(
        fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Secondary)));

    fixture.controller.clearSecondaryPage();

    QVERIFY(projectionReady(
        fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Secondary)));

    fixture.controller.showTransitionPlaceholder();

    QCOMPARE(fixture.controller.presentationTransitionState(),
        kiriview::ImagePresentationTransitionState::TransitioningPlaceholder);
    QVERIFY(!projectionReady(
        fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Primary)));
    QVERIFY(!projectionReady(
        fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Secondary)));

    fixture.controller.abortTransition();

    QCOMPARE(fixture.controller.presentationTransitionState(),
        kiriview::ImagePresentationTransitionState::CommittedActive);
    QVERIFY(projectionReady(
        fixture.controller.displaySourceProjection(kiriview::DisplayedPageRole::Primary)));
}

QTEST_GUILESS_MAIN(TestImageSpreadPresentationController)

#include "test_imagespreadpresentationcontroller.moc"
