// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "filedeletion.h"

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
                 KiriView::fileDeletionCompletionAction(KiriView::FileDeletionResult::Succeeded)),
        static_cast<int>(KiriView::FileDeletionCompletionAction::ClearDeletedImageAndOpenFallback));
    QCOMPARE(static_cast<int>(
                 KiriView::fileDeletionCompletionAction(KiriView::FileDeletionResult::Canceled)),
        static_cast<int>(KiriView::FileDeletionCompletionAction::Ignore));
    QCOMPARE(static_cast<int>(
                 KiriView::fileDeletionCompletionAction(KiriView::FileDeletionResult::Failed)),
        static_cast<int>(KiriView::FileDeletionCompletionAction::ReportFailure));
}

QTEST_GUILESS_MAIN(TestFileDeletion)

#include "test_filedeletion.moc"
