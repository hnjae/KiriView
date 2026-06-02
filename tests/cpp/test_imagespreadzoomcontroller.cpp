// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadzoomcontroller.h"

#include "image_test_support.h"
#include "presentation/imagepresentationcontroller.h"
#include "rendering/imagerendering.h"

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <limits>

namespace {
using KiriView::TestSupport::staticTestImagePayload;
using KiriView::TestSupport::testImage;

bool hasZoomChange(const KiriView::ImageZoomChangeSet &changes)
{
    return changes.imageSizeChanged || changes.viewportSizeChanged || changes.zoomModeChanged
        || changes.zoomPercentChanged || changes.displaySizeChanged
        || changes.maximumManualZoomPercentChanged || changes.scheduleVisibleTileDecode;
}

KiriView::ImageCacheBudgets testCacheBudgets()
{
    return KiriView::ImageCacheBudgets {
        1024 * 1024,
        512 * 1024,
    };
}

class SpreadZoomFixture
{
public:
    SpreadZoomFixture()
        : primary(
              &context, [this]() { return renderContext(); }, {}, testCacheBudgets())
        , secondary(
              &context, [this]() { return renderContext(); }, {}, testCacheBudgets())
        , controller([this]() { return renderContext(); }, primary, secondary)
    {
        primary.setViewportSize(QSizeF(800.0, 600.0));
        secondary.setViewportSize(QSizeF(800.0, 600.0));
        primary.setStaticImage(staticTestImagePayload(testImage(QSize(800, 1200))), false);
        secondary.setStaticImage(staticTestImagePayload(testImage(QSize(800, 1200))), false);
    }

    KiriView::ImageDocumentRenderContext renderContext() const
    {
        return KiriView::ImageDocumentRenderContext {
            devicePixelRatio,
            KiriView::fallbackTextureSizeMax,
        };
    }

    QObject context;
    qreal devicePixelRatio = 1.0;
    KiriView::ImagePresentationController primary;
    KiriView::ImagePresentationController secondary;
    KiriView::ImageSpreadZoomController controller;
};
}

class TestImageSpreadZoomController : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void updateFromPrimaryOwnsSpreadZoomState();
    void manualZoomReportsOnlyActualSpreadZoomChanges();
    void renderContextChangesReportDerivedZoomChanges();
    void pageVisibleRectsUseExplicitSpreadRect();
    void rightToLeftReadingReprojectsExplicitSpreadRect();
};

void TestImageSpreadZoomController::updateFromPrimaryOwnsSpreadZoomState()
{
    SpreadZoomFixture fixture;

    const KiriView::ImageZoomChangeSet changes = fixture.controller.updateFromPrimaryPresentation();

    QVERIFY(changes.imageSizeChanged);
    QVERIFY(changes.viewportSizeChanged);
    QVERIFY(changes.zoomPercentChanged);
    QVERIFY(changes.displaySizeChanged);
    QVERIFY(changes.maximumManualZoomPercentChanged);
    QVERIFY(changes.scheduleVisibleTileDecode);
    QCOMPARE(fixture.controller.spreadImageSize(), QSize(1600, 1200));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        fixture.controller.displaySize(), QSizeF(800.0, 600.0)));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        fixture.controller.primaryDisplaySize(), QSizeF(400.0, 600.0)));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        fixture.controller.secondaryDisplaySize(), QSizeF(400.0, 600.0)));

    fixture.controller.applyZoomStateToPages(QRectF(), false);
    QCOMPARE(fixture.primary.zoomMode(), KiriView::ImageZoomMode::Fit);
    QCOMPARE(fixture.secondary.zoomMode(), KiriView::ImageZoomMode::Fit);
}

void TestImageSpreadZoomController::manualZoomReportsOnlyActualSpreadZoomChanges()
{
    SpreadZoomFixture fixture;
    fixture.controller.updateFromPrimaryPresentation();
    fixture.controller.applyZoomStateToPages(QRectF(), false);

    const KiriView::ImageZoomChangeSet changes = fixture.controller.setZoomPercent(125.0);

    QVERIFY(!changes.imageSizeChanged);
    QVERIFY(!changes.viewportSizeChanged);
    QVERIFY(changes.zoomModeChanged);
    QVERIFY(changes.zoomPercentChanged);
    QVERIFY(changes.displaySizeChanged);
    QVERIFY(changes.scheduleVisibleTileDecode);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(fixture.controller.zoomPercent(), 125.0));
    fixture.controller.applyZoomStateToPages(QRectF(), false);
    QCOMPARE(fixture.primary.zoomMode(), KiriView::ImageZoomMode::Fit);
    QCOMPARE(fixture.secondary.zoomMode(), KiriView::ImageZoomMode::Fit);

    const KiriView::ImageZoomChangeSet invalidChanges
        = fixture.controller.setZoomPercent(std::numeric_limits<qreal>::quiet_NaN());
    QVERIFY(!hasZoomChange(invalidChanges));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(fixture.controller.zoomPercent(), 125.0));
}

void TestImageSpreadZoomController::renderContextChangesReportDerivedZoomChanges()
{
    SpreadZoomFixture fixture;
    fixture.controller.updateFromPrimaryPresentation();

    fixture.devicePixelRatio = 2.0;
    const KiriView::ImageZoomChangeSet changes = fixture.controller.updateRenderContext();

    QVERIFY(!changes.imageSizeChanged);
    QVERIFY(!changes.viewportSizeChanged);
    QVERIFY(!changes.zoomModeChanged);
    QVERIFY(changes.zoomPercentChanged);
    QVERIFY(!changes.displaySizeChanged);
    QVERIFY(changes.maximumManualZoomPercentChanged);
    QVERIFY(changes.scheduleVisibleTileDecode);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(
        fixture.controller.displaySize(), QSizeF(800.0, 600.0)));
}

void TestImageSpreadZoomController::pageVisibleRectsUseExplicitSpreadRect()
{
    SpreadZoomFixture fixture;
    fixture.controller.updateFromPrimaryPresentation();

    fixture.controller.applyVisibleItemRects(QRectF(0.0, 0.0, 400.0, 600.0), false);
    QCOMPARE(fixture.primary.visibleItemRect(), QRectF(0.0, 0.0, 400.0, 600.0));
    QVERIFY(fixture.secondary.visibleItemRect().isEmpty());

    fixture.controller.applyVisibleItemRects(QRectF(400.0, 0.0, 400.0, 600.0), false);
    QVERIFY(fixture.primary.visibleItemRect().isEmpty());
    QCOMPARE(fixture.secondary.visibleItemRect(), QRectF(0.0, 0.0, 400.0, 600.0));
}

void TestImageSpreadZoomController::rightToLeftReadingReprojectsExplicitSpreadRect()
{
    SpreadZoomFixture fixture;
    fixture.controller.updateFromPrimaryPresentation();

    fixture.controller.applyVisibleItemRects(QRectF(0.0, 0.0, 400.0, 600.0), true);

    QVERIFY(fixture.primary.visibleItemRect().isEmpty());
    QCOMPARE(fixture.secondary.visibleItemRect(), QRectF(0.0, 0.0, 400.0, 600.0));
}

QTEST_GUILESS_MAIN(TestImageSpreadZoomController)

#include "test_imagespreadzoomcontroller.moc"
