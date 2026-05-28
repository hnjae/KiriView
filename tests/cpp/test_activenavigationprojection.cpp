// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/activenavigationprojection.h"

#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QObject>
#include <QTest>
#include <variant>

namespace {
void compareUnavailable(const KiriView::ActiveNavigationSnapshot &snapshot)
{
    QVERIFY(!snapshot.available);
    QVERIFY(!snapshot.known);
    QVERIFY(!snapshot.editable);
    QVERIFY(!snapshot.canOpenPrevious);
    QVERIFY(!snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(!snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 0);
    QCOMPARE(snapshot.count, 0);
}

void compareUnknown(const KiriView::ActiveNavigationSnapshot &snapshot)
{
    QVERIFY(snapshot.available);
    QVERIFY(!snapshot.known);
    QVERIFY(!snapshot.editable);
    QVERIFY(!snapshot.canOpenPrevious);
    QVERIFY(!snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(!snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 0);
    QCOMPARE(snapshot.count, 0);
}

template <typename Operation>
const Operation *dispatchOperation(const KiriView::ActiveNavigationDispatchPlan &plan)
{
    return std::get_if<Operation>(&plan.operation);
}

KiriView::ImageDocumentPageActiveNavigationSnapshot imageDocumentActiveNavigationSnapshot(
    bool canOpenPrevious, bool canOpenNext, bool atKnownFirst, bool atKnownLast, int currentNumber,
    int count)
{
    return KiriView::ImageDocumentPageActiveNavigationSnapshot {
        true,
        canOpenPrevious,
        canOpenNext,
        atKnownFirst,
        atKnownLast,
        currentNumber,
        count,
    };
}
}

class TestActiveNavigationProjection : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unavailableSourceProjectsUnavailable();
    void unknownMediaStateProjectsAvailableButUnknown();
    void validDirectMediaNavigationBoundaryStateProjectsReadoutAndActions();
    void invalidMediaNumbersNormalizeToUnknown();
    void validImageDocumentSnapshotProjectsSpreadAwareBoundaries();
    void imageDocumentProjectionPreservesLeafOwnedSnapshotBoundaries();
    void invalidImagePageValuesNormalizeToUnknown();
    void deletionMaskingDisablesDispatchButKeepsKnownReadout();
    void boundaryScopeMapsSourceKind();
    void directMediaDispatchPlanFollowsProjectedBoundaryGates();
    void imageDocumentDispatchPlanUsesNumberedPageTargets();
    void previousNextDispatchPlanReportsEditableBoundaries();
    void dispatchPlanRejectsUnknownMaskedAndUnavailableNavigation();
};

void TestActiveNavigationProjection::unavailableSourceProjectsUnavailable()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::None, {}, {}, false);

    compareUnavailable(snapshot);
}

void TestActiveNavigationProjection::unknownMediaStateProjectsAvailableButUnknown()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, {}, {}, false);

    compareUnknown(snapshot);
}

void TestActiveNavigationProjection::
    validDirectMediaNavigationBoundaryStateProjectsReadoutAndActions()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::DirectMediaActiveNavigationInput {
            KiriView::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
        {}, false);

    QVERIFY(snapshot.available);
    QVERIFY(snapshot.known);
    QVERIFY(snapshot.editable);
    QVERIFY(snapshot.canOpenPrevious);
    QVERIFY(snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(!snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 2);
    QCOMPARE(snapshot.count, 4);
}

void TestActiveNavigationProjection::invalidMediaNumbersNormalizeToUnknown()
{
    const KiriView::ActiveNavigationSnapshot zeroCurrent = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::DirectMediaActiveNavigationInput {
            KiriView::DirectMediaNavigationBoundaryState { true, true, false, false, 0, 4 }, true },
        {}, false);
    compareUnknown(zeroCurrent);

    const KiriView::ActiveNavigationSnapshot outOfRange = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::DirectMediaActiveNavigationInput {
            KiriView::DirectMediaNavigationBoundaryState { true, false, false, true, 5, 4 }, true },
        {}, false);
    compareUnknown(outOfRange);
}

