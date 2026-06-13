// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "system/filedeletion.h"

#include <QObject>
#include <QTest>

class TestFileDeletion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void completionActionRoutesDeletionResults();
};

void TestFileDeletion::completionActionRoutesDeletionResults()
{
    QCOMPARE(static_cast<int>(
                 kiriview::fileDeletionCompletionAction(kiriview::FileDeletionResult::Succeeded)),
        static_cast<int>(
            kiriview::FileDeletionCompletionAction::ClearDeletedTargetAndOpenFallback));
    QCOMPARE(static_cast<int>(
                 kiriview::fileDeletionCompletionAction(kiriview::FileDeletionResult::Canceled)),
        static_cast<int>(kiriview::FileDeletionCompletionAction::Ignore));
    QCOMPARE(static_cast<int>(
                 kiriview::fileDeletionCompletionAction(kiriview::FileDeletionResult::Failed)),
        static_cast<int>(kiriview::FileDeletionCompletionAction::ReportFailure));
}

QTEST_GUILESS_MAIN(TestFileDeletion)

#include "test_filedeletion.moc"
