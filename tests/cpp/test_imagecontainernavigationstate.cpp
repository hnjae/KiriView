// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainernavigationstate.h"

#include <QObject>
#include <QTest>
#include <limits>

class TestImageContainerNavigationState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void navigationCompletionClaimsOnlyCurrentOperation();
    void cancelRejectsPendingOperation();
    void operationIdsStayNonZeroAfterWrap();
};

void TestImageContainerNavigationState::navigationCompletionClaimsOnlyCurrentOperation()
{
    KiriView::ImageContainerNavigationState state;

    const quint64 stale = state.startNavigation();
    const quint64 current = state.startNavigation();

    QVERIFY(stale != 0);
    QVERIFY(current != 0);
    QVERIFY(stale != current);
    QVERIFY(!state.acceptsNavigation(stale));
    QVERIFY(state.acceptsNavigation(current));
    QVERIFY(!state.finishNavigation(stale));
    QVERIFY(state.finishNavigation(current));
    QVERIFY(!state.acceptsNavigation(current));
    QVERIFY(!state.finishNavigation(current));
}

void TestImageContainerNavigationState::cancelRejectsPendingOperation()
{
    KiriView::ImageContainerNavigationState state;
    const quint64 operation = state.startNavigation();

    QVERIFY(state.acceptsNavigation(operation));
    state.cancel();
    QVERIFY(!state.acceptsNavigation(operation));
    QVERIFY(!state.finishNavigation(operation));
}

void TestImageContainerNavigationState::operationIdsStayNonZeroAfterWrap()
{
    KiriView::ImageContainerNavigationState state(std::numeric_limits<quint64>::max());

    QCOMPARE(state.startNavigation(), quint64(1));
    QCOMPARE(state.startNavigation(), quint64(2));
}

QTEST_GUILESS_MAIN(TestImageContainerNavigationState)

#include "test_imagecontainernavigationstate.moc"
