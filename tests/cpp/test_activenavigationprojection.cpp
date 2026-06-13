// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "session/activenavigationprojection.h"

#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QObject>
#include <QTest>
#include <variant>

namespace {
void compareUnavailable(const kiriview::ActiveNavigationSnapshot &snapshot)
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

void compareUnknown(const kiriview::ActiveNavigationSnapshot &snapshot)
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
const Operation *dispatchOperation(const kiriview::ActiveNavigationDispatchPlan &plan)
{
    return std::get_if<Operation>(&plan.operation);
}

kiriview::ImageDocumentPageActiveNavigationSnapshot imageDocumentActiveNavigationSnapshot(
    bool canOpenPrevious, bool canOpenNext, bool atKnownFirst, bool atKnownLast, int currentNumber,
    int count)
{
    return kiriview::ImageDocumentPageActiveNavigationSnapshot {
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
    void numberedDispatchPlanClampsToKnownRange();
    void previousNextDispatchPlanReportsEditableBoundaries();
    void dispatchPlanRejectsUnknownMaskedAndUnavailableNavigation();
};

void TestActiveNavigationProjection::unavailableSourceProjectsUnavailable()
{
    const kiriview::ActiveNavigationSnapshot snapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::None, {}, {}, false);

    compareUnavailable(snapshot);
}

void TestActiveNavigationProjection::unknownMediaStateProjectsAvailableButUnknown()
{
    const kiriview::ActiveNavigationSnapshot snapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, {}, {}, false);

    compareUnknown(snapshot);
}

void TestActiveNavigationProjection::
    validDirectMediaNavigationBoundaryStateProjectsReadoutAndActions()
{
    const kiriview::ActiveNavigationSnapshot snapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        kiriview::DirectMediaActiveNavigationInput {
            kiriview::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
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
    const kiriview::ActiveNavigationSnapshot zeroCurrent = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        kiriview::DirectMediaActiveNavigationInput {
            kiriview::DirectMediaNavigationBoundaryState { true, true, false, false, 0, 4 }, true },
        {}, false);
    compareUnknown(zeroCurrent);

    const kiriview::ActiveNavigationSnapshot outOfRange = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        kiriview::DirectMediaActiveNavigationInput {
            kiriview::DirectMediaNavigationBoundaryState { true, false, false, true, 5, 4 }, true },
        {}, false);
    compareUnknown(outOfRange);
}

void TestActiveNavigationProjection::validImageDocumentSnapshotProjectsSpreadAwareBoundaries()
{
    const kiriview::ActiveNavigationSnapshot firstSpread = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, {},
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

    const kiriview::ActiveNavigationSnapshot lastSpread = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, {},
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
    kiriview::ImageDocumentPageActiveNavigationSnapshot leafSnapshot;
    leafSnapshot.known = true;
    leafSnapshot.canOpenPrevious = false;
    leafSnapshot.canOpenNext = false;
    leafSnapshot.atKnownFirst = false;
    leafSnapshot.atKnownLast = false;
    leafSnapshot.currentNumber = 2;
    leafSnapshot.count = 5;

    const kiriview::ActiveNavigationSnapshot snapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, {}, leafSnapshot, false);

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
    const kiriview::ActiveNavigationSnapshot zeroCurrent = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(false, true, true, false, 0, 5), false);
    compareUnknown(zeroCurrent);

    const kiriview::ActiveNavigationSnapshot outOfRange = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(true, false, false, true, 6, 5), false);
    compareUnknown(outOfRange);
}

void TestActiveNavigationProjection::deletionMaskingDisablesDispatchButKeepsKnownReadout()
{
    const kiriview::ActiveNavigationSnapshot snapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        kiriview::DirectMediaActiveNavigationInput {
            kiriview::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
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
        kiriview::activeNavigationBoundaryScopeForSource(kiriview::ActiveNavigationSourceKind::None)
        == kiriview::ActiveNavigationBoundaryScope::None);
    QVERIFY(kiriview::activeNavigationBoundaryScopeForSource(
                kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia)
        == kiriview::ActiveNavigationBoundaryScope::DirectMedia);
    QVERIFY(kiriview::activeNavigationBoundaryScopeForSource(
                kiriview::ActiveNavigationSourceKind::ImageDocumentPages)
        == kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage);
}

