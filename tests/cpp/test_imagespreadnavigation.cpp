// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadnavigation.h"

#include <QTest>

class TestImageSpreadNavigation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void previousPageTargetUsesSpreadPolicy();
    void currentLastPageNumberTracksSecondaryVisibility();
    void relativePageTargetRejectsOutOfRangePages();
    void nextPageTargetStopsAtEnd();
    void transitionPolicyRequiresActiveValidDifferentTarget();
    void adjacentNavigationFallsBackWhenSpreadIsInactive();
    void adjacentNavigationUsesVisibleSpreadEdges();
    void previousNavigationAccountsForWidePreviousPage();
    void relativeNavigationAndTransitionsUseSpreadState();
};

void TestImageSpreadNavigation::previousPageTargetUsesSpreadPolicy()
{
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(0, false, false), 0);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(1, false, false), 1);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(2, false, false), 1);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(5, true, false), 3);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(5, true, true), 4);
    QCOMPARE(KiriView::imageSpreadPreviousPageTarget(5, false, false), 4);
}

void TestImageSpreadNavigation::currentLastPageNumberTracksSecondaryVisibility()
{
    QCOMPARE(KiriView::imageSpreadCurrentLastPageNumber(0, false), 0);
    QCOMPARE(KiriView::imageSpreadCurrentLastPageNumber(2, false), 2);
    QCOMPARE(KiriView::imageSpreadCurrentLastPageNumber(2, true), 3);
}

void TestImageSpreadNavigation::relativePageTargetRejectsOutOfRangePages()
{
    QCOMPARE(KiriView::imageSpreadRelativePageTarget(3, 5, -1), 2);
    QCOMPARE(KiriView::imageSpreadRelativePageTarget(3, 5, 1), 4);
    QCOMPARE(KiriView::imageSpreadRelativePageTarget(1, 5, -1), 0);
    QCOMPARE(KiriView::imageSpreadRelativePageTarget(5, 5, 1), 0);
}

void TestImageSpreadNavigation::nextPageTargetStopsAtEnd()
{
    QCOMPARE(KiriView::imageSpreadNextPageTarget(2, 5), 3);
    QCOMPARE(KiriView::imageSpreadNextPageTarget(5, 5), 0);
}

void TestImageSpreadNavigation::transitionPolicyRequiresActiveValidDifferentTarget()
{
    QVERIFY(KiriView::imageSpreadShouldBeginTransition(true, 2, 4, 6));
    QVERIFY(!KiriView::imageSpreadShouldBeginTransition(false, 2, 4, 6));
    QVERIFY(!KiriView::imageSpreadShouldBeginTransition(true, 2, 2, 6));
    QVERIFY(!KiriView::imageSpreadShouldBeginTransition(true, 2, 7, 6));
}

void TestImageSpreadNavigation::adjacentNavigationFallsBackWhenSpreadIsInactive()
{
    const KiriView::ImageSpreadNavigationState state { false, 3, 6, true, false };

    const KiriView::ImageSpreadPageNavigationTarget target
        = KiriView::imageSpreadPageNavigationTarget(KiriView::NavigationDirection::Next, state);

    QVERIFY(!target.handledBySpread);
    QCOMPARE(target.pageNumber, 0);
}

void TestImageSpreadNavigation::adjacentNavigationUsesVisibleSpreadEdges()
{
    const KiriView::ImageSpreadNavigationState state { true, 2, 6, true, false };

    const KiriView::ImageSpreadPageNavigationTarget next
        = KiriView::imageSpreadPageNavigationTarget(KiriView::NavigationDirection::Next, state);
    const KiriView::ImageSpreadPageNavigationTarget previous
        = KiriView::imageSpreadPageNavigationTarget(KiriView::NavigationDirection::Previous, state);

    QVERIFY(next.handledBySpread);
    QCOMPARE(next.pageNumber, 4);
    QVERIFY(previous.handledBySpread);
    QCOMPARE(previous.pageNumber, 1);
    QCOMPARE(KiriView::imageSpreadNavigationCurrentLastPageNumber(state), 3);
}

void TestImageSpreadNavigation::previousNavigationAccountsForWidePreviousPage()
{
    const KiriView::ImageSpreadNavigationState narrowPreviousPage { true, 5, 8, true, false };
    const KiriView::ImageSpreadNavigationState widePreviousPage { true, 5, 8, true, true };

    QCOMPARE(KiriView::imageSpreadPageNavigationTarget(
                 KiriView::NavigationDirection::Previous, narrowPreviousPage)
                 .pageNumber,
        3);
    QCOMPARE(KiriView::imageSpreadPageNavigationTarget(
                 KiriView::NavigationDirection::Previous, widePreviousPage)
                 .pageNumber,
        4);
}

void TestImageSpreadNavigation::relativeNavigationAndTransitionsUseSpreadState()
{
    const KiriView::ImageSpreadNavigationState state { true, 3, 5, false, false };

    QCOMPARE(KiriView::imageSpreadRelativePageNavigationTarget(state, -1), 2);
    QCOMPARE(KiriView::imageSpreadRelativePageNavigationTarget(state, 1), 4);
    QVERIFY(KiriView::imageSpreadShouldBeginNavigationTransition(state, 4));
    QVERIFY(!KiriView::imageSpreadShouldBeginNavigationTransition(state, 6));
}

QTEST_GUILESS_MAIN(TestImageSpreadNavigation)

#include "test_imagespreadnavigation.moc"
