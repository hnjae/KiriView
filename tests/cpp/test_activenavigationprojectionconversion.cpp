// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/activenavigationprojectionconversion.h"

#include <QObject>
#include <QTest>
#include <variant>

namespace {
template <typename Operation>
const Operation *dispatchOperation(const kiriview::ActiveNavigationDispatchPlan &plan)
{
    return std::get_if<Operation>(&plan.operation);
}
}

class TestActiveNavigationProjectionConversion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sourceKindAndBoundaryScopeConvert();
    void snapshotsConvertAcrossBridge();
    void dispatchRequestAndPlanConvertAcrossBridge();
};

void TestActiveNavigationProjectionConversion::sourceKindAndBoundaryScopeConvert()
{
    QVERIFY(
        kiriview::Bridge::rustActiveNavigationSourceKind(kiriview::ActiveNavigationSourceKind::None)
        == kiriview::RustActiveNavigationSourceKind::NoSource);
    QVERIFY(kiriview::Bridge::rustActiveNavigationSourceKind(
                kiriview::ActiveNavigationSourceKind::OrdinaryDirectMedia)
        == kiriview::RustActiveNavigationSourceKind::OrdinaryDirectMedia);
    QVERIFY(kiriview::Bridge::rustActiveNavigationSourceKind(
                kiriview::ActiveNavigationSourceKind::ImageDocumentPages)
        == kiriview::RustActiveNavigationSourceKind::ImageDocumentPages);

    QVERIFY(kiriview::Bridge::activeNavigationBoundaryScopeFromRust(
                kiriview::RustActiveNavigationBoundaryScope::NoBoundary)
        == kiriview::ActiveNavigationBoundaryScope::None);
    QVERIFY(kiriview::Bridge::activeNavigationBoundaryScopeFromRust(
                kiriview::RustActiveNavigationBoundaryScope::DirectMedia)
        == kiriview::ActiveNavigationBoundaryScope::DirectMedia);
    QVERIFY(kiriview::Bridge::activeNavigationBoundaryScopeFromRust(
                kiriview::RustActiveNavigationBoundaryScope::ImageDocumentPage)
        == kiriview::ActiveNavigationBoundaryScope::ImageDocumentPage);
}

void TestActiveNavigationProjectionConversion::snapshotsConvertAcrossBridge()
{
    const kiriview::DirectMediaActiveNavigationInput directMediaInput {
        kiriview::DirectMediaNavigationBoundaryState { true, false, false, true, 4, 4 },
        true,
    };
    const kiriview::RustDirectMediaActiveNavigationInput rustDirectMediaInput
        = kiriview::Bridge::rustDirectMediaActiveNavigationInput(directMediaInput);
    QVERIFY(rustDirectMediaInput.known);
    QVERIFY(rustDirectMediaInput.boundary_state.can_open_previous);
    QVERIFY(!rustDirectMediaInput.boundary_state.can_open_next);
    QVERIFY(rustDirectMediaInput.boundary_state.at_known_last);
    QCOMPARE(rustDirectMediaInput.boundary_state.current_number, 4);
    QCOMPARE(rustDirectMediaInput.boundary_state.count, 4);

    const kiriview::ImageDocumentPageActiveNavigationSnapshot imagePageSnapshot { true, true, false,
        false, true, 7, 7 };
    const kiriview::RustImageDocumentPageActiveNavigationSnapshot rustImagePageSnapshot
        = kiriview::Bridge::rustImageDocumentPageActiveNavigationSnapshot(imagePageSnapshot);
    QVERIFY(rustImagePageSnapshot.known);
    QVERIFY(rustImagePageSnapshot.can_open_previous);
    QVERIFY(!rustImagePageSnapshot.can_open_next);
    QVERIFY(rustImagePageSnapshot.at_known_last);
    QCOMPARE(rustImagePageSnapshot.current_number, 7);
    QCOMPARE(rustImagePageSnapshot.count, 7);

    kiriview::RustActiveNavigationSnapshot rustSnapshot {};
    rustSnapshot.available = true;
    rustSnapshot.known = true;
    rustSnapshot.editable = true;
    rustSnapshot.can_open_previous = true;
    rustSnapshot.can_open_next = false;
    rustSnapshot.at_known_first = false;
    rustSnapshot.at_known_last = true;
    rustSnapshot.current_number = 3;
    rustSnapshot.count = 3;

    const kiriview::ActiveNavigationSnapshot snapshot
        = kiriview::Bridge::activeNavigationSnapshotFromRust(rustSnapshot);
    QVERIFY(snapshot.available);
    QVERIFY(snapshot.known);
    QVERIFY(snapshot.editable);
    QVERIFY(snapshot.canOpenPrevious);
    QVERIFY(!snapshot.canOpenNext);
    QVERIFY(snapshot.atKnownLast);
    QCOMPARE(snapshot.currentNumber, 3);
    QCOMPARE(snapshot.count, 3);
}

void TestActiveNavigationProjectionConversion::dispatchRequestAndPlanConvertAcrossBridge()
{
    const kiriview::RustActiveNavigationDispatchRequest request
        = kiriview::Bridge::rustActiveNavigationDispatchRequest(
            kiriview::numberedActiveNavigationDispatchRequest(12));
    QVERIFY(request.kind == kiriview::RustActiveNavigationDispatchRequestKind::Number);
    QCOMPARE(request.number, 12);

    kiriview::RustActiveNavigationDispatchPlan rustPlan {};
    rustPlan.operation_kind
        = kiriview::RustActiveNavigationDispatchOperationKind::OpenDirectMediaAtNumber;
    rustPlan.operation_number = 5;
    rustPlan.outcome = kiriview::RustActiveNavigationDispatchOutcome::Dispatch;

    const kiriview::ActiveNavigationDispatchPlan plan
        = kiriview::Bridge::activeNavigationDispatchPlanFromRust(rustPlan);
    QVERIFY(plan.shouldDispatch());
    QCOMPARE(plan.outcome, kiriview::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *operation
        = dispatchOperation<kiriview::OpenDirectMediaNavigationAtNumberOperation>(plan);
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->number, 5);
}

QTEST_GUILESS_MAIN(TestActiveNavigationProjectionConversion)

#include "test_activenavigationprojectionconversion.moc"
