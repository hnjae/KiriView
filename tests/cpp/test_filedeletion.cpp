// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "system/filedeletion.h"
#include "system/kiooperationfailure.h"

#include <KIO/Global>
#include <KJob>
#include <QObject>
#include <QTest>
#include <QUrl>

class TestFileDeletion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void kioOperationFailurePreservesKJobFields();
    void kioOperationFailureClassifiesCanceledErrors();
    void validationFailureHasDiagnosticWithoutRawCode();
    void completionActionRoutesDeletionResults();
};

void TestFileDeletion::kioOperationFailurePreservesKJobFields()
{
    const QUrl targetUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));
    const kiriview::KioOperationFailure failure
        = kiriview::kioOperationFailureFromKJob(kiriview::KioOperationKind::FileDeletion, targetUrl,
            KIO::ERR_ACCESS_DENIED, QStringLiteral("permission denied"));

    QCOMPARE(failure.operationKind, kiriview::KioOperationKind::FileDeletion);
    QCOMPARE(failure.targetUrl, targetUrl);
    QVERIFY(failure.rawErrorCode.has_value());
    QCOMPARE(*failure.rawErrorCode, KIO::ERR_ACCESS_DENIED);
    QVERIFY(!failure.canceled);
    QCOMPARE(failure.userMessage, QStringLiteral("permission denied"));
    QCOMPARE(failure.diagnosticDetail, QStringLiteral("permission denied"));
    QVERIFY(failure.retryable);
}

void TestFileDeletion::kioOperationFailureClassifiesCanceledErrors()
{
    const QUrl targetUrl = QUrl::fromLocalFile(QStringLiteral("/images/page.png"));

    for (int errorCode :
        { static_cast<int>(KJob::KilledJobError), static_cast<int>(KIO::ERR_USER_CANCELED) }) {
        const kiriview::KioOperationFailure failure
            = kiriview::kioOperationFailureFromKJob(kiriview::KioOperationKind::MediaOpenWith,
                targetUrl, errorCode, QStringLiteral("operation canceled"));

        QCOMPARE(failure.operationKind, kiriview::KioOperationKind::MediaOpenWith);
        QCOMPARE(failure.targetUrl, targetUrl);
        QVERIFY(failure.rawErrorCode.has_value());
        QCOMPARE(*failure.rawErrorCode, errorCode);
        QVERIFY(failure.canceled);
        QCOMPARE(failure.userMessage, QString());
        QCOMPARE(failure.diagnosticDetail, QStringLiteral("operation canceled"));
        QVERIFY(!failure.retryable);
    }
}

void TestFileDeletion::validationFailureHasDiagnosticWithoutRawCode()
{
    const kiriview::KioOperationFailure failure
        = kiriview::kioOperationValidationFailure(kiriview::KioOperationKind::MediaOpenWith, QUrl(),
            QStringLiteral("No Open With target is available."));

    QCOMPARE(failure.operationKind, kiriview::KioOperationKind::MediaOpenWith);
    QVERIFY(failure.targetUrl.isEmpty());
    QVERIFY(!failure.rawErrorCode.has_value());
    QVERIFY(!failure.canceled);
    QCOMPARE(failure.userMessage, QString());
    QCOMPARE(failure.diagnosticDetail, QStringLiteral("No Open With target is available."));
    QVERIFY(!failure.retryable);
}

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
