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

void comparePoint(const QPointF& actual, const QPointF& expected)
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
    void zScanMovesForwardBySevenEighthsOfViewport();
    void zScanMovesBackwardThroughSamePositions();
    void zScanMovesRightToLeftWhenReadingDirectionIsRightToLeft();
    void zScanStopsAtBoundaries();
    void zScanStartAndEndUseReadingDirection();
    void zScanHandlesSingleAxisPanning();
    void smallImagesAreCenteredAndNotPannable();
    void nearestImagePointClampsViewportPointToImage();
    void nearestImagePointHandlesPannedContent();
    void nearestImagePointRejectsEmptyImageRect();
    void viewportFrameTreatsFitToleranceAsNotPannable();
    void subEpsilonAxesUseViewportFrameClampRules();
    void viewportFrameClampsContentPosition();
};

void TestImageViewportGeometry::anchoredZoomKeepsViewportPointOnSameImageRatio()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QPointF contentPosition(50.0, 50.0);
    const QPointF anchor(25.0, 25.0);

    const QPointF zoomedPosition = kiriview::imageViewportContentPositionForZoom(
        viewportSize, QSizeF(200.0, 200.0), QSizeF(400.0, 400.0), contentPosition, anchor);

    comparePoint(zoomedPosition, QPointF(125.0, 125.0));
}

void TestImageViewportGeometry::panPositionClampsToContentBounds()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 300.0, 80.0);

    const QPointF clampedToMaximum = kiriview::imageViewportPanPosition(
        viewportSize, imageRect, QPointF(180.0, 0.0), QPointF(80.0, 20.0));
    comparePoint(clampedToMaximum, QPointF(200.0, 0.0));

    const QPointF clampedToMinimum = kiriview::imageViewportPanPosition(
        viewportSize, imageRect, QPointF(10.0, 0.0), QPointF(-50.0, 0.0));
    comparePoint(clampedToMinimum, QPointF(0.0, 0.0));
}

void TestImageViewportGeometry::zScanMovesForwardBySevenEighthsOfViewport()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 400.0, 300.0);

    comparePoint(
        kiriview::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(0.0, 0.0)),
        QPointF(87.5, 0.0));
    comparePoint(
        kiriview::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(225.0, 0.0)),
        QPointF(262.5, 0.0));
    comparePoint(
        kiriview::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(300.0, 0.0)),
        QPointF(0.0, 87.5));
}

void TestImageViewportGeometry::zScanMovesBackwardThroughSamePositions()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 400.0, 300.0);

    comparePoint(
        kiriview::imageViewportPreviousZScanPosition(viewportSize, imageRect, QPointF(0.0, 75.0)),
        QPointF(300.0, 0.0));
    comparePoint(
        kiriview::imageViewportPreviousZScanPosition(viewportSize, imageRect, QPointF(300.0, 0.0)),
        QPointF(262.5, 0.0));
}

void TestImageViewportGeometry::zScanMovesRightToLeftWhenReadingDirectionIsRightToLeft()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 400.0, 300.0);

    comparePoint(kiriview::imageViewportNextZScanPosition(
                     viewportSize, imageRect, QPointF(300.0, 0.0), true),
        QPointF(262.5, 0.0));
    comparePoint(
        kiriview::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(0.0, 0.0), true),
        QPointF(300.0, 87.5));
    comparePoint(kiriview::imageViewportPreviousZScanPosition(
                     viewportSize, imageRect, QPointF(300.0, 87.5), true),
        QPointF(0.0, 0.0));
    comparePoint(kiriview::imageViewportPreviousZScanPosition(
                     viewportSize, imageRect, QPointF(262.5, 0.0), true),
        QPointF(300.0, 0.0));
}

void TestImageViewportGeometry::zScanStopsAtBoundaries()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 150.0, 150.0);

    comparePoint(
        kiriview::imageViewportPreviousZScanPosition(viewportSize, imageRect, QPointF(0.0, 0.0)),
        QPointF(0.0, 0.0));
    comparePoint(
        kiriview::imageViewportNextZScanPosition(viewportSize, imageRect, QPointF(50.0, 50.0)),
        QPointF(50.0, 50.0));
    comparePoint(
        kiriview::imageViewportFinalZScanPosition(viewportSize, imageRect), QPointF(50.0, 50.0));
}

void TestImageViewportGeometry::zScanStartAndEndUseReadingDirection()
{
    const QSizeF viewportSize(100.0, 100.0);
    const QRectF imageRect(0.0, 0.0, 400.0, 300.0);

    comparePoint(
        kiriview::imageViewportInitialZScanPosition(viewportSize, imageRect), QPointF(0.0, 0.0));
    comparePoint(
        kiriview::imageViewportFinalZScanPosition(viewportSize, imageRect), QPointF(300.0, 200.0));
    comparePoint(kiriview::imageViewportInitialZScanPosition(viewportSize, imageRect, true),
        QPointF(300.0, 0.0));
    comparePoint(kiriview::imageViewportFinalZScanPosition(viewportSize, imageRect, true),
        QPointF(0.0, 200.0));
}

