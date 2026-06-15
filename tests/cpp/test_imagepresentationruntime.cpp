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
using kiriview::TestSupport::localUrl;

kiriview::ImageDocumentRenderContext renderContext()
{
    return kiriview::ImageDocumentRenderContext {
        1.0,
        kiriview::fallbackTextureSizeMax,
        7,
    };
}

kiriview::ImagePresentationPageSlotSnapshot pageSlot(const QSize &imageSize, quint64 revision)
{
    return kiriview::ImagePresentationPageSlotSnapshot {
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
    void providerReadyProjectionRequiresProviderUrl();
    void singlePageSnapshotDoesNotExposeSecondaryVisibility();
};

void TestImagePresentationRuntime::
    openedCollectionPageChangePreservesZoomClearsPanAndResetsRotation()
{
    kiriview::ImagePresentationRuntime runtime(renderContext);
    const kiriview::ImagePresentationScopeKey scope
        = kiriview::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz")));

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 600), 1), scope,
        kiriview::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.setZoomPercent(150.0);
    runtime.requestViewportContentPosition(QPointF(80.0, 40.0));
    QVERIFY(runtime.rotateClockwise());

    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 600), 2), scope,
        kiriview::ImagePresentationPrimaryChangePolicy::PreserveZoomAndClearPan);

    QCOMPARE(runtime.zoomMode(), kiriview::ImageZoomMode::Manual);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(runtime.zoomPercent(), 150.0));
    QCOMPARE(runtime.viewportContentPosition(), QPointF());
    QCOMPARE(runtime.rotationDegrees(), 0);
}

void TestImagePresentationRuntime::directImageChangeResetsZoom()
{
    kiriview::ImagePresentationRuntime runtime(renderContext);

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 600), 1),
        kiriview::ImagePresentationScopeKey::directImage(localUrl(QStringLiteral("/images/a.png"))),
        kiriview::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.setZoomPercent(175.0);

    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 600), 2),
        kiriview::ImagePresentationScopeKey::directImage(localUrl(QStringLiteral("/images/b.png"))),
        kiriview::ImagePresentationPrimaryChangePolicy::ResetZoom);

    QCOMPARE(runtime.zoomMode(), kiriview::ImageZoomMode::Fit);
}

void TestImagePresentationRuntime::spreadModePreservesZoomAndDisablesRotation()
{
    kiriview::ImagePresentationRuntime runtime(renderContext);

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 1200), 1),
        kiriview::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz"))),
        kiriview::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.commitSecondaryPageSlot(pageSlot(QSize(800, 1200), 2));
    runtime.setSecondaryPageVisible(true);
    runtime.setZoomPercent(140.0);
    QVERIFY(runtime.rotateClockwise());

    runtime.setMode(kiriview::ImagePresentationMode::TwoPageSpread);

    QCOMPARE(runtime.rotationDegrees(), 0);
    QCOMPARE(runtime.zoomMode(), kiriview::ImageZoomMode::Manual);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(runtime.zoomPercent(), 140.0));
    QVERIFY(!runtime.rotateClockwise());

    runtime.setMode(kiriview::ImagePresentationMode::SinglePage);

    QCOMPARE(runtime.zoomMode(), kiriview::ImageZoomMode::Manual);
    QVERIFY(kiriview::imageZoomApproximatelyEqual(runtime.zoomPercent(), 140.0));
}

void TestImagePresentationRuntime::previousActiveTransitionKeepsCommittedProjectionAuthoritative()
{
    kiriview::ImagePresentationRuntime runtime(renderContext);

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 1200), 1),
        kiriview::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz"))),
        kiriview::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.commitSecondaryPageSlot(pageSlot(QSize(800, 1200), 2));
    runtime.setSecondaryPageVisible(true);
    runtime.setMode(kiriview::ImagePresentationMode::TwoPageSpread);
    QVERIFY(runtime.renderProjection(kiriview::DisplayedPageRole::Secondary).visible);

    QVERIFY(runtime.beginTransition());
    runtime.setSecondaryPageVisible(false);

    QCOMPARE(runtime.transitionState(), kiriview::ImagePresentationTransitionState::PreviousActive);
    QVERIFY(runtime.renderProjection(kiriview::DisplayedPageRole::Secondary).visible);

    QVERIFY(runtime.showTransitionPlaceholder());
    QVERIFY(!runtime.renderProjection(kiriview::DisplayedPageRole::Primary).visible);
    QVERIFY(!runtime.renderProjection(kiriview::DisplayedPageRole::Secondary).visible);

    QVERIFY(runtime.abortTransition());
    QCOMPARE(
        runtime.transitionState(), kiriview::ImagePresentationTransitionState::CommittedActive);
    QVERIFY(runtime.renderProjection(kiriview::DisplayedPageRole::Secondary).visible);
}

void TestImagePresentationRuntime::hiddenSecondaryProjectionIsNotVisible()
{
    kiriview::ImagePresentationRuntime runtime(renderContext);

    runtime.setViewportSize(QSizeF(400.0, 300.0));
    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 1200), 1),
        kiriview::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz"))),
        kiriview::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.commitSecondaryPageSlot(pageSlot(QSize(800, 1200), 2));
    runtime.setMode(kiriview::ImagePresentationMode::TwoPageSpread);
    runtime.setSecondaryPageVisible(false);

    const kiriview::ImagePresentationRenderProjection projection
        = runtime.renderProjection(kiriview::DisplayedPageRole::Secondary);

    QVERIFY(!projection.visible);
    QVERIFY(projection.visibleItemRect.isEmpty());
}

void TestImagePresentationRuntime::providerReadyProjectionRequiresProviderUrl()
{
    kiriview::ImagePresentationRuntime runtime(renderContext);
    kiriview::ImagePresentationPageSlotSnapshot slot = pageSlot(QSize(800, 600), 1);
    slot.displaySource.status = kiriview::ImageDisplaySourceStatus::Ready;
    slot.displaySource.originalSize = QSize(800, 600);
    slot.displaySource.rasterSize = QSize(400, 300);

    runtime.commitPrimaryPageSlot(slot,
        kiriview::ImagePresentationScopeKey::directImage(localUrl(QStringLiteral("/images/a.png"))),
        kiriview::ImagePresentationPrimaryChangePolicy::ResetZoom);

    const kiriview::ImageDisplaySourceProjection projection
        = runtime.displaySourceProjection(kiriview::DisplayedPageRole::Primary);

    QVERIFY(projection.visible);
    QVERIFY(projection.providerUrl.isEmpty());
    QCOMPARE(projection.status, kiriview::ImageDisplaySourceStatus::Error);
}

void TestImagePresentationRuntime::singlePageSnapshotDoesNotExposeSecondaryVisibility()
{
    kiriview::ImagePresentationRuntime runtime(renderContext);

    runtime.commitPrimaryPageSlot(pageSlot(QSize(800, 1200), 1),
        kiriview::ImagePresentationScopeKey::openedCollection(
            localUrl(QStringLiteral("/books/book.cbz"))),
        kiriview::ImagePresentationPrimaryChangePolicy::ResetZoom);
    runtime.commitSecondaryPageSlot(pageSlot(QSize(800, 1200), 2));
    runtime.setSecondaryPageVisible(true);

    QCOMPARE(runtime.mode(), kiriview::ImagePresentationMode::SinglePage);
    QVERIFY(!runtime.secondaryPageVisible());
    QVERIFY(!runtime.snapshot().secondaryPageVisible);
}

QTEST_GUILESS_MAIN(TestImagePresentationRuntime)

#include "test_imagepresentationruntime.moc"
