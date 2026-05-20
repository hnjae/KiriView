// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadzoomcontroller.h"

#include "image_test_support.h"
#include "imagepresentationcontroller.h"
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

class SpreadZoomFixture
{
public:
    SpreadZoomFixture()
        : primary(&context, [this]() { return renderContext(); }, {})
        , secondary(&context, [this]() { return renderContext(); }, {})
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
};

void TestImageSpreadZoomController::updateFromPrimaryOwnsSpreadZoomState()
{
    SpreadZoomFixture fixture;

    const KiriView::ImageZoomChangeSet changes
        = fixture.controller.updateFromPrimaryPresentation(false);

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
    QCOMPARE(fixture.primary.zoomMode(), KiriView::ImageZoomMode::Manual);
    QCOMPARE(fixture.secondary.zoomMode(), KiriView::ImageZoomMode::Manual);
}

void TestImageSpreadZoomController::manualZoomReportsOnlyActualSpreadZoomChanges()
{
    SpreadZoomFixture fixture;
    fixture.controller.updateFromPrimaryPresentation(false);

    const KiriView::ImageZoomChangeSet changes = fixture.controller.setZoomPercent(125.0, false);

    QVERIFY(!changes.imageSizeChanged);
    QVERIFY(!changes.viewportSizeChanged);
    QVERIFY(changes.zoomModeChanged);
    QVERIFY(changes.zoomPercentChanged);
    QVERIFY(changes.displaySizeChanged);
    QVERIFY(changes.scheduleVisibleTileDecode);
    QVERIFY(KiriView::imageZoomApproximatelyEqual(fixture.controller.zoomPercent(), 125.0));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(fixture.primary.zoomPercent(), 125.0));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(fixture.secondary.zoomPercent(), 125.0));

    const KiriView::ImageZoomChangeSet invalidChanges
        = fixture.controller.setZoomPercent(std::numeric_limits<qreal>::quiet_NaN(), false);
    QVERIFY(!hasZoomChange(invalidChanges));
    QVERIFY(KiriView::imageZoomApproximatelyEqual(fixture.controller.zoomPercent(), 125.0));
}

void TestImageSpreadZoomController::renderContextChangesReportDerivedZoomChanges()
{
    SpreadZoomFixture fixture;
    fixture.controller.updateFromPrimaryPresentation(false);

    fixture.devicePixelRatio = 2.0;
    const KiriView::ImageZoomChangeSet changes = fixture.controller.updateRenderContext(false);

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

QTEST_GUILESS_MAIN(TestImageSpreadZoomController)

#include "test_imagespreadzoomcontroller.moc"
