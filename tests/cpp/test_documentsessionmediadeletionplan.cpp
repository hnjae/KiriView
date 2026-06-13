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

kiriview::DirectMediaNavigationCandidate directMediaNavigationCandidate(const QUrl &url)
{
    return kiriview::DirectMediaNavigationCandidate { url, url.fileName(QUrl::PrettyDecoded) };
}

kiriview::DocumentSessionMediaDeletionFallbackPlan fallbackPlan(
    std::optional<QUrl> preferredFallback = {}, std::optional<QUrl> fallback = {})
{
    return kiriview::DocumentSessionMediaDeletionFallbackPlan {
        localUrl(QStringLiteral("/media/02.mp4")),
        std::move(preferredFallback),
        std::move(fallback),
    };
}

template <typename Operation>
const Operation *operationAt(
    const kiriview::DocumentSessionMediaDeletionCompletionPlan &plan, std::size_t index)
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
    const kiriview::DocumentSessionMediaDeletionStartPlan plan
        = kiriview::documentSessionMediaDeletionStartPlan(
            kiriview::FileDeletionMode::MoveToTrash, {}, {});

    QVERIFY(!plan.shouldStartDeletion);
    QVERIFY(plan.request.targetUrl.isEmpty());
    QVERIFY(!plan.fallbackPlan.hasTarget());
}

void TestDocumentSessionMediaDeletionPlan::fallbackPlanUsesOriginalDirectMediaUrl()
{
    std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/01.jpg"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/03.png"))),
    };
    kiriview::sortDirectMediaNavigationCandidates(&candidates);

    const QUrl deletedUrl = localUrl(QStringLiteral("/media/02.mp4"));
    const kiriview::DocumentSessionMediaDeletionFallbackPlan plan
        = kiriview::documentSessionMediaDeletionFallbackPlan(candidates, deletedUrl);

    QCOMPARE(plan.targetUrl, deletedUrl);
    QCOMPARE(plan.preferredFallbackUrl.value(), localUrl(QStringLiteral("/media/03.png")));
    QCOMPARE(plan.fallbackUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));
}

void TestDocumentSessionMediaDeletionPlan::startPlanUsesDirectMediaTargetAndFallbacks()
{
    std::vector<kiriview::DirectMediaNavigationCandidate> candidates {
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/01.jpg"))),
        directMediaNavigationCandidate(localUrl(QStringLiteral("/media/03.png"))),
    };

    const kiriview::DocumentSessionMediaDeletionStartPlan plan
        = kiriview::documentSessionMediaDeletionStartPlan(
            kiriview::FileDeletionMode::DeletePermanently, std::move(candidates),
            localUrl(QStringLiteral("/media/02.mp4")));

    QVERIFY(plan.shouldStartDeletion);
    QCOMPARE(plan.request.targetUrl, localUrl(QStringLiteral("/media/02.mp4")));
    QCOMPARE(plan.request.mode, kiriview::FileDeletionMode::DeletePermanently);
    QCOMPARE(plan.fallbackPlan.targetUrl, localUrl(QStringLiteral("/media/02.mp4")));
    QCOMPARE(
        plan.fallbackPlan.preferredFallbackUrl.value(), localUrl(QStringLiteral("/media/03.png")));
    QCOMPARE(plan.fallbackPlan.fallbackUrl.value(), localUrl(QStringLiteral("/media/01.jpg")));
}

