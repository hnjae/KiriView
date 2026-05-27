// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/documentsessionmediadeletionplan.h"

#include <QObject>
#include <QTest>
#include <QUrl>
#include <optional>
#include <utility>

namespace {
QUrl localUrl(const QString &path) { return QUrl::fromLocalFile(path); }

KiriView::MediaNavigationCandidate mediaCandidate(const QUrl &url)
{
    return KiriView::MediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

KiriView::DocumentSessionMediaDeletionFallbackPlan fallbackPlan(
    std::optional<QUrl> preferredFallback = {}, std::optional<QUrl> fallback = {})
{
    return KiriView::DocumentSessionMediaDeletionFallbackPlan {
        localUrl(QStringLiteral("/media/02.mp4")),
        std::move(preferredFallback),
        std::move(fallback),
    };
}
}

class TestDocumentSessionMediaDeletionPlan : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startPlanRejectsEmptyCurrentUrl();
    void fallbackPlanUsesOriginalDirectMediaUrl();
    void startPlanUsesDirectMediaTargetAndFallbacks();
    void completionOpensPreferredFallbackAfterClearingActiveDocument();
    void completionFallsBackToPreviousCandidateWhenNoNextCandidateExists();
    void completionClearsSessionWhenNoFallbackExists();
    void canceledCompletionIsNoOp();
    void failedCompletionReportsFailureWithoutClearingDocument();
};

void TestDocumentSessionMediaDeletionPlan::startPlanRejectsEmptyCurrentUrl()
{
    const KiriView::DocumentSessionMediaDeletionStartPlan plan
        = KiriView::documentSessionMediaDeletionStartPlan(
            KiriView::FileDeletionMode::MoveToTrash, {}, {});

    QVERIFY(!plan.shouldStartDeletion);
    QVERIFY(plan.request.targetUrl.isEmpty());
    QVERIFY(!plan.fallbackPlan.hasTarget());
}

void TestDocumentSessionMediaDeletionPlan::fallbackPlanUsesOriginalDirectMediaUrl()
{
    std::vector<KiriView::MediaNavigationCandidate> candidates {
        mediaCandidate(localUrl(QStringLiteral("/media/01.jpg"))),
        mediaCandidate(localUrl(QStringLiteral("/media/03.png"))),
    };
    KiriView::sortMediaNavigationCandidates(&candidates);

    const QUrl deletedUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const KiriView::DocumentSessionMediaDeletionFallbackPlan plan
        = KiriView::documentSessionMediaDeletionFallbackPlan(candidates, deletedUrl);

    QCOMPARE(plan.targetUrl, deletedUrl);
    QCOMPARE(plan.preferredFallbackUrl.value(), localUrl(QStringLiteral("/media/03.png")));
    QCOMPARE(plan.fallbackUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));
}

void TestDocumentSessionMediaDeletionPlan::startPlanUsesDirectMediaTargetAndFallbacks()
{
    std::vector<KiriView::MediaNavigationCandidate> candidates {
        mediaCandidate(localUrl(QStringLiteral("/media/01.jpg"))),
        mediaCandidate(localUrl(QStringLiteral("/media/03.png"))),
    };

    const KiriView::DocumentSessionMediaDeletionStartPlan plan
        = KiriView::documentSessionMediaDeletionStartPlan(
            KiriView::FileDeletionMode::DeletePermanently, std::move(candidates),
            localUrl(QStringLiteral("/media/02.mp4")));

    QVERIFY(plan.shouldStartDeletion);
    QCOMPARE(plan.request.targetUrl, localUrl(QStringLiteral("/media/02.mp4")));
    QCOMPARE(plan.request.mode, KiriView::FileDeletionMode::DeletePermanently);
    QCOMPARE(plan.fallbackPlan.targetUrl, localUrl(QStringLiteral("/media/02.mp4")));
    QCOMPARE(
        plan.fallbackPlan.preferredFallbackUrl.value(), localUrl(QStringLiteral("/media/03.png")));
    QCOMPARE(plan.fallbackPlan.fallbackUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));
}

