// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imageviewportframe.h"
#include "presentation/imageviewportgeometry.h"

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QString>
#include <QTest>
#include <cmath>

namespace {
bool approximatelyEqual(qreal left, qreal right) { return std::abs(left - right) < 0.001; }

void comparePoint(const QPointF &actual, const QPointF &expected)
{
    QVERIFY2(approximatelyEqual(actual.x(), expected.x()),
        qPrintable(QStringLiteral("x=%1").arg(actual.x())));
    QVERIFY2(approximatelyEqual(actual.y(), expected.y()),
        qPrintable(QStringLiteral("y=%1").arg(actual.y())));
}
}

class TestImageViewportGeometry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void anchoredZoomKeepsViewportPointOnSameImageRatio();
    void panPositionClampsToContentBounds();
    void zScanMovesForwardByThreeQuartersOfViewport();
    void zScanMovesBackwardThroughSamePositions();
    void zScanMovesRightToLeftWhenReadingDirectionIsRightToLeft();
    void zScanStopsAtBoundaries();
    void zScanStartAndEndUseReadingDirection();
    void zScanHandlesSingleAxisPanning();
    void smallImagesAreCenteredAndNotPannable();
    void viewportFrameTreatsFitToleranceAsNotPannable();
    void subEpsilonAxesUseViewportFrameClampRules();
    void viewportFrameClampsContentPosition();
};

void TestImageViewportGeometry::anchoredZoomKeepsViewportPointOnSameImageRatio()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QPointF contentPosition(50.0, 50.0);
    const QPointF anchor(25.0, 25.0);

    const QPointF zoomedPosition = KiriView::imageViewportContentPositionForZoom(
        viewportSize, QSizeF(200.0, 200.0), QSizeF(400.0, 400.0), contentPosition, anchor);

    comparePoint(zoomedPosition, QPointF(125.0, 125.0));
}

void TestImageViewportGeometry::panPositionClampsToContentBounds()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 300.0, 80.0);

    const QPointF clampedToMaximum = KiriView::imageViewportPanPosition(
        viewportSize, imageRect, QPointF(180.0, 0.0), QPointF(80.0, 20.0));
    comparePoint(clampedToMaximum, QPointF(200.0, 0.0));

    const QPointF clampedToMinimum = KiriView::imageViewportPanPosition(
        viewportSize, imageRect, QPointF(10.0, 0.0), QPointF(-50.0, 0.0));
    comparePoint(clampedToMinimum, QPointF(0.0, 0.0));
}

void TestImageViewportGeometry::zScanMovesForwardByThreeQuartersOfViewport()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 400.0, 300.0);

    comparePoint(
        KiriView::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(0.0, 0.0)),
        QPointF(75.0, 0.0));
    comparePoint(
        KiriView::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(225.0, 0.0)),
        QPointF(300.0, 0.0));
    comparePoint(
        KiriView::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(300.0, 0.0)),
        QPointF(0.0, 75.0));
}

void TestImageViewportGeometry::zScanMovesBackwardThroughSamePositions()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 400.0, 300.0);

    comparePoint(
        KiriView::imageViewportPreviousZScanPosition(viewportSize, imageRect, QPointF(0.0, 75.0)),
        QPointF(300.0, 0.0));
    comparePoint(
        KiriView::imageViewportPreviousZScanPosition(viewportSize, imageRect, QPointF(300.0, 0.0)),
        QPointF(225.0, 0.0));
}

void TestImageViewportGeometry::zScanMovesRightToLeftWhenReadingDirectionIsRightToLeft()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 400.0, 300.0);

    comparePoint(KiriView::imageViewportNextZScanPosition(
                     viewportSize, imageRect, QPointF(300.0, 0.0), true),
        QPointF(225.0, 0.0));
    comparePoint(
        KiriView::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(0.0, 0.0), true),
        QPointF(300.0, 75.0));
    comparePoint(KiriView::imageViewportPreviousZScanPosition(
                     viewportSize, imageRect, QPointF(300.0, 75.0), true),
        QPointF(0.0, 0.0));
    comparePoint(KiriView::imageViewportPreviousZScanPosition(
                     viewportSize, imageRect, QPointF(225.0, 0.0), true),
        QPointF(300.0, 0.0));
}

void TestImageViewportGeometry::zScanStopsAtBoundaries()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 150.0, 150.0);

    comparePoint(
        KiriView::imageViewportPreviousZScanPosition(viewportSize, imageRect, QPointF(0.0, 0.0)),
        QPointF(0.0, 0.0));
    comparePoint(
        KiriView::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(50.0, 50.0)),
        QPointF(50.0, 50.0));
    comparePoint(
        KiriView::imageViewportFinalZScanPosition(viewportSize, imageRect), QPointF(50.0, 50.0));
}

void TestImageViewportGeometry::zScanStartAndEndUseReadingDirection()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 400.0, 300.0);

    comparePoint(
        KiriView::imageViewportInitialZScanPosition(viewportSize, imageRect), QPointF(0.0, 0.0));
    comparePoint(
        KiriView::imageViewportFinalZScanPosition(viewportSize, imageRect), QPointF(300.0, 200.0));
    comparePoint(KiriView::imageViewportInitialZScanPosition(viewportSize, imageRect, true),
        QPointF(300.0, 0.0));
    comparePoint(KiriView::imageViewportFinalZScanPosition(viewportSize, imageRect, true),
        QPointF(0.0, 200.0));
}