void TestActiveNavigationProjection::validImageDocumentSnapshotProjectsSpreadAwareBoundaries()
{
    const KiriView::ActiveNavigationSnapshot firstSpread = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(false, true, true, false, 1, 5), false);
    QVERIFY(firstSpread.available);
    QVERIFY(firstSpread.known);
    QVERIFY(firstSpread.editable);
    QVERIFY(!firstSpread.canOpenPrevious);
    QVERIFY(firstSpread.canOpenNext);
    QVERIFY(firstSpread.atKnownFirst);
    QVERIFY(!firstSpread.atKnownLast);
    QCOMPARE(firstSpread.currentNumber, 1);
    QCOMPARE(firstSpread.count, 5);

    const KiriView::ActiveNavigationSnapshot lastSpread = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(true, false, false, true, 4, 5), false);
    QVERIFY(lastSpread.canOpenPrevious);
    QVERIFY(!lastSpread.canOpenNext);
    QVERIFY(!lastSpread.atKnownFirst);
    QVERIFY(lastSpread.atKnownLast);
    QCOMPARE(lastSpread.currentNumber, 4);
    QCOMPARE(lastSpread.count, 5);
}

void TestActiveNavigationProjection::imageDocumentProjectionPreservesLeafOwnedSnapshotBoundaries()
{
    KiriView::ImageDocumentPageActiveNavigationSnapshot leafSnapshot;
    leafSnapshot.known = true;
    leafSnapshot.canOpenPrevious = false;
    leafSnapshot.canOpenNext = false;
    leafSnapshot.atKnownFirst = false;
    leafSnapshot.atKnownLast = false;
    leafSnapshot.currentNumber = 2;
    leafSnapshot.count = 5;

    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {}, leafSnapshot, false);

    QVERIFY(snapshot.available);
    QVERIFY(snapshot.known);
    QVERIFY(snapshot.editable);
    QVERIFY(!snapshot.canOpenPrevious);
    QVERIFY(!snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(!snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 2);
    QCOMPARE(snapshot.count, 5);
}

void TestActiveNavigationProjection::invalidImagePageValuesNormalizeToUnknown()
{
    const KiriView::ActiveNavigationSnapshot zeroCurrent = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(false, true, true, false, 0, 5), false);
    compareUnknown(zeroCurrent);

    const KiriView::ActiveNavigationSnapshot outOfRange = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(true, false, false, true, 6, 5), false);
    compareUnknown(outOfRange);
}

void TestActiveNavigationProjection::deletionMaskingDisablesDispatchButKeepsKnownReadout()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::DirectMediaActiveNavigationInput {
            KiriView::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
        {}, true);

    QVERIFY(snapshot.available);
    QVERIFY(snapshot.known);
    QVERIFY(!snapshot.editable);
    QVERIFY(!snapshot.canOpenPrevious);
    QVERIFY(!snapshot.canOpenNext);
    QVERIFY(!snapshot.atKnownFirst);
    QVERIFY(!snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 2);
    QCOMPARE(snapshot.count, 4);
}

void TestActiveNavigationProjection::boundaryScopeMapsSourceKind()
{
    QVERIFY(
        KiriView::activeNavigationBoundaryScopeForSource(KiriView::ActiveNavigationSourceKind::None)
        == KiriView::ActiveNavigationBoundaryScope::None);
    QVERIFY(KiriView::activeNavigationBoundaryScopeForSource(
                KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia)
        == KiriView::ActiveNavigationBoundaryScope::DirectMedia);
    QVERIFY(KiriView::activeNavigationBoundaryScopeForSource(
                KiriView::ActiveNavigationSourceKind::ImageDocumentPages)
        == KiriView::ActiveNavigationBoundaryScope::ImageDocumentPage);
}

void TestActiveNavigationProjection::directMediaDispatchPlanFollowsProjectedBoundaryGates()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::DirectMediaActiveNavigationInput {
            KiriView::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
        {}, false);

    const KiriView::ActiveNavigationDispatchPlan previous = KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, snapshot,
        KiriView::previousActiveNavigationDispatchRequest());
    QVERIFY(previous.shouldDispatch());
    QCOMPARE(previous.outcome, KiriView::ActiveNavigationDispatchOutcome::Dispatch);
    QVERIFY(dispatchOperation<KiriView::OpenPreviousDirectMediaNavigationOperation>(previous)
        != nullptr);

    const KiriView::ActiveNavigationDispatchPlan next = KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, snapshot,
        KiriView::nextActiveNavigationDispatchRequest());
    QVERIFY(next.shouldDispatch());
    QCOMPARE(next.outcome, KiriView::ActiveNavigationDispatchOutcome::Dispatch);
    QVERIFY(dispatchOperation<KiriView::OpenNextDirectMediaNavigationOperation>(next) != nullptr);

    const KiriView::ActiveNavigationDispatchPlan numbered = KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, snapshot,
        KiriView::numberedActiveNavigationDispatchRequest(3));
    QVERIFY(numbered.shouldDispatch());
    QCOMPARE(numbered.outcome, KiriView::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *numberedOperation
        = dispatchOperation<KiriView::OpenDirectMediaNavigationAtNumberOperation>(numbered);
    QVERIFY(numberedOperation != nullptr);
    QCOMPARE(numberedOperation->number, 3);

    const KiriView::ActiveNavigationSnapshot firstSnapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::DirectMediaActiveNavigationInput {
            KiriView::DirectMediaNavigationBoundaryState { false, true, true, false, 1, 4 }, true },
        {}, false);
    QVERIFY(!KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, firstSnapshot,
        KiriView::previousActiveNavigationDispatchRequest())
            .shouldDispatch());
}

