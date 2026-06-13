// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadgeometry.h"

#include <QObject>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTest>

class TestImageSpreadGeometry : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void spreadSizeCombinesPagesWhenBothAreAvailable();
    void scaledPageDisplaySizeUsesSpreadWidthRatio();
    void pageRectsRespectReadingDirectionAndVerticalCentering();
    void widePagePolicyRequiresLandscapeImage();
};

void TestImageSpreadGeometry::spreadSizeCombinesPagesWhenBothAreAvailable()
{
    QCOMPARE(kiriview::imageSpreadImageSize(QSize(800, 1200), QSize(700, 1000)), QSize(1500, 1200));
    QCOMPARE(kiriview::imageSpreadImageSize(QSize(800, 1200), QSize()), QSize(800, 1200));
}

void TestImageSpreadGeometry::scaledPageDisplaySizeUsesSpreadWidthRatio()
{
    QCOMPARE(kiriview::imageSpreadScaledPageDisplaySize(
                 QSize(800, 1200), QSize(1500, 1200), QSizeF(750.0, 600.0)),
        QSizeF(400.0, 600.0));

    const QSizeF invalidSize = kiriview::imageSpreadScaledPageDisplaySize(
        QSize(), QSize(1500, 1200), QSizeF(750.0, 600.0));
    QVERIFY(!invalidSize.isValid());
}

void TestImageSpreadGeometry::pageRectsRespectReadingDirectionAndVerticalCentering()
{
    const QSizeF primarySize(400.0, 600.0);
    const QSizeF secondarySize(350.0, 500.0);
    const QSizeF spreadSize(750.0, 600.0);

    QCOMPARE(kiriview::imageSpreadPrimaryPageRect(primarySize, secondarySize, spreadSize, false),
        QRectF(0.0, 0.0, 400.0, 600.0));
    QCOMPARE(kiriview::imageSpreadSecondaryPageRect(primarySize, secondarySize, spreadSize, false),
        QRectF(400.0, 50.0, 350.0, 500.0));
    QCOMPARE(kiriview::imageSpreadPrimaryPageRect(primarySize, secondarySize, spreadSize, true),
        QRectF(350.0, 0.0, 400.0, 600.0));
    QCOMPARE(kiriview::imageSpreadSecondaryPageRect(primarySize, secondarySize, spreadSize, true),
        QRectF(0.0, 50.0, 350.0, 500.0));
}

void TestImageSpreadGeometry::widePagePolicyRequiresLandscapeImage()
{
    QVERIFY(kiriview::imageSpreadPageIsWide(QSize(1200, 800)));
    QVERIFY(!kiriview::imageSpreadPageIsWide(QSize(800, 800)));
    QVERIFY(!kiriview::imageSpreadPageIsWide(QSize()));
}

QTEST_GUILESS_MAIN(TestImageSpreadGeometry)

#include "test_imagespreadgeometry.moc"
