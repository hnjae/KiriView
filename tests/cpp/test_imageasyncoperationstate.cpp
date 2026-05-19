// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageasyncoperationstate.h"

#include <QObject>
#include <QTest>
#include <limits>

class TestImageAsyncOperationState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void completionClaimsOnlyCurrentOperation();
    void cancelRejectsPendingOperation();
    void operationIdsStayNonZeroAfterWrap();
};

void TestImageAsyncOperationState::completionClaimsOnlyCurrentOperation()
{
    KiriView::ImageAsyncOperationState state;

    const quint64 stale = state.start();
    const quint64 current = state.start();

    QVERIFY(stale != 0);
    QVERIFY(current != 0);
    QVERIFY(stale != current);
    QVERIFY(!state.accepts(stale));
    QVERIFY(state.accepts(current));
    QVERIFY(!state.finish(stale));
    QVERIFY(state.finish(current));
    QVERIFY(!state.accepts(current));
    QVERIFY(!state.finish(current));
}

void TestImageAsyncOperationState::cancelRejectsPendingOperation()
{
    KiriView::ImageAsyncOperationState state;
    const quint64 operation = state.start();

    QVERIFY(state.accepts(operation));
    state.cancel();
    QVERIFY(!state.accepts(operation));
    QVERIFY(!state.finish(operation));
}

void TestImageAsyncOperationState::operationIdsStayNonZeroAfterWrap()
{
    KiriView::ImageAsyncOperationState state(std::numeric_limits<quint64>::max());

    QCOMPARE(state.start(), quint64(1));
    QCOMPARE(state.start(), quint64(2));
}

QTEST_GUILESS_MAIN(TestImageAsyncOperationState)

#include "test_imageasyncoperationstate.moc"
