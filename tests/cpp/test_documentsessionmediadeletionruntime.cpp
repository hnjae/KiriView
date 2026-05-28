// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediadeletionruntime.h"

#include "image_async_test_support.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <utility>
#include <vector>

class TestDocumentSessionMediaDeletionRuntime : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyTargetDoesNotStartFileOperation();
    void startRunsFileOperationAndPublishesCompletionPlan();
    void cancelRejectsLateCompletion();
    void replacementStartRejectsStaleCompletion();
    void failedCompletionReportsFailureWithErrorText();
};

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return KiriView::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

struct RuntimeFixture {
    QObject receiver;
    KiriView::TestSupport::ManualFileOperationProvider fileOperations;
    KiriView::DocumentSessionMediaDeletionRuntime runtime {
        KiriView::TestSupport::fileOperationProviderFor(fileOperations)
    };
    int completionCount = 0;
    KiriView::DocumentSessionMediaDeletionCompletion completion;

    KiriView::DocumentSessionMediaDeletionStartPlan start(KiriView::FileDeletionMode mode,
        std::vector<KiriView::DirectMediaNavigationCandidate> candidates, const QUrl &currentUrl,
        KiriView::DocumentSessionKind kind = KiriView::DocumentSessionKind::Video)
    {
        return runtime.start(&receiver, mode, std::move(candidates), currentUrl, kind,
            [this](KiriView::DocumentSessionMediaDeletionCompletion deletionCompletion) {
                ++completionCount;
                completion = std::move(deletionCompletion);
            });
    }
};
}

void TestDocumentSessionMediaDeletionRuntime::emptyTargetDoesNotStartFileOperation()
{
    RuntimeFixture fixture;

    const KiriView::DocumentSessionMediaDeletionStartPlan plan = fixture.start(
        KiriView::FileDeletionMode::MoveToTrash, {}, QUrl(), KiriView::DocumentSessionKind::Video);

    QVERIFY(!plan.shouldStartDeletion);
    QCOMPARE(fixture.fileOperations.operationCount(), std::size_t(0));
    QVERIFY(!fixture.runtime.active());
    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaDeletionRuntime::startRunsFileOperationAndPublishesCompletionPlan()
{
    RuntimeFixture fixture;
    const QUrl previousUrl = localUrl(QStringLiteral("/media/01.jpg"));
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const QUrl nextUrl = localUrl(QStringLiteral("/media/03.png"));

    const KiriView::DocumentSessionMediaDeletionStartPlan startPlan = fixture.start(
        KiriView::FileDeletionMode::DeletePermanently,
        { directMediaNavigationCandidate(previousUrl), directMediaNavigationCandidate(currentUrl),
            directMediaNavigationCandidate(nextUrl) },
        currentUrl);

    QVERIFY(startPlan.shouldStartDeletion);
    QCOMPARE(fixture.fileOperations.operationCount(), std::size_t(1));
    QCOMPARE(fixture.fileOperations.backOperation().request.targetUrl, currentUrl);
    QCOMPARE(fixture.fileOperations.backOperation().request.mode,
        KiriView::FileDeletionMode::DeletePermanently);
    QVERIFY(fixture.runtime.active());
    QCOMPARE(startPlan.fallbackPlan.preferredFallbackUrl.value(), nextUrl);

    fixture.fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(fixture.completionCount, 1);
    QVERIFY(!fixture.runtime.active());
    QCOMPARE(fixture.completion.plan.clearDocument,
        KiriView::DocumentSessionMediaDeletionDocumentClear::Video);
    QCOMPARE(fixture.completion.plan.followUp,
        KiriView::DocumentSessionMediaDeletionFollowUp::OpenFallback);
    QCOMPARE(fixture.completion.plan.fallbackUrl, nextUrl);
    QCOMPARE(fixture.completion.errorString, QString());
}

void TestDocumentSessionMediaDeletionRuntime::cancelRejectsLateCompletion()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.mp4"));

    fixture.start(KiriView::FileDeletionMode::MoveToTrash,
        { directMediaNavigationCandidate(currentUrl) }, currentUrl);
    fixture.runtime.cancel();
    QVERIFY(fixture.fileOperations.backOperation().canceled);
    QVERIFY(!fixture.runtime.active());

    fixture.fileOperations.deliverOperationAtIgnoringCancellation(
        0, KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(fixture.completionCount, 0);
}

void TestDocumentSessionMediaDeletionRuntime::replacementStartRejectsStaleCompletion()
{
    RuntimeFixture fixture;
    const QUrl firstUrl = localUrl(QStringLiteral("/media/01.mp4"));
    const QUrl firstFallbackUrl = localUrl(QStringLiteral("/media/02.png"));
    const QUrl secondUrl = localUrl(QStringLiteral("/media/03.mp4"));
    const QUrl secondFallbackUrl = localUrl(QStringLiteral("/media/04.png"));

    fixture.start(KiriView::FileDeletionMode::MoveToTrash,
        { directMediaNavigationCandidate(firstUrl),
            directMediaNavigationCandidate(firstFallbackUrl) },
        firstUrl);
    fixture.start(KiriView::FileDeletionMode::MoveToTrash,
        { directMediaNavigationCandidate(secondUrl),
            directMediaNavigationCandidate(secondFallbackUrl) },
        secondUrl);

    QCOMPARE(fixture.fileOperations.operationCount(), std::size_t(2));
    QVERIFY(fixture.fileOperations.operationAt(0).canceled);

    fixture.fileOperations.deliverOperationAtIgnoringCancellation(
        0, KiriView::FileDeletionResult::Succeeded);
    QCOMPARE(fixture.completionCount, 0);

    fixture.fileOperations.finishBackOperation(KiriView::FileDeletionResult::Succeeded);
    QCOMPARE(fixture.completionCount, 1);
    QCOMPARE(fixture.completion.plan.fallbackUrl, secondFallbackUrl);
}

void TestDocumentSessionMediaDeletionRuntime::failedCompletionReportsFailureWithErrorText()
{
    RuntimeFixture fixture;
    const QUrl currentUrl = localUrl(QStringLiteral("/media/02.mp4"));

    fixture.start(KiriView::FileDeletionMode::MoveToTrash,
        { directMediaNavigationCandidate(currentUrl) }, currentUrl,
        KiriView::DocumentSessionKind::Image);
    fixture.fileOperations.finishBackOperation(
        KiriView::FileDeletionResult::Failed, QStringLiteral("delete failed"));

    QCOMPARE(fixture.completionCount, 1);
    QCOMPARE(fixture.completion.plan.clearDocument,
        KiriView::DocumentSessionMediaDeletionDocumentClear::None);
    QCOMPARE(fixture.completion.plan.followUp,
        KiriView::DocumentSessionMediaDeletionFollowUp::ReportFailure);
    QCOMPARE(fixture.completion.errorString, QStringLiteral("delete failed"));
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaDeletionRuntime)

#include "test_documentsessionmediadeletionruntime.moc"
