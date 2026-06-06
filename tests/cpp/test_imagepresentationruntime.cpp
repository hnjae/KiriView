// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationruntime.h"

#include "image_test_support.h"
#include "presentation/imagezoomstate.h"
#include "rendering/imagerendering.h"

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <QUrl>

namespace {
using KiriView::TestSupport::localUrl;

KiriView::ImageDocumentRenderContext renderContext()
{
    return KiriView::ImageDocumentRenderContext {
        1.0,
        KiriView::fallbackTextureSizeMax,
        7,
    };
}

KiriView::ImagePresentationPageSlotSnapshot pageSlot(const QSize &imageSize, quint64 revision)
{
    return KiriView::ImagePresentationPageSlotSnapshot {
        revision,
        imageSize,
        true,
        {},
    };
}
}

class TestImagePresentationRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void openedCollectionPageChangePreservesZoomClearsPanAndResetsRotation();
    void directImageChangeResetsZoom();
    void spreadModePreservesZoomAndDisablesRotation();
    void previousActiveTransitionKeepsCommittedProjectionAuthoritative();
    void hiddenSecondaryProjectionIsNotVisible();
    void singlePageSnapshotDoesNotExposeSecondaryVisibility();
};

void TestImagePresentationRuntime::
    openedCollectionPageChangePreservesZoomClearsPanAndResetsRotation()
{
    KiriView::ImagePresentationRuntime runtime(renderContext);
    const KiriView::ImagePresentationScopeKey scope
        = KiriView::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz")));

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 600), 1), scope,
        KiriView::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.setZoomPercent(150.0);
    runtime.requestViewportContentPosition(QPointF(80.0, 40.0));
    QVERIFY(runtime.rotateClockwise());

    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 600), 2), scope,
        KiriView::ImagePresentationPrimaryChangePolicy::PreserveZoomAndClearPan);

    QCOMPARE(runtime.zoomMode(), KiriView::ImageZoomMode::Manual);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(runtime.zoomPercent(), 150.0));
    QCOMPARE(runtime.viewportContentPosition(), QPointF());
    QCOMPARE(runtime.rotationDegrees(), 0);
}

void TestImagePresentationRuntime::directImageChangeResetsZoom()
{
    KiriView::ImagePresentationRuntime runtime(renderContext);

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 600), 1),
        KiriView::ImagePresentationScopeKey::directImage(localUrl(QStringLiteral("/images/a.png"))),
        KiriView::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.setZoomPercent(175.0);

    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 600), 2),
        KiriView::ImagePresentationScopeKey::directImage(localUrl(QStringLiteral("/images/b.png"))),
        KiriView::ImagePresentationPrimaryChangePolicy::ResetZoom);

    QCOMPARE(runtime.zoomMode(), KiriView::ImageZoomMode::Fit);
}

void TestImagePresentationRuntime::spreadModePreservesZoomAndDisablesRotation()
{
    KiriView::ImagePresentationRuntime runtime(renderContext);

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 1200), 1),
        KiriView::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz"))),
        KiriView::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.commitSecondaryPageSlot(pageSlot(QSize(800, 1200), 2));
    runtime.setSecondaryPageVisible(true);
    runtime.setZoomPercent(140.0);
    QVERIFY(runtime.rotateClockwise());

    runtime.setMode(KiriView::ImagePresentationMode::TwoPageSpread);

    QCOMPARE(runtime.rotationDegrees(), 0);
    QCOMPARE(runtime.zoomMode(), KiriView::ImageZoomMode::Manual);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(runtime.zoomPercent(), 140.0));
    QVERIFY(!runtime.rotateClockwise());

    runtime.setMode(KiriView::ImagePresentationMode::SinglePage);

    QCOMPARE(runtime.zoomMode(), KiriView::ImageZoomMode::Manual);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(runtime.zoomPercent(), 140.0));
}

void TestImagePresentationRuntime::previousActiveTransitionKeepsCommittedProjectionAuthoritative()
{
    KiriView::ImagePresentationRuntime runtime(renderContext);

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 1200), 1),
        KiriView::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz"))),
        KiriView::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.commitSecondaryPageSlot(pageSlot(QSize(800, 1200), 2));
    runtime.setSecondaryPageVisible(true);
    runtime.setMode(KiriView::ImagePresentationMode::TwoPageSpread);
    QVERIFY(runtime.renderProjection(KiriView::DisplayedPageRole::Secondary).visible);

    QVERIFY(runtime.beginTransition());
    runtime.setSecondaryPageVisible(false);

    QCOMPARE(runtime.transitionState(), KiriView::ImagePresentationTransitionState::PreviousActive);
    QVERIFY(runtime.renderProjection(KiriView::DisplayedPageRole::Secondary).visible);

    QVERIFY(runtime.showTransitionPlaceholder());
    QVERIFY(!runtime.renderProjection(KiriView::DisplayedPageRole::Primary).visible);
    QVERIFY(!runtime.renderProjection(KiriView::DisplayedPageRole::Secondary).visible);

    QVERIFY(runtime.abortTransition());
    QCOMPARE(
        runtime.transitionState(), KiriView::ImagePresentationTransitionState::CommittedActive);
    QVERIFY(runtime.renderProjection(KiriView::DisplayedPageRole::Secondary).visible);
}

void TestImagePresentationRuntime::hiddenSecondaryProjectionIsNotVisible()
{
    KiriView::ImagePresentationRuntime runtime(renderContext);

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 1200), 1),
        KiriView::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz"))),
        KiriView::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.commitSecondaryPageSlot(pageSlot(QSize(800, 1200), 2));
    runtime.setMode(KiriView::ImagePresentationMode::TwoPageSpread);
    runtime.setSecondaryPageVisible(false);

    const KiriView::ImagePresentationRenderProjection projection
        = runtime.renderProjection(KiriView::DisplayedPageRole::Secondary);

    QVERIFY(!projection.visible);
    QVERIFY(projection.visibleItemRect.isEmpty());
}

void TestImagePresentationRuntime::singlePageSnapshotDoesNotExposeSecondaryVisibility()
{
    KiriView::ImagePresentationRuntime runtime(renderContext);

    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 1200), 1),
        KiriView::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz"))),
        KiriView::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.commitSecondaryPageSlot(pageSlot(QSize(800, 1200), 2));
    runtime.setSecondaryPageVisible(true);

    QCOMPARE(runtime.mode(), KiriView::ImagePresentationMode::SinglePage);
    QVERIFY(!runtime.secondaryPageVisible());
    QVERIFY(!runtime.snapshot().secondaryPageVisible);
}

QTEST_GUILESS_MAIN(TestImagePresentationRuntime)

#include "test_imagepresentationruntime.moc"