void TestActiveNavigationProjection::directMediaDispatchPlanFollowsProjectedBoundaryGates()
{
    const kiriview::ActiveNavigationSnapshot snapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        kiriview::DirectMediaActiveNavigationInput {
            kiriview::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
        {}, false);

    const kiriview::ActiveNavigationDispatchPlan previous = kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, snapshot,
        kiriview::previousActiveNavigationDispatchRequest());
    QVERIFY(previous.shouldDispatch());
    QCOMPARE(previous.outcome, kiriview::ActiveNavigationDispatchOutcome::Dispatch);
    QVERIFY(dispatchOperation<kiriview::OpenPreviousDirectMediaNavigationOperation>(previous)
        != nullptr);

    const kiriview::ActiveNavigationDispatchPlan next = kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, snapshot,
        kiriview::nextActiveNavigationDispatchRequest());
    QVERIFY(next.shouldDispatch());
    QCOMPARE(next.outcome, kiriview::ActiveNavigationDispatchOutcome::Dispatch);
    QVERIFY(dispatchOperation<kiriview::OpenNextDirectMediaNavigationOperation>(next) != nullptr);

    const kiriview::ActiveNavigationDispatchPlan numbered = kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, snapshot,
        kiriview::numberedActiveNavigationDispatchRequest(3));
    QVERIFY(numbered.shouldDispatch());
    QCOMPARE(numbered.outcome, kiriview::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *numberedOperation
        = dispatchOperation<kiriview::OpenDirectMediaNavigationAtNumberOperation>(numbered);
    QVERIFY(numberedOperation != nullptr);
    QCOMPARE(numberedOperation->number, 3);

    const kiriview::ActiveNavigationSnapshot firstSnapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        kiriview::DirectMediaActiveNavigationInput {
            kiriview::DirectMediaNavigationBoundaryState { false, true, true, false, 1, 4 }, true },
        {}, false);
    QVERIFY(!kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, firstSnapshot,
        kiriview::previousActiveNavigationDispatchRequest())
            .shouldDispatch());
}

void TestActiveNavigationProjection::imageDocumentDispatchPlanUsesNumberedPageTargets()
{
    const kiriview::ActiveNavigationSnapshot snapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(true, true, false, false, 2, 5), false);

    const kiriview::ActiveNavigationDispatchPlan first = kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, snapshot,
        kiriview::firstActiveNavigationDispatchRequest());
    QVERIFY(first.shouldDispatch());
    QCOMPARE(first.outcome, kiriview::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *firstOperation
        = dispatchOperation<kiriview::OpenImageDocumentPageAtNumberOperation>(first);
    QVERIFY(firstOperation != nullptr);
    QCOMPARE(firstOperation->number, 1);

    const kiriview::ActiveNavigationDispatchPlan last = kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, snapshot,
        kiriview::lastActiveNavigationDispatchRequest());
    QVERIFY(last.shouldDispatch());
    QCOMPARE(last.outcome, kiriview::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *lastOperation
        = dispatchOperation<kiriview::OpenImageDocumentPageAtNumberOperation>(last);
    QVERIFY(lastOperation != nullptr);
    QCOMPARE(lastOperation->number, 5);

    const kiriview::ActiveNavigationDispatchPlan numbered = kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, snapshot,
        kiriview::numberedActiveNavigationDispatchRequest(4));
    QVERIFY(numbered.shouldDispatch());
    QCOMPARE(numbered.outcome, kiriview::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *numberedOperation
        = dispatchOperation<kiriview::OpenImageDocumentPageAtNumberOperation>(numbered);
    QVERIFY(numberedOperation != nullptr);
    QCOMPARE(numberedOperation->number, 4);
}