void TestImageViewportGeometry::zScanHandlesSingleAxisPanning()
{
    const QSizeF viewportSize(100.0, 100.0);

    const QRectF tallImageRect(10.0, 0.0, 80.0, 300.0);
    comparePoint(
        kiriview::imageViewportNextZScanPosition(viewportSize, tallImageRect, QPointF(0.0, 0.0)),
        QPointF(0.0, 87.5));
    const QPointF previousTallScanPosition = kiriview::imageViewportPreviousZScanPosition(
        viewportSize, tallImageRect, QPointF(0.0, 87.5));
    comparePoint(previousTallScanPosition, QPointF(0.0, 0.0));
    comparePoint(kiriview::imageViewportFinalZScanPosition(viewportSize, tallImageRect),
        QPointF(0.0, 200.0));

    const QRectF wideImageRect(0.0, 10.0, 300.0, 80.0);
    comparePoint(
        kiriview::imageViewportNextZScanPosition(viewportSize, wideImageRect, QPointF(150.0, 0.0)),
        QPointF(175.0, 0.0));
    const QPointF previousWideScanPosition = kiriview::imageViewportPreviousZScanPosition(
        viewportSize, wideImageRect, QPointF(200.0, 0.0));
    comparePoint(previousWideScanPosition, QPointF(175.0, 0.0));
    comparePoint(kiriview::imageViewportFinalZScanPosition(viewportSize, wideImageRect),
        QPointF(200.0, 0.0));
}

void TestImageViewportGeometry::smallImagesAreCenteredAndNotPannable()
{
    const QSizeF viewportSize(200.0, 200.0);
    const QRectF imageRect = kiriview::imageViewportImageRect(viewportSize, QSizeF(50.0, 50.0));

    QCOMPARE(imageRect, QRectF(75.0, 75.0, 50.0, 50.0));
    comparePoint(
        kiriview::imageViewportMaximumContentPosition(viewportSize, imageRect), QPointF(0.0, 0.0));
    QVERIFY(kiriview::imageViewportPointInsideImage(
        QPointF(0.0, 0.0), QPointF(100.0, 100.0), imageRect));
    QVERIFY(!kiriview::imageViewportPointInsideImage(
        QPointF(0.0, 0.0), QPointF(20.0, 20.0), imageRect));
}

void TestImageViewportGeometry::nearestImagePointClampsViewportPointToImage()
{
    const QSizeF viewportSize(200.0, 200.0);
    const QRectF imageRect = kiriview::imageViewportImageRect(viewportSize, QSizeF(50.0, 50.0));

    comparePoint(
        kiriview::imageViewportNearestImagePoint(QPointF(0.0, 0.0), QPointF(20.0, 20.0), imageRect),
        QPointF(75.0, 75.0));
    comparePoint(kiriview::imageViewportNearestImagePoint(
                     QPointF(0.0, 0.0), QPointF(180.0, 100.0), imageRect),
        QPointF(125.0, 100.0));
    comparePoint(kiriview::imageViewportNearestImagePoint(
                     QPointF(0.0, 0.0), QPointF(100.0, 140.0), imageRect),
        QPointF(100.0, 125.0));
}

void TestImageViewportGeometry::nearestImagePointHandlesPannedContent()
{
    const QRectF imageRect(50.0, 30.0, 100.0, 80.0);

    comparePoint(kiriview::imageViewportNearestImagePoint(
                     QPointF(40.0, 20.0), QPointF(200.0, 0.0), imageRect),
        QPointF(110.0, 10.0));
}

void TestImageViewportGeometry::nearestImagePointRejectsEmptyImageRect()
{
    const QPointF point = kiriview::imageViewportNearestImagePoint(
        QPointF(0.0, 0.0), QPointF(20.0, 20.0), QRectF(0.0, 0.0, 0.0, 50.0));

    QVERIFY(std::isnan(point.x()));
    QVERIFY(std::isnan(point.y()));
}

void TestImageViewportGeometry::viewportFrameTreatsFitToleranceAsNotPannable()
{
    const kiriview::ImageViewportFrame frame = kiriview::projectImageViewportFrame(
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
    const QRectF imageRect = kiriview::imageViewportImageRect(viewportSize, displaySize);
    const kiriview::ImageViewportFrame frame
        = kiriview::projectImageViewportFrame(viewportSize, displaySize, QPointF(10.0, 10.0));

    comparePoint(kiriview::imageViewportPanPosition(
                     viewportSize, imageRect, QPointF(0.0, 0.0), QPointF(10.0, 10.0)),
        frame.contentPosition);
    comparePoint(kiriview::imageViewportFinalZScanPosition(viewportSize, imageRect, true),
        frame.contentPosition);
    comparePoint(kiriview::imageViewportContentPositionForZoom(viewportSize, viewportSize,
                     displaySize, QPointF(10.0, 10.0), QPointF(100.0, 80.0)),
        frame.contentPosition);
}

void TestImageViewportGeometry::viewportFrameClampsContentPosition()
{
    const kiriview::ImageViewportFrame frame = kiriview::projectImageViewportFrame(
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