void TestImageViewportGeometry::zScanHandlesSingleAxisPanning()
{
    const QSizeF viewportSize(100.0, 100.0);

    const QRectF tallImageRect(10.0, 0.0, 80.0, 300.0);
    comparePoint(
        KiriView::imageViewportNextZScanPosition(viewportSize, tallImageRect, QPointF(0.0, 0.0)),
        QPointF(0.0, 75.0));
    const QPointF previousTallScanPosition = KiriView::imageViewportPreviousZScanPosition(
        viewportSize, tallImageRect, QPointF(0.0, 75.0));
    comparePoint(previousTallScanPosition, QPointF(0.0, 0.0));
    comparePoint(KiriView::imageViewportFinalZScanPosition(viewportSize, tallImageRect),
        QPointF(0.0, 200.0));

    const QRectF wideImageRect(0.0, 10.0, 300.0, 80.0);
    comparePoint(
        KiriView::imageViewportNextZScanPosition(viewportSize, wideImageRect, QPointF(150.0, 0.0)),
        QPointF(200.0, 0.0));
    const QPointF previousWideScanPosition = KiriView::imageViewportPreviousZScanPosition(
        viewportSize, wideImageRect, QPointF(200.0, 0.0));
    comparePoint(previousWideScanPosition, QPointF(150.0, 0.0));
    comparePoint(KiriView::imageViewportFinalZScanPosition(viewportSize, wideImageRect),
        QPointF(200.0, 0.0));
}

void TestImageViewportGeometry::smallImagesAreCenteredAndNotPannable()
{
    const QSizeF viewportSize(200.0, 200.0);
    const QRectF imageRect = KiriView::imageViewportImageRect(viewportSize, QSizeF(50.0, 50.0));

    QCOMPARE(imageRect, QRectF(75.0, 75.0, 50.0, 50.0));
    comparePoint(
        KiriView::imageViewportMaximumContentPosition(viewportSize, imageRect), QPointF(0.0, 0.0));
    QVERIFY(KiriView::imageViewportPointInsideImage(
        QPointF(0.0, 0.0), QPointF(100.0, 100.0), imageRect));
    QVERIFY(!KiriView::imageViewportPointInsideImage(
        QPointF(0.0, 0.0), QPointF(20.0, 20.0), imageRect));
}

void TestImageViewportGeometry::viewportFrameTreatsFitToleranceAsNotPannable()
{
    const KiriView::ImageViewportFrame frame = KiriView::projectImageViewportFrame(
        QSizeF(200.0, 160.0), QSizeF(200.0005, 160.0005), QPointF(10.0, 10.0));

    QVERIFY(!frame.horizontalPannable);
    QVERIFY(!frame.verticalPannable);
    QVERIFY(!frame.pannable);
    QCOMPARE(frame.contentSize, QSizeF(200.0, 160.0));
    comparePoint(frame.maximumContentPosition, QPointF(0.0, 0.0));
    comparePoint(frame.contentPosition, QPointF(0.0, 0.0));
}

void TestImageViewportGeometry::subEpsilonAxesUseViewportFrameClampRules()
{
    const QSizeF viewportSize(200.0, 160.0);
    const QSizeF displaySize(200.0005, 160.0005);
    const QRectF imageRect = KiriView::imageViewportImageRect(viewportSize, displaySize);
    const KiriView::ImageViewportFrame frame
        = KiriView::projectImageViewportFrame(viewportSize, displaySize, QPointF(10.0, 10.0));

    comparePoint(KiriView::imageViewportPanPosition(
                     viewportSize, imageRect, QPointF(0.0, 0.0), QPointF(10.0, 10.0)),
        frame.contentPosition);
    comparePoint(KiriView::imageViewportFinalZScanPosition(viewportSize, imageRect, true),
        frame.contentPosition);
    comparePoint(KiriView::imageViewportContentPositionForZoom(viewportSize, viewportSize,
                     displaySize, QPointF(10.0, 10.0), QPointF(100.0, 80.0)),
        frame.contentPosition);
}

void TestImageViewportGeometry::viewportFrameClampsContentPosition()
{
    const KiriView::ImageViewportFrame frame = KiriView::projectImageViewportFrame(
        QSizeF(200.0, 160.0), QSizeF(400.0, 240.0), QPointF(500.0, 500.0));

    QVERIFY(frame.horizontalPannable);
    QVERIFY(frame.verticalPannable);
    QVERIFY(frame.pannable);
    QCOMPARE(frame.contentSize, QSizeF(400.0, 240.0));
    comparePoint(frame.maximumContentPosition, QPointF(200.0, 80.0));
    comparePoint(frame.contentPosition, QPointF(200.0, 80.0));
    QCOMPARE(frame.visibleItemRect, QRectF(200.0, 80.0, 200.0, 160.0));
}

QTEST_GUILESS_MAIN(TestImageViewportGeometry)

#include "test_imageviewportgeometry.moc"
