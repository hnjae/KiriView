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

KiriView::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return KiriView::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
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

template <typename Operation>
const Operation *operationAt(
    const KiriView::DocumentSessionMediaDeletionCompletionPlan &plan, std::size_t index)
{
    if (index >= plan.routePlan.operations.size()) {
        return nullptr;
    }

    return std::get_if<Operation>(&plan.routePlan.operations.at(index));
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
    std::vector<KiriView::DirectMediaNavigationCandidate> candidates {
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/01.jpg"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/03.png"))),
    };
    KiriView::sortDirectMediaNavigationCandidates(&candidates);

    const QUrl deletedUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const KiriView::DocumentSessionMediaDeletionFallbackPlan plan
        = KiriView::documentSessionMediaDeletionFallbackPlan(candidates, deletedUrl);

    QCOMPARE(plan.targetUrl, deletedUrl);
    QCOMPARE(plan.preferredFallbackUrl.value(), localUrl(QStringLiteral("/media/03.png")));
    QCOMPARE(plan.fallbackUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));
}

void TestDocumentSessionMediaDeletionPlan::startPlanUsesDirectMediaTargetAndFallbacks()
{
    std::vector<KiriView::DirectMediaNavigationCandidate> candidates {
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/01.jpg"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/03.png"))),
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

    QVERIFY(imagePlan.hasRoutePlan());
    QVERIFY(!imagePlan.reportFailure);
    QCOMPARE(imagePlan.routePlan.kind, KiriView::DocumentSessionRouteKind::DirectImage);
    QCOMPARE(imagePlan.routePlan.sourceUrl, localUrl(QStringLiteral("/media/03.png")));
    QVERIFY(operationAt<KiriView::ClearImageDocumentRouteOperation>(imagePlan, 0) != nullptr);
    const auto *imageCursor
        = operationAt<KiriView::RequestDirectImageCursorRouteOperation>(imagePlan, 1);
    QVERIFY(imageCursor != nullptr);
    QCOMPARE(imageCursor->url, localUrl(QStringLiteral("/media/03.png")));

    const KiriView::DocumentSessionMediaDeletionCompletionPlan videoPlan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Video,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png"))),
            KiriView::FileDeletionResult::Succeeded);

    QVERIFY(videoPlan.hasRoutePlan());
    QCOMPARE(videoPlan.routePlan.kind, KiriView::DocumentSessionRouteKind::DirectImage);
    QVERIFY(operationAt<KiriView::LeaveVideoModeRouteOperation>(videoPlan, 0) != nullptr);
    QVERIFY(operationAt<KiriView::EnterEmptyDocumentRouteOperation>(videoPlan, 1) != nullptr);
}

void TestDocumentSessionMediaDeletionPlan::
    completionFallsBackToPreviousCandidateWhenNoNextCandidateExists()
{
    const KiriView::DocumentSessionMediaDeletionCompletionPlan plan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Video,
            fallbackPlan({}, localUrl(QStringLiteral("/media/01.jpg"))),
            KiriView::FileDeletionResult::Succeeded);

    QVERIFY(plan.hasRoutePlan());
    QCOMPARE(plan.routePlan.kind, KiriView::DocumentSessionRouteKind::DirectImage);
    QCOMPARE(plan.routePlan.sourceUrl, localUrl(QStringLiteral("/media/01.jpg")));
}

void TestDocumentSessionMediaDeletionPlan::completionClearsSessionWhenNoFallbackExists()
{
    const KiriView::DocumentSessionMediaDeletionCompletionPlan plan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Video,
            fallbackPlan(), KiriView::FileDeletionResult::Succeeded);

    QVERIFY(plan.hasRoutePlan());
    QVERIFY(!plan.reportFailure);
    QCOMPARE(plan.routePlan.kind, KiriView::DocumentSessionRouteKind::Empty);
    QVERIFY(plan.routePlan.sourceUrl.isEmpty());
    QVERIFY(operationAt<KiriView::ClearDirectMediaNavigationRouteOperation>(plan, 0) != nullptr);
    QVERIFY(operationAt<KiriView::ClearMediaPredecodeRouteOperation>(plan, 7) != nullptr);
}

void TestDocumentSessionMediaDeletionPlan::canceledCompletionIsNoOp()
{
    const KiriView::DocumentSessionMediaDeletionCompletionPlan plan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Image,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png"))),
            KiriView::FileDeletionResult::Canceled);

    QVERIFY(!plan.hasRoutePlan());
    QVERIFY(!plan.reportFailure);
}

void TestDocumentSessionMediaDeletionPlan::failedCompletionReportsFailureWithoutClearingDocument()
{
    const KiriView::DocumentSessionMediaDeletionCompletionPlan plan
        = KiriView::documentSessionMediaDeletionCompletionPlan(KiriView::DocumentSessionKind::Image,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png"))),
            KiriView::FileDeletionResult::Failed);

    QVERIFY(!plan.hasRoutePlan());
    QVERIFY(plan.reportFailure);
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaDeletionPlan)

#include "test_documentsessionmediadeletionplan.moc"
