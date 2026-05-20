// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagerectmapping.h"

#include <QObject>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>
#include <limits>

class TestImageRectMapping : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void boundedIntegerRectClipsToBounds();
    void scaledIntegerRectMapsHalfOpenBounds();
    void scaledIntegerRectClampsToSourceBounds();
    void scaledIntegerRectRejectsInvalidInputs();
};

void TestImageRectMapping::boundedIntegerRectClipsToBounds()
{
    QCOMPARE(KiriView::boundedIntegerRect(QRect(-1, -2, 4, 5), QSize(10, 10)), QRect(0, 0, 3, 3));
    QCOMPARE(KiriView::boundedIntegerRect(QRect(12, 0, 1, 1), QSize(10, 10)), QRect());
}

void TestImageRectMapping::scaledIntegerRectMapsHalfOpenBounds()
{
    QCOMPARE(
        KiriView::scaledIntegerRect(QRectF(1.0, 2.0, 3.0, 4.0), QSizeF(10.0, 10.0), QSize(100, 50)),
        QRect(10, 10, 30, 20));
}

void TestImageRectMapping::scaledIntegerRectClampsToSourceBounds()
{
    QCOMPARE(KiriView::scaledIntegerRect(
                 QRectF(-5.0, 2.0, 10.0, 5.0), QSizeF(20.0, 10.0), QSize(40, 20)),
        QRect(0, 4, 10, 10));
}

void TestImageRectMapping::scaledIntegerRectRejectsInvalidInputs()
{
    QVERIFY(
        KiriView::scaledIntegerRect(QRectF(std::numeric_limits<qreal>::quiet_NaN(), 0.0, 1.0, 1.0),
            QSizeF(10.0, 10.0), QSize(10, 10))
            .isEmpty());
    QVERIFY(KiriView::scaledIntegerRect(QRectF(0.0, 0.0, 1.0, 1.0),
        QSizeF(std::numeric_limits<qreal>::quiet_NaN(), 10.0), QSize(10, 10))
            .isEmpty());
    QVERIFY(KiriView::scaledIntegerRect(QRectF(0.0, 0.0, 1.0, 1.0), QSizeF(10.0, 10.0), QSize())
            .isEmpty());
    QVERIFY(
        KiriView::scaledIntegerRect(QRectF(20.0, 0.0, 1.0, 1.0), QSizeF(10.0, 10.0), QSize(10, 10))
            .isEmpty());
}

QTEST_GUILESS_MAIN(TestImageRectMapping)

#include "test_imagerectmapping.moc"
