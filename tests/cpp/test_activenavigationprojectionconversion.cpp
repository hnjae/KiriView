// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/activenavigationprojectionconversion.h"

#include <QObject>
#include <QTest>
#include <variant>

namespace {
template <typename Operation>
const Operation *dispatchOperation(const KiriView::ActiveNavigationDispatchPlan &plan)
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
        KiriView::Bridge::rustActiveNavigationSourceKind(KiriView::ActiveNavigationSourceKind::None)
        == KiriView::RustActiveNavigationSourceKind::NoSource);
    QVERIFY(KiriView::Bridge::rustActiveNavigationSourceKind(
                KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia)
        == KiriView::RustActiveNavigationSourceKind::OrdinaryDirectMedia);
    QVERIFY(KiriView::Bridge::rustActiveNavigationSourceKind(
                KiriView::ActiveNavigationSourceKind::ImageDocumentPages)
        == KiriView::RustActiveNavigationSourceKind::ImageDocumentPages);

    QVERIFY(KiriView::Bridge::activeNavigationBoundaryScopeFromRust(
                KiriView::RustActiveNavigationBoundaryScope::NoBoundary)
        == KiriView::ActiveNavigationBoundaryScope::None);
    QVERIFY(KiriView::Bridge::activeNavigationBoundaryScopeFromRust(
                KiriView::RustActiveNavigationBoundaryScope::DirectMedia)
        == KiriView::ActiveNavigationBoundaryScope::DirectMedia);
    QVERIFY(KiriView::Bridge::activeNavigationBoundaryScopeFromRust(
                KiriView::RustActiveNavigationBoundaryScope::ImageDocumentPage)
        == KiriView::ActiveNavigationBoundaryScope::ImageDocumentPage);
}

void TestActiveNavigationProjectionConversion::snapshotsConvertAcrossBridge()
{
    const KiriView::DirectMediaActiveNavigationInput directMediaInput {
        KiriView::DirectMediaNavigationBoundaryState { true, false, false, true, 4, 4 },
        true,
    };
    const KiriView::RustDirectMediaActiveNavigationInput rustDirectMediaInput
        = KiriView::Bridge::rustDirectMediaActiveNavigationInput(directMediaInput);
    QVERIFY(rustDirectMediaInput.known);
    QVERIFY(rustDirectMediaInput.boundary_state.can_open_previous);
    QVERIFY(!rustDirectMediaInput.boundary_state.can_open_next);
    QVERIFY(rustDirectMediaInput.boundary_state.at_known_last);
    QCOMPARE(rustDirectMediaInput.boundary_state.current_number, 4);
    QCOMPARE(rustDirectMediaInput.boundary_state.count, 4);

    const KiriView::ImageDocumentPageActiveNavigationSnapshot imagePageSnapshot { true, true, false,
        false, true, 7, 7 };
    const KiriView::RustImageDocumentPageActiveNavigationSnapshot rustImagePageSnapshot
        = KiriView::Bridge::rustImageDocumentPageActiveNavigationSnapshot(imagePageSnapshot);
    QVERIFY(rustImagePageSnapshot.known);
    QVERIFY(rustImagePageSnapshot.can_open_previous);
    QVERIFY(!rustImagePageSnapshot.can_open_next);
    QVERIFY(rustImagePageSnapshot.at_known_last);
    QCOMPARE(rustImagePageSnapshot.current_number, 7);
    QCOMPARE(rustImagePageSnapshot.count, 7);

    KiriView::RustActiveNavigationSnapshot rustSnapshot {};
    rustSnapshot.available = true;
    rustSnapshot.known = true;
    rustSnapshot.editable = true;
    rustSnapshot.can_open_previous = true;
    rustSnapshot.can_open_next = false;
    rustSnapshot.at_known_first = false;
    rustSnapshot.at_known_last = true;
    rustSnapshot.current_number = 3;
    rustSnapshot.count = 3;

    const KiriView::ActiveNavigationSnapshot snapshot
        = KiriView::Bridge::activeNavigationSnapshotFromRust(rustSnapshot);
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
    const KiriView::RustActiveNavigationDispatchRequest request
        = KiriView::Bridge::rustActiveNavigationDispatchRequest(
            KiriView::numberedActiveNavigationDispatchRequest(12));
    QVERIFY(request.kind == KiriView::RustActiveNavigationDispatchRequestKind::Number);
    QCOMPARE(request.number, 12);

    KiriView::RustActiveNavigationDispatchPlan rustPlan {};
    rustPlan.operation_kind
        = KiriView::RustActiveNavigationDispatchOperationKind::OpenDirectMediaAtNumber;
    rustPlan.operation_number = 5;
    rustPlan.outcome = KiriView::RustActiveNavigationDispatchOutcome::Dispatch;

    const KiriView::ActiveNavigationDispatchPlan plan
        = KiriView::Bridge::activeNavigationDispatchPlanFromRust(rustPlan);
    QVERIFY(plan.shouldDispatch());
    QCOMPARE(plan.outcome, KiriView::ActiveNavigationDispatchOutcome::Dispatch);
    const auto *operation
        = dispatchOperation<KiriView::OpenDirectMediaNavigationAtNumberOperation>(plan);
    QVERIFY(operation != nullptr);
    QCOMPARE(operation->number, 5);
}

QTEST_GUILESS_MAIN(TestActiveNavigationProjectionConversion)

#include "test_activenavigationprojectionconversion.moc"
