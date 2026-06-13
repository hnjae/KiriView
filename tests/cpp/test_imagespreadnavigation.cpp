// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadnavigation.h"

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
    const kiriview::ImageSpreadNavigationState state { false, 3, 6, true, false };

    const kiriview::ImageSpreadPageNavigationTarget target
        = kiriview::imageSpreadPageNavigationTarget(kiriview::NavigationDirection::Next, state);

    QVERIFY(!target.handledBySpread);
    QCOMPARE(target.pageNumber, 0);
}

void TestImageSpreadNavigation::adjacentNavigationUsesVisibleSpreadEdges()
{
    const kiriview::ImageSpreadNavigationState state { true, 2, 6, true, false };

    const kiriview::ImageSpreadPageNavigationTarget next
        = kiriview::imageSpreadPageNavigationTarget(kiriview::NavigationDirection::Next, state);
    const kiriview::ImageSpreadPageNavigationTarget previous
        = kiriview::imageSpreadPageNavigationTarget(kiriview::NavigationDirection::Previous, state);

    QVERIFY(next.handledBySpread);
    QCOMPARE(next.pageNumber, 4);
    QVERIFY(previous.handledBySpread);
    QCOMPARE(previous.pageNumber, 1);
    QCOMPARE(kiriview::imageSpreadNavigationCurrentLastPageNumber(state), 3);
}

void TestImageSpreadNavigation::previousNavigationAccountsForWidePreviousPage()
{
    const kiriview::ImageSpreadNavigationState narrowPreviousPage { true, 5, 8, true, false };
    const kiriview::ImageSpreadNavigationState widePreviousPage { true, 5, 8, true, true };

    QCOMPARE(kiriview::imageSpreadPageNavigationTarget(
                 kiriview::NavigationDirection::Previous, narrowPreviousPage)
                 .pageNumber,
        3);
    QCOMPARE(kiriview::imageSpreadPageNavigationTarget(
                 kiriview::NavigationDirection::Previous, widePreviousPage)
                 .pageNumber,
        4);
}

void TestImageSpreadNavigation::relativeNavigationAndTransitionsUseSpreadState()
{
    const kiriview::ImageSpreadNavigationState state { true, 3, 5, false, false };

    QCOMPARE(kiriview::imageSpreadRelativePageNavigationTarget(state, -1), 2);
    QCOMPARE(kiriview::imageSpreadRelativePageNavigationTarget(state, 1), 4);
    QVERIFY(kiriview::imageSpreadShouldBeginNavigationTransition(state, 4));
    QVERIFY(!kiriview::imageSpreadShouldBeginNavigationTransition(state, 6));
}

QTEST_GUILESS_MAIN(TestImageSpreadNavigation)

#include "test_imagespreadnavigation.moc"
