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
    kiriview::ImageDocumentDeletionState state;

    const kiriview::ImageDocumentDeletionFileOperationStart stale = state.startFileDeletion();
    const kiriview::ImageDocumentDeletionFileOperationStart current = state.startFileDeletion();

    QVERIFY(stale.operationId != 0);
    QVERIFY(current.operationId != 0);
    QVERIFY(stale.operationId != current.operationId);
    QVERIFY(state.inProgress());
    QVERIFY(stale.inProgressChanged);
    QVERIFY(!current.inProgressChanged);

    const kiriview::ImageDocumentDeletionFileOperationFinish staleFinish
        = state.finishFileDeletion(stale.operationId);
    QVERIFY(!staleFinish.accepted);
    QVERIFY(!staleFinish.inProgressChanged);
    QVERIFY(state.inProgress());

    const kiriview::ImageDocumentDeletionFileOperationFinish currentFinish
        = state.finishFileDeletion(current.operationId);
    QVERIFY(currentFinish.accepted);
    QVERIFY(currentFinish.inProgressChanged);
    QVERIFY(!state.inProgress());

    const kiriview::ImageDocumentDeletionFileOperationFinish repeatedFinish
        = state.finishFileDeletion(current.operationId);
    QVERIFY(!repeatedFinish.accepted);
    QVERIFY(!repeatedFinish.inProgressChanged);
}

void TestImageDocumentDeletionState::cancelFileDeletionClearsProgressAndRejectsCompletion()
{
    kiriview::ImageDocumentDeletionState state;
    const kiriview::ImageDocumentDeletionFileOperationStart operation = state.startFileDeletion();

    QVERIFY(state.inProgress());
    QVERIFY(state.cancelFileDeletion());
    QVERIFY(!state.inProgress());

    const kiriview::ImageDocumentDeletionFileOperationFinish finish
        = state.finishFileDeletion(operation.operationId);
    QVERIFY(!finish.accepted);
    QVERIFY(!finish.inProgressChanged);
    QVERIFY(!state.cancelFileDeletion());
}

void TestImageDocumentDeletionState::operationIdsStayNonZeroAfterWrap()
{
    kiriview::ImageDocumentDeletionState state(std::numeric_limits<quint64>::max());

    const kiriview::ImageDocumentDeletionFileOperationStart fileDeletion
        = state.startFileDeletion();

    QCOMPARE(fileDeletion.operationId, quint64(1));
}

QTEST_GUILESS_MAIN(TestImageDocumentDeletionState)

#include "test_imagedocumentdeletionstate.moc"
