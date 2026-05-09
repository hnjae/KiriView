// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadgeometry.h"

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
    void previousPageTargetUsesSpreadPolicy();
    void currentLastPageNumberTracksSecondaryVisibility();
    void relativePageTargetRejectsOutOfRangePages();
    void nextPageTargetStopsAtEnd();
    void transitionPolicyRequiresActiveValidDifferentTarget();
    void secondaryPageDecisionSelectsPrimaryKeepOrLoad();
};

void TestImageSpreadGeometry::spreadSizeCombinesPagesWhenBothAreAvailable()
{
    QCOMPARE(KiriView::imageSpreadImageSize(QSize(800, 1200), QSize(700, 1000)), QSize(1500, 1200));
    QCOMPARE(KiriView::imageSpreadImageSize(QSize(800, 1200), QSize()), QSize(800, 1200));
}

void TestImageSpreadGeometry::scaledPageDisplaySizeUsesSpreadWidthRatio()
{
    QCOMPARE(KiriView::imageSpreadScaledPageDisplaySize(
                 QSize(800, 1200), QSize(1500, 1200), QSizeF(750.0, 600.0)),
        QSizeF(400.0, 600.0));

    const QSizeF invalidSize = KiriView::imageSpreadScaledPageDisplaySize(
        QSize(), QSize(1500, 1200), QSizeF(750.0, 600.0));
    QVERIFY(!invalidSize.isValid());
}

void TestImageSpreadGeometry::pageRectsRespectReadingDirectionAndVerticalCentering()
{
    const QSizeF primarySize(400.0, 600.0);
    const QSizeF secondarySize(350.0, 500.0);
    const QSizeF spreadSize(750.0, 600.0);

    QCOMPARE(KiriView::imageSpreadPrimaryPageRect(primarySize, secondarySize, spreadSize, false),
        QRectF(0.0, 0.0, 400.0, 600.0));
    QCOMPARE(KiriView::imageSpreadSecondaryPageRect(primarySize, secondarySize, spreadSize, false),
        QRectF(400.0, 50.0, 350.0, 500.0));
    QCOMPARE(KiriView::imageSpreadPrimaryPageRect(primarySize, secondarySize, spreadSize, true),
        QRectF(350.0, 0.0, 400.0, 600.0));
    QCOMPARE(KiriView::imageSpreadSecondaryPageRect(primarySize, secondarySize, spreadSize, true),
        QRectF(0.0, 50.0, 350.0, 500.0));
}

void TestImageSpreadGeometry::widePagePolicyRequiresLandscapeImage()
{
    QVERIFY(KiriView::imageSpreadPageIsWide(QSize(1200, 800)));
    QVERIFY(!KiriView::imageSpreadPageIsWide(QSize(800, 800)));
    QVERIFY(!KiriView::imageSpreadPageIsWide(QSize()));
}

void TestImageSpreadGeometry::previousPageTargetUsesSpreadPolicy()
{
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(0, false, false), 0);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(1, false, false), 1);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(2, false, false), 1);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(5, true, false), 3);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(5, true, true), 4);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(5, false, false), 4);
}

void TestImageSpreadGeometry::currentLastPageNumberTracksSecondaryVisibility()
{
    QCOMPARE(KiriView::imageSpreadCurrentLastPageNumber(0, false), 0);
    QCOMPARE(KiriView::imageSpreadCurrentLastPageNumber(2, false), 2);
    QCOMPARE(KiriView::imageSpreadCurrentLastPageNumber(2, true), 3);
}

void TestImageSpreadGeometry::relativePageTargetRejectsOutOfRangePages()
{
    QCOMPARE(KiriView::imageSpreadRelativePageTarget(3, 5, -1), 2);
    QCOMPARE(KiriView::imageSpreadRelativePageTarget(3, 5, 1), 4);
    QCOMPARE(KiriView::imageSpreadRelativePageTarget(1, 5, -1), 0);
    QCOMPARE(KiriView::imageSpreadRelativePageTarget(5, 5, 1), 0);
}

void TestImageSpreadGeometry::nextPageTargetStopsAtEnd()
{
    QCOMPARE(KiriView::imageSpreadNextPageTarget(2, 5), 3);
    QCOMPARE(KiriView::imageSpreadNextPageTarget(5, 5), 0);
}

void TestImageSpreadGeometry::transitionPolicyRequiresActiveValidDifferentTarget()
{
    QVERIFY(KiriView::imageSpreadShouldBeginTransition(true, 2, 4, 6));
    QVERIFY(!KiriView::imageSpreadShouldBeginTransition(false, 2, 4, 6));
    QVERIFY(!KiriView::imageSpreadShouldBeginTransition(true, 2, 2, 6));
    QVERIFY(!KiriView::imageSpreadShouldBeginTransition(true, 2, 7, 6));
}

void TestImageSpreadGeometry::secondaryPageDecisionSelectsPrimaryKeepOrLoad()
{
    using KiriView::ImageSpreadSecondaryPageDecision;

    QCOMPARE(KiriView::imageSpreadSecondaryPageDecision(true, 1, 4, false, true, false, false),
        ImageSpreadSecondaryPageDecision::PrimaryOnly);
    QCOMPARE(KiriView::imageSpreadSecondaryPageDecision(true, 2, 4, true, true, false, false),
        ImageSpreadSecondaryPageDecision::PrimaryOnly);
    QCOMPARE(KiriView::imageSpreadSecondaryPageDecision(true, 2, 4, false, true, true, false),
        ImageSpreadSecondaryPageDecision::PrimaryOnly);
    QCOMPARE(KiriView::imageSpreadSecondaryPageDecision(true, 2, 4, false, true, false, true),
        ImageSpreadSecondaryPageDecision::KeepCurrentSecondary);
    QCOMPARE(KiriView::imageSpreadSecondaryPageDecision(true, 2, 4, false, true, false, false),
        ImageSpreadSecondaryPageDecision::LoadNext);
}

QTEST_GUILESS_MAIN(TestImageSpreadGeometry)

#include "test_imagespreadgeometry.moc"