void TestActiveNavigationProjection::imageDocumentDispatchPlanUsesNumberedPageTargets()
{
    const KiriView::ActiveNavigationSnapshot snapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(true, true, false, false, 2, 5), false);

    const KiriView::ActiveNavigationDispatchPlan first = KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, snapshot,
        KiriView::firstActiveNavigationDispatchRequest());
    QVERIFY(first.shouldDispatch());
    QCOMPARE(first.outcome, KiriView::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *firstOperation
        = dispatchOperation<KiriView::OpenImageDocumentPageAtNumberOperation>(first);
    QVERIFY(firstOperation != nullptr);
    QCOMPARE(firstOperation->number, 1);

    const KiriView::ActiveNavigationDispatchPlan last = KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, snapshot,
        KiriView::lastActiveNavigationDispatchRequest());
    QVERIFY(last.shouldDispatch());
    QCOMPARE(last.outcome, KiriView::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *lastOperation
        = dispatchOperation<KiriView::OpenImageDocumentPageAtNumberOperation>(last);
    QVERIFY(lastOperation != nullptr);
    QCOMPARE(lastOperation->number, 5);

    const KiriView::ActiveNavigationDispatchPlan numbered = KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, snapshot,
        KiriView::numberedActiveNavigationDispatchRequest(4));
    QVERIFY(numbered.shouldDispatch());
    QCOMPARE(numbered.outcome, KiriView::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *numberedOperation
        = dispatchOperation<KiriView::OpenImageDocumentPageAtNumberOperation>(numbered);
    QVERIFY(numberedOperation != nullptr);
    QCOMPARE(numberedOperation->number, 4);
}

void TestActiveNavigationProjection::previousNextDispatchPlanReportsEditableBoundaries()
{
    const KiriView::ActiveNavigationSnapshot firstSnapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(false, true, true, false, 1, 3), false);
    const KiriView::ActiveNavigationDispatchPlan previous = KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, firstSnapshot,
        KiriView::previousActiveNavigationDispatchRequest());
    QVERIFY(!previous.shouldDispatch());
    QCOMPARE(previous.outcome, KiriView::ActiveNavigationDispatchOutcome::FirstBoundary);

    const KiriView::ActiveNavigationSnapshot lastSnapshot = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(true, false, false, true, 3, 3), false);
    const KiriView::ActiveNavigationDispatchPlan next = KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::ImageDocumentPages, lastSnapshot,
        KiriView::nextActiveNavigationDispatchRequest());
    QVERIFY(!next.shouldDispatch());
    QCOMPARE(next.outcome, KiriView::ActiveNavigationDispatchOutcome::LastBoundary);
}

void TestActiveNavigationProjection::dispatchPlanRejectsUnknownMaskedAndUnavailableNavigation()
{
    const KiriView::ActiveNavigationSnapshot unknown = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, {}, {}, false);
    QVERIFY(!KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, unknown,
        KiriView::numberedActiveNavigationDispatchRequest(1))
            .shouldDispatch());

    const KiriView::ActiveNavigationSnapshot masked = KiriView::projectActiveNavigation(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        KiriView::DirectMediaActiveNavigationInput {
            KiriView::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
        {}, true);
    QVERIFY(!KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, masked,
        KiriView::previousActiveNavigationDispatchRequest())
            .shouldDispatch());
    QCOMPARE(KiriView::activeNavigationDispatchPlan(
                 KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, masked,
                 KiriView::previousActiveNavigationDispatchRequest())
                 .outcome,
        KiriView::ActiveNavigationDispatchOutcome::NoOp);
    QVERIFY(!KiriView::activeNavigationDispatchPlan(
        KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia, masked,
        KiriView::numberedActiveNavigationDispatchRequest(1))
            .shouldDispatch());

    KiriView::ActiveNavigationSnapshot unavailable;
    unavailable.canOpenPrevious = true;
    QVERIFY(!KiriView::activeNavigationDispatchPlan(KiriView::ActiveNavigationSourceKind::None,
        unavailable, KiriView::previousActiveNavigationDispatchRequest())
            .shouldDispatch());
}

QTEST_GUILESS_MAIN(TestActiveNavigationProjection)

#include "test_activenavigationprojection.moc"