void TestDocumentSessionMediaDeletionPlan::
    completionOpensPreferredFallbackAfterClearingActiveDocument()
{
    const kiriview::DocumentSessionMediaDeletionCompletionPlan imagePlan
        = kiriview::documentSessionMediaDeletionCompletionPlan(kiriview::DocumentSessionKind::Image,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png")),
                localUrl(QStringLiteral("/media/01.jpg"))),
            kiriview::FileDeletionResult::Succeeded);

    QVERIFY(imagePlan.hasRoutePlan());
    QVERIFY(!imagePlan.reportFailure);
    QCOMPARE(imagePlan.routePlan.kind, kiriview::DocumentSessionRouteKind::DirectImage);
    QCOMPARE(imagePlan.routePlan.sourceUrl, localUrl(QStringLiteral("/media/03.png")));
    QVERIFY(operationAt<kiriview::ClearImageDocumentRouteOperation>(imagePlan, 0) != nullptr);
    const auto *imageCursor
        = operationAt<kiriview::RequestDirectImageCursorRouteOperation>(imagePlan, 1);
    QVERIFY(imageCursor != nullptr);
    QCOMPARE(imageCursor->url, localUrl(QStringLiteral("/media/03.png")));

    const kiriview::DocumentSessionMediaDeletionCompletionPlan videoPlan
        = kiriview::documentSessionMediaDeletionCompletionPlan(kiriview::DocumentSessionKind::Video,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png"))),
            kiriview::FileDeletionResult::Succeeded);

    QVERIFY(videoPlan.hasRoutePlan());
    QCOMPARE(videoPlan.routePlan.kind, kiriview::DocumentSessionRouteKind::DirectImage);
    QVERIFY(operationAt<kiriview::LeaveVideoModeRouteOperation>(videoPlan, 0) != nullptr);
    QVERIFY(operationAt<kiriview::EnterEmptyDocumentRouteOperation>(videoPlan, 1) != nullptr);
}

void TestDocumentSessionMediaDeletionPlan::
    completionFallsBackToPreviousCandidateWhenNoNextCandidateExists()
{
    const kiriview::DocumentSessionMediaDeletionCompletionPlan plan
        = kiriview::documentSessionMediaDeletionCompletionPlan(kiriview::DocumentSessionKind::Video,
            fallbackPlan({}, localUrl(QStringLiteral("/media/01.jpg"))),
            kiriview::FileDeletionResult::Succeeded);

    QVERIFY(plan.hasRoutePlan());
    QCOMPARE(plan.routePlan.kind, kiriview::DocumentSessionRouteKind::DirectImage);
    QCOMPARE(plan.routePlan.sourceUrl, localUrl(QStringLiteral("/media/01.jpg")));
}

void TestDocumentSessionMediaDeletionPlan::completionClearsSessionWhenNoFallbackExists()
{
    const kiriview::DocumentSessionMediaDeletionCompletionPlan plan
        = kiriview::documentSessionMediaDeletionCompletionPlan(kiriview::DocumentSessionKind::Video,
            fallbackPlan(), kiriview::FileDeletionResult::Succeeded);

    QVERIFY(plan.hasRoutePlan());
    QVERIFY(!plan.reportFailure);
    QCOMPARE(plan.routePlan.kind, kiriview::DocumentSessionRouteKind::Empty);
    QVERIFY(plan.routePlan.sourceUrl.isEmpty());
    QVERIFY(operationAt<kiriview::ClearDirectMediaNavigationRouteOperation>(plan, 0) != nullptr);
    QVERIFY(operationAt<kiriview::ClearMediaPredecodeRouteOperation>(plan, 7) != nullptr);
}

void TestDocumentSessionMediaDeletionPlan::canceledCompletionIsNoOp()
{
    const kiriview::DocumentSessionMediaDeletionCompletionPlan plan
        = kiriview::documentSessionMediaDeletionCompletionPlan(kiriview::DocumentSessionKind::Image,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png"))),
            kiriview::FileDeletionResult::Canceled);

    QVERIFY(!plan.hasRoutePlan());
    QVERIFY(!plan.reportFailure);
}

void TestDocumentSessionMediaDeletionPlan::failedCompletionReportsFailureWithoutClearingDocument()
{
    const kiriview::DocumentSessionMediaDeletionCompletionPlan plan
        = kiriview::documentSessionMediaDeletionCompletionPlan(kiriview::DocumentSessionKind::Image,
            fallbackPlan(localUrl(QStringLiteral("/media/03.png"))),
            kiriview::FileDeletionResult::Failed);

    QVERIFY(!plan.hasRoutePlan());
    QVERIFY(plan.reportFailure);
}

QTEST_GUILESS_MAIN(TestDocumentSessionMediaDeletionPlan)

#include "test_documentsessionmediadeletionplan.moc"
