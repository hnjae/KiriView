// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadnavigation.h"

#include <QTest>

class TestImageSpreadNavigation : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void adjacentNavigationFallsBackWhenSpreadIsInactive();
    void adjacentNavigationUsesVisibleSpreadEdges();
    void previousNavigationAccountsForWidePreviousPage();
    void relativeNavigationAndTransitionsUseSpreadState();
};

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
