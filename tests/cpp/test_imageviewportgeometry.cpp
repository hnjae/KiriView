// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageviewportgeometry.h"

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
    void smallImagesAreCenteredAndNotPannable();
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

QTEST_GUILESS_MAIN(TestImageViewportGeometry)

#include "test_imageviewportgeometry.moc"
