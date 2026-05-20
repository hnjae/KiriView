// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "document/imagedocumentdeletionstate.h"

#include <QObject>
#include <QTest>
#include <limits>

class TestImageDocumentDeletionState : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fileDeletionCompletionClaimsOnlyCurrentOperation();
    void cancelFileDeletionClearsProgressAndRejectsCompletion();
    void operationIdsStayNonZeroAfterWrap();
};

void TestImageDocumentDeletionState::fileDeletionCompletionClaimsOnlyCurrentOperation()
{
    KiriView::ImageDocumentDeletionState state;

    const KiriView::ImageDocumentDeletionFileOperationStart stale = state.startFileDeletion();
    const KiriView::ImageDocumentDeletionFileOperationStart current = state.startFileDeletion();

    QVERIFY(stale.operationId != 0);
    QVERIFY(current.operationId != 0);
    QVERIFY(stale.operationId != current.operationId);
    QVERIFY(state.inProgress());
    QVERIFY(stale.inProgressChanged);
    QVERIFY(!current.inProgressChanged);

    const KiriView::ImageDocumentDeletionFileOperationFinish staleFinish
        = state.finishFileDeletion(stale.operationId);
    QVERIFY(!staleFinish.accepted);
    QVERIFY(!staleFinish.inProgressChanged);
    QVERIFY(state.inProgress());

    const KiriView::ImageDocumentDeletionFileOperationFinish currentFinish
        = state.finishFileDeletion(current.operationId);
    QVERIFY(currentFinish.accepted);
    QVERIFY(currentFinish.inProgressChanged);
    QVERIFY(!state.inProgress());

    const KiriView::ImageDocumentDeletionFileOperationFinish repeatedFinish
        = state.finishFileDeletion(current.operationId);
    QVERIFY(!repeatedFinish.accepted);
    QVERIFY(!repeatedFinish.inProgressChanged);
}

void TestImageDocumentDeletionState::cancelFileDeletionClearsProgressAndRejectsCompletion()
{
    KiriView::ImageDocumentDeletionState state;
    const KiriView::ImageDocumentDeletionFileOperationStart operation = state.startFileDeletion();

    QVERIFY(state.inProgress());
    QVERIFY(state.cancelFileDeletion());
    QVERIFY(!state.inProgress());

    const KiriView::ImageDocumentDeletionFileOperationFinish finish
        = state.finishFileDeletion(operation.operationId);
    QVERIFY(!finish.accepted);
    QVERIFY(!finish.inProgressChanged);
    QVERIFY(!state.cancelFileDeletion());
}

void TestImageDocumentDeletionState::operationIdsStayNonZeroAfterWrap()
{
    KiriView::ImageDocumentDeletionState state(std::numeric_limits<quint64>::max());

    const KiriView::ImageDocumentDeletionFileOperationStart fileDeletion
        = state.startFileDeletion();

    QCOMPARE(fileDeletion.operationId, quint64(1));
}

QTEST_GUILESS_MAIN(TestImageDocumentDeletionState)

#include "test_imagedocumentdeletionstate.moc"
