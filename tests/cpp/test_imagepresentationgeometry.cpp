// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationgeometry.h"

#include <QObject>
#include <QSize>
#include <QTest>

class TestImagePresentationGeometry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceAndRotationProduceLogicalImageSize();
    void redundantUpdatesReportNoChange();
};

void TestImagePresentationGeometry::sourceAndRotationProduceLogicalImageSize()
{
    KiriView::ImagePresentationGeometry geometry;

    QVERIFY(geometry.setSourceImageSize(QSize(100, 200)));
    QCOMPARE(geometry.sourceImageSize(), QSize(100, 200));
    QCOMPARE(geometry.logicalImageSize(), QSize(100, 200));
    QCOMPARE(geometry.rotationDegrees(), 0);

    QVERIFY(geometry.rotateClockwise());
    QCOMPARE(geometry.logicalImageSize(), QSize(200, 100));
    QCOMPARE(geometry.rotationDegrees(), 90);

    QVERIFY(geometry.rotateCounterclockwise());
    QCOMPARE(geometry.logicalImageSize(), QSize(100, 200));
    QCOMPARE(geometry.rotationDegrees(), 0);

    QVERIFY(geometry.setRotationDegrees(-90));
    QCOMPARE(geometry.logicalImageSize(), QSize(200, 100));
    QCOMPARE(geometry.rotationDegrees(), 270);

    QVERIFY(geometry.resetRotation());
    QCOMPARE(geometry.logicalImageSize(), QSize(100, 200));
    QCOMPARE(geometry.rotationDegrees(), 0);
}

void TestImagePresentationGeometry::redundantUpdatesReportNoChange()
{
    KiriView::ImagePresentationGeometry geometry;

    QVERIFY(!geometry.resetRotation());
    QVERIFY(geometry.setSourceImageSize(QSize(20, 10)));
    QVERIFY(!geometry.setSourceImageSize(QSize(20, 10)));
    QVERIFY(geometry.setRotationDegrees(450));
    QCOMPARE(geometry.rotationDegrees(), 90);
    QVERIFY(!geometry.setRotationDegrees(90));
}

QTEST_GUILESS_MAIN(TestImagePresentationGeometry)

#include "test_imagepresentationgeometry.moc"
