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
using KiriView::TestSupport::localUrl;
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

KiriView::ImageDocumentRenderContext renderContext()
{
    return KiriView::ImageDocumentRenderContext {
        1.0,
        KiriView::fallbackTextureSizeMax,
    };
}

KiriView::ImageCacheBudgets testCacheBudgets()
{
    return KiriView::ImageCacheBudgets {
        1024 * 1024,
        512 * 1024,
    };
}

KiriView::OpenedCollectionScopeLocation openedCollectionScope()
{
    return KiriView::OpenedCollectionScopeLocation::fromUrls(
        localUrl(QStringLiteral("/books/book.cbz")), localUrl(QStringLiteral("/books/")),
        KiriView::OpenedCollectionScopeKind::ComicBookArchive);
}

KiriView::DisplayedImageLocation displayedLocation(const QUrl &url)
{
    return KiriView::DisplayedImageLocation::fromOpenedCollectionScope(
        url, openedCollectionScope());
}

KiriView::ImageDocumentPageNavigationSnapshot navigationSnapshot(
    const std::vector<QUrl> &urls, int currentPageNumber)
{
    return KiriView::ImageDocumentPageNavigationSnapshot {
        KiriView::PageNavigationState {
            urls,
            currentPageNumber - 1,
        },
    };
}

class SpreadPresentationFixture
{
public:
    SpreadPresentationFixture()
        : primaryPageSurface(&context, {}, testCacheBudgets())
        , presentationRuntime(renderContext)
        , controller(&context, renderContext, state, primaryPageSurface, presentationRuntime,
              KiriView::ImageSpreadPresentationController::Callbacks {
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
        primaryPageSurface.setStaticImage(
            staticTestImagePayload(testImage(imageSize)), false, renderContext());
        controller.commitPrimaryPageSlot(state.displayedImageLocation());
    }

    std::optional<KiriView::PredecodedImage> findPredecodedImage(const QUrl &url) const
    {
        const auto imageSize = predecodedImageSizes.find(url);
        if (imageSize == predecodedImageSizes.cend()) {
            return std::nullopt;
        }

        return KiriView::PredecodedImage {
            staticTestImagePayload(testImage(imageSize->second)),
            displayedLocation(url),
        };
    }

    QObject context;
    KiriView::ImageDocumentState state;
    KiriView::ImagePageSurfaceController primaryPageSurface;
    KiriView::ImagePresentationRuntime presentationRuntime;
    std::vector<QUrl> pageUrls {
        localUrl(QStringLiteral("/books/001.png")),
        localUrl(QStringLiteral("/books/002.png")),
        localUrl(QStringLiteral("/books/003.png")),
        localUrl(QStringLiteral("/books/004.png")),
        localUrl(QStringLiteral("/books/005.png")),
        localUrl(QStringLiteral("/books/006.png")),
    };
    std::map<QUrl, QSize> predecodedImageSizes;
    KiriView::ImageDocumentPageNavigationSnapshot snapshot;
    KiriView::ImageSpreadPresentationController controller;
};
}

class TestImageSpreadPresentationController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pageWidthCacheBelongsToSpreadNavigationOwner();
    void spreadVisibleRectOwnsPageVisibleRectProjection();
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

    const KiriView::ImageSpreadPageNavigationTarget target
        = fixture.controller.imageDocumentPageNavigationTarget(
            KiriView::NavigationDirection::Previous);

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
    QVERIFY(fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Primary)
            .visibleItemRect.isEmpty());
    QCOMPARE(
        fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Secondary).visibleItemRect,
        QRectF(0.0, 0.0, 800.0, 600.0));

    fixture.controller.setRightToLeftReadingEnabled(true);

    QCOMPARE(fixture.controller.visibleItemRect(), QRectF(800.0, 0.0, 800.0, 600.0));
    QCOMPARE(
        fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Primary).visibleItemRect,
        QRectF(0.0, 0.0, 800.0, 600.0));
    QVERIFY(fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Secondary)
            .visibleItemRect.isEmpty());
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

    QCOMPARE(fixture.controller.zoomMode(), KiriView::ImageZoomMode::Manual);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(fixture.controller.zoomPercent(), 125.0));
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
    QCOMPARE(fixture.controller.zoomMode(), KiriView::ImageZoomMode::Manual);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(fixture.controller.zoomPercent(), 150.0));
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
        KiriView::ImagePresentationTransitionState::CommittedActive);

    fixture.controller.beginTransition();

    QCOMPARE(fixture.controller.presentationTransitionState(),
        KiriView::ImagePresentationTransitionState::PreviousActive);
    QVERIFY(fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Primary).isRenderable());
    QVERIFY(
        fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Secondary).isRenderable());

    fixture.controller.clearSecondaryPage();

    QVERIFY(
        fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Secondary).isRenderable());

    fixture.controller.showTransitionPlaceholder();

    QCOMPARE(fixture.controller.presentationTransitionState(),
        KiriView::ImagePresentationTransitionState::TransitioningPlaceholder);
    QVERIFY(
        !fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Primary).isRenderable());
    QVERIFY(
        !fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Secondary).isRenderable());

    fixture.controller.abortTransition();

    QCOMPARE(fixture.controller.presentationTransitionState(),
        KiriView::ImagePresentationTransitionState::CommittedActive);
    QVERIFY(fixture.controller.renderSnapshot(KiriView::DisplayedPageRole::Primary).isRenderable());
}

QTEST_GUILESS_MAIN(TestImageSpreadPresentationController)

#include "test_imagespreadpresentationcontroller.moc"
