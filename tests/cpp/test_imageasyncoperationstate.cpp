// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "async/imageasyncoperationstate.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <limits>

class TestImageAsyncOperationState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void completionClaimsOnlyCurrentOperation();
    void cancelRejectsPendingOperation();
    void operationIdsStayNonZeroAfterWrap();
    void scopedCompletionRequiresCurrentOperationAndScope();
    void scopedCancelRejectsPendingOperation();
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

void TestImageAsyncOperationState::scopedCompletionRequiresCurrentOperationAndScope()
{
    KiriView::ImageAsyncScopedOperationState<QUrl> state;

    const KiriView::ImageAsyncScopedOperation<QUrl> stale
        = state.start(QUrl::fromLocalFile(QStringLiteral("/media/01.mp4")));
    const KiriView::ImageAsyncScopedOperation<QUrl> current
        = state.start(QUrl::fromLocalFile(QStringLiteral("/media/02.mp4")));

    QVERIFY(stale.operationId != 0);
    QVERIFY(current.operationId != 0);
    QVERIFY(stale.operationId != current.operationId);
    QVERIFY(state.active());
    QVERIFY(!state.accepts(stale));
    QVERIFY(!state.accepts(current.operationId, stale.scope));
    QVERIFY(state.accepts(current));
    QVERIFY(!state.finish(stale));
    QVERIFY(state.finish(current));
    QVERIFY(!state.active());
    QVERIFY(!state.accepts(current));
}

void TestImageAsyncOperationState::scopedCancelRejectsPendingOperation()
{
    KiriView::ImageAsyncScopedOperationState<QUrl> state;
    const KiriView::ImageAsyncScopedOperation<QUrl> operation
        = state.start(QUrl::fromLocalFile(QStringLiteral("/media/01.mp4")));

    QVERIFY(state.active());
    QVERIFY(state.accepts(operation));
    state.cancel();
    QVERIFY(!state.active());
    QVERIFY(!state.accepts(operation));
    QVERIFY(!state.finish(operation));
}

QTEST_GUILESS_MAIN(TestImageAsyncOperationState)

#include "test_imageasyncoperationstate.moc"