void TestDocumentSessionMediaDeletionPlan::
    completionOpensPreferredFallbackAfterClearingActiveDocument()
{
    const KiriView::DocumentSessionMediaDeletionCompletionPlan imagePlan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Image,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png")),
                localUrl(QStringLiteral("/media/01.jpg"))),
            KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(imagePlan.clearDocument, KiriView::DocumentSessionMediaDeletionDocumentClear::Image);
    QCOMPARE(imagePlan.followUp, KiriView::DocumentSessionMediaDeletionFollowUp::OpenFallback);
    QCOMPARE(imagePlan.fallbackUrl, localUrl(QStringLiteral("/media/03.png")));
    QVERIFY(!imagePlan.clearMediaNavigation);
    QVERIFY(!imagePlan.clearPredecode);

    const KiriView::DocumentSessionMediaDeletionCompletionPlan videoPlan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Video,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png"))),
            KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(videoPlan.clearDocument, KiriView::DocumentSessionMediaDeletionDocumentClear::Video);
    QCOMPARE(videoPlan.followUp, KiriView::DocumentSessionMediaDeletionFollowUp::OpenFallback);
}

void TestDocumentSessionMediaDeletionPlan::
    completionFallsBackToPreviousCandidateWhenNoNextCandidateExists()
{
    const KiriView::DocumentSessionMediaDeletionCompletionPlan plan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Video,
            fallbackPlan({}, localUrl(QStringLiteral("/media/01.jpg"))),
            KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(plan.clearDocument, KiriView::DocumentSessionMediaDeletionDocumentClear::Video);
    QCOMPARE(plan.followUp, KiriView::DocumentSessionMediaDeletionFollowUp::OpenFallback);
    QCOMPARE(plan.fallbackUrl, localUrl(QStringLiteral("/media/01.jpg")));
}

void TestDocumentSessionMediaDeletionPlan::completionClearsSessionWhenNoFallbackExists()
{
    const KiriView::DocumentSessionMediaDeletionCompletionPlan plan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Video,
            fallbackPlan(), KiriView::FileDeletionResult::Succeeded);

    QCOMPARE(plan.clearDocument, KiriView::DocumentSessionMediaDeletionDocumentClear::Video);
    QCOMPARE(plan.followUp, KiriView::DocumentSessionMediaDeletionFollowUp::ClearSession);
    QVERIFY(plan.fallbackUrl.isEmpty());
    QVERIFY(plan.clearMediaNavigation);
    QVERIFY(plan.clearPredecode);
}

void TestDocumentSessionMediaDeletionPlan::canceledCompletionIsNoOp()
{
    const KiriView::DocumentSessionMediaDeletionCompletionPlan plan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Image,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png"))),
            KiriView::FileDeletionResult::Canceled);

    QCOMPARE(plan.clearDocument, KiriView::DocumentSessionMediaDeletionDocumentClear::None);
    QCOMPARE(plan.followUp, KiriView::DocumentSessionMediaDeletionFollowUp::None);
    QVERIFY(plan.fallbackUrl.isEmpty());
    QVERIFY(!plan.clearMediaNavigation);
    QVERIFY(!plan.clearPredecode);
}

void TestDocumentSessionMediaDeletionPlan::failedCompletionReportsFailureWithoutClearingDocument()
{
    const KiriView::DocumentSessionMediaDeletionCompletionPlan plan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Image,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png"))),
            KiriView::FileDeletionResult::Failed);

    QCOMPARE(plan.clearDocument, KiriView::DocumentSessionMediaDeletionDocumentClear::None);
    QCOMPARE(plan.followUp, KiriView::DocumentSessionMediaDeletionFollowUp::ReportFailure);
    QVERIFY(plan.fallbackUrl.isEmpty());
    QVERIFY(!plan.clearMediaNavigation);
    QVERIFY(!plan.clearPredecode);
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaDeletionPlan)

#include "test_documentsessionmediadeletionplan.moc"
