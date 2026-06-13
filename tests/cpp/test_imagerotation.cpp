// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/imagerotation.h"

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>

class TestImageRotation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rotationDegreesNormalizeToQuarterTurns();
    void rotatedSizesSwapAxesForSidewaysRotation();
    void rotatedSourceRectsMapIntoTargetSpace();
    void unrotatedVisibleRectsMapBackToSourceOrientation();
    void textureCoordinateTransformsRotateAxes();
};

void TestImageRotation::rotationDegreesNormalizeToQuarterTurns()
{
    QCOMPARE(kiriview::normalizedImageRotationDegrees(0), 0);
    QCOMPARE(kiriview::normalizedImageRotationDegrees(450), 90);
    QCOMPARE(kiriview::normalizedImageRotationDegrees(-90), 270);
    QCOMPARE(kiriview::normalizedImageRotationDegrees(45), 0);
    QCOMPARE(kiriview::imageRotationClockwise(270), 0);
    QCOMPARE(kiriview::imageRotationCounterclockwise(0), 270);
}

void TestImageRotation::rotatedSizesSwapAxesForSidewaysRotation()
{
    QVERIFY(kiriview::imageRotationSwapsAxes(90));
    QVERIFY(kiriview::imageRotationSwapsAxes(270));
    QVERIFY(!kiriview::imageRotationSwapsAxes(180));
    QCOMPARE(kiriview::rotatedImageSize(QSize(640, 480), 90), QSize(480, 640));
    QCOMPARE(kiriview::rotatedImageSize(QSizeF(640.0, 480.0), 180), QSizeF(640.0, 480.0));
}

void TestImageRotation::rotatedSourceRectsMapIntoTargetSpace()
{
    const QRectF sourceRect(250.0, 100.0, 500.0, 200.0);
    const QSizeF sourceSize(1000.0, 500.0);

    QCOMPARE(kiriview::rotatedSourceRectInTarget(
                 sourceRect, sourceSize, QRectF(10.0, 20.0, 500.0, 1000.0), 90),
        QRectF(210.0, 270.0, 200.0, 500.0));
    QCOMPARE(kiriview::rotatedSourceRectInTarget(
                 sourceRect, sourceSize, QRectF(10.0, 20.0, 1000.0, 500.0), 180),
        QRectF(260.0, 220.0, 500.0, 200.0));
    QCOMPARE(kiriview::rotatedSourceRectInTarget(
                 sourceRect, sourceSize, QRectF(10.0, 20.0, 500.0, 1000.0), 270),
        QRectF(110.0, 270.0, 200.0, 500.0));
}

void TestImageRotation::unrotatedVisibleRectsMapBackToSourceOrientation()
{
    QCOMPARE(kiriview::unrotatedVisibleRectForRotation(
                 QSizeF(1000.0, 500.0), QRectF(10.0, 20.0, 100.0, 200.0), 90),
        QRectF(20.0, 390.0, 200.0, 100.0));
    QCOMPARE(kiriview::unrotatedVisibleRectForRotation(
                 QSizeF(1000.0, 500.0), QRectF(10.0, 20.0, 100.0, 200.0), 270),
        QRectF(780.0, 10.0, 200.0, 100.0));
}

void TestImageRotation::textureCoordinateTransformsRotateAxes()
{
    const kiriview::ImageTextureCoordinateTransform clockwise
        = kiriview::imageTextureCoordinateTransform(QRectF(0.25, 0.5, 0.5, 0.25), 90);
    QCOMPARE(clockwise.origin, QPointF(0.25, 0.75));
    QCOMPARE(clockwise.xAxis, QPointF(0.0, -0.25));
    QCOMPARE(clockwise.yAxis, QPointF(0.5, 0.0));

    const kiriview::ImageTextureCoordinateTransform counterclockwise
        = kiriview::imageTextureCoordinateTransform(QRectF(0.25, 0.5, 0.5, 0.25), 270);
    QCOMPARE(counterclockwise.origin, QPointF(0.75, 0.5));
    QCOMPARE(counterclockwise.xAxis, QPointF(0.0, 0.25));
    QCOMPARE(counterclockwise.yAxis, QPointF(-0.5, 0.0));
}

QTEST_GUILESS_MAIN(TestImageRotation)

#include "test_imagerotation.moc"