void TestActiveNavigationProjection::numberedDispatchPlanClampsToKnownRange()
{
    const kiriview::ActiveNavigationSnapshot directMedia = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        kiriview::DirectMediaActiveNavigationInput {
            kiriview::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
        {}, false);

    const kiriview::ActiveNavigationDispatchPlan directMediaBelowRange
        = kiriview::activeNavigationDispatchPlan(
            kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, directMedia,
            kiriview::numberedActiveNavigationDispatchRequest(0));
    QVERIFY(directMediaBelowRange.shouldDispatch());
    const auto *directMediaOperation
        = dispatchOperation<kiriview::OpenDirectMediaNavigationAtNumberOperation>(
            directMediaBelowRange);
    QVERIFY(directMediaOperation != nullptr);
    QCOMPARE(directMediaOperation->number, 1);

    const kiriview::ActiveNavigationSnapshot imageDocument = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(true, true, false, false, 2, 5), false);

    const kiriview::ActiveNavigationDispatchPlan imageDocumentAboveRange
        = kiriview::activeNavigationDispatchPlan(
            kiriview::ActiveNavigationSourceKind::ImageDocumentPages, imageDocument,
            kiriview::numberedActiveNavigationDispatchRequest(8));
    QVERIFY(imageDocumentAboveRange.shouldDispatch());
    const auto *imageDocumentOperation
        = dispatchOperation<kiriview::OpenImageDocumentPageAtNumberOperation>(
            imageDocumentAboveRange);
    QVERIFY(imageDocumentOperation != nullptr);
    QCOMPARE(imageDocumentOperation->number, 5);
}

void TestActiveNavigationProjection::previousNextDispatchPlanReportsEditableBoundaries()
{
    const kiriview::ActiveNavigationSnapshot firstSnapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(false, true, true, false, 1, 3), false);
    const kiriview::ActiveNavigationDispatchPlan previous = kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, firstSnapshot,
        kiriview::previousActiveNavigationDispatchRequest());
    QVERIFY(!previous.shouldDispatch());
    QCOMPARE(previous.outcome, kiriview::ActiveNavigationDispatchOutcome::FirstBoundary);

    const kiriview::ActiveNavigationSnapshot lastSnapshot = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, {},
        imageDocumentActiveNavigationSnapshot(true, false, false, true, 3, 3), false);
    const kiriview::ActiveNavigationDispatchPlan next = kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::ImageDocumentPages, lastSnapshot,
        kiriview::nextActiveNavigationDispatchRequest());
    QVERIFY(!next.shouldDispatch());
    QCOMPARE(next.outcome, kiriview::ActiveNavigationDispatchOutcome::LastBoundary);
}

void TestActiveNavigationProjection::dispatchPlanRejectsUnknownMaskedAndUnavailableNavigation()
{
    const kiriview::ActiveNavigationSnapshot unknown = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, {}, {}, false);
    QVERIFY(!kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, unknown,
        kiriview::numberedActiveNavigationDispatchRequest(1))
            .shouldDispatch());

    const kiriview::ActiveNavigationSnapshot masked = kiriview::projectActiveNavigation(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia,
        kiriview::DirectMediaActiveNavigationInput {
            kiriview::DirectMediaNavigationBoundaryState { true, true, false, false, 2, 4 }, true },
        {}, true);
    QVERIFY(!kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, masked,
        kiriview::previousActiveNavigationDispatchRequest())
            .shouldDispatch());
    QCOMPARE(kiriview::activeNavigationDispatchPlan(
                 kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, masked,
                 kiriview::previousActiveNavigationDispatchRequest())
                 .outcome,
        kiriview::ActiveNavigationDispatchOutcome::NoOp);
    QVERIFY(!kiriview::activeNavigationDispatchPlan(
        kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia, masked,
        kiriview::numberedActiveNavigationDispatchRequest(1))
            .shouldDispatch());

    kiriview::ActiveNavigationSnapshot unavailable;
    unavailable.canOpenPrevious = true;
    QVERIFY(!kiriview::activeNavigationDispatchPlan(kiriview::ActiveNavigationSourceKind::None,
        unavailable, kiriview::previousActiveNavigationDispatchRequest())
            .shouldDispatch());
}

QTEST_GUILESS_MAIN(TestActiveNavigationProjection)

#include "test_activenavigationprojection.moc"
