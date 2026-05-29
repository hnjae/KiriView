// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/activenavigationprojectionconversion.h"

namespace {
KiriView::ActiveNavigationDispatchOperation activeNavigationDispatchOperationFromRust(
    KiriView::RustActiveNavigationDispatchOperationKind kind, int number)
{
    switch (kind) {
    case KiriView::RustActiveNavigationDispatchOperationKind::NoOperation:
        return std::monostate {};
    case KiriView::RustActiveNavigationDispatchOperationKind::OpenPreviousDirectMedia:
        return KiriView::OpenPreviousDirectMediaNavigationOperation {};
    case KiriView::RustActiveNavigationDispatchOperationKind::OpenNextDirectMedia:
        return KiriView::OpenNextDirectMediaNavigationOperation {};
    case KiriView::RustActiveNavigationDispatchOperationKind::OpenDirectMediaAtNumber:
        return KiriView::OpenDirectMediaNavigationAtNumberOperation { number };
    case KiriView::RustActiveNavigationDispatchOperationKind::OpenPreviousImageDocumentPage:
        return KiriView::OpenPreviousImageDocumentPageOperation {};
    case KiriView::RustActiveNavigationDispatchOperationKind::OpenNextImageDocumentPage:
        return KiriView::OpenNextImageDocumentPageOperation {};
    case KiriView::RustActiveNavigationDispatchOperationKind::OpenImageDocumentPageAtNumber:
        return KiriView::OpenImageDocumentPageAtNumberOperation { number };
    }

    return std::monostate {};
}

KiriView::ActiveNavigationDispatchOutcome activeNavigationDispatchOutcomeFromRust(
    KiriView::RustActiveNavigationDispatchOutcome outcome)
{
    switch (outcome) {
    case KiriView::RustActiveNavigationDispatchOutcome::NoOp:
        return KiriView::ActiveNavigationDispatchOutcome::NoOp;
    case KiriView::RustActiveNavigationDispatchOutcome::Dispatch:
        return KiriView::ActiveNavigationDispatchOutcome::Dispatch;
    case KiriView::RustActiveNavigationDispatchOutcome::FirstBoundary:
        return KiriView::ActiveNavigationDispatchOutcome::FirstBoundary;
    case KiriView::RustActiveNavigationDispatchOutcome::LastBoundary:
        return KiriView::ActiveNavigationDispatchOutcome::LastBoundary;
    }

    return KiriView::ActiveNavigationDispatchOutcome::NoOp;
}
}

namespace KiriView::Bridge {
RustActiveNavigationSourceKind rustActiveNavigationSourceKind(ActiveNavigationSourceKind sourceKind)
{
    switch (sourceKind) {
    case ActiveNavigationSourceKind::None:
        return RustActiveNavigationSourceKind::NoSource;
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        return RustActiveNavigationSourceKind::OrdinaryDirectMedia;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        return RustActiveNavigationSourceKind::ImageDocumentPages;
    }

    return RustActiveNavigationSourceKind::NoSource;
}

RustDirectMediaNavigationBoundaryState rustDirectMediaNavigationBoundaryState(
    const DirectMediaNavigationBoundaryState &state)
{
    return RustDirectMediaNavigationBoundaryState { state.canOpenPrevious, state.canOpenNext,
        state.atKnownFirst, state.atKnownLast, state.currentNumber, state.count };
}

RustDirectMediaActiveNavigationInput rustDirectMediaActiveNavigationInput(
    DirectMediaActiveNavigationInput input)
{
    return RustDirectMediaActiveNavigationInput {
        rustDirectMediaNavigationBoundaryState(input.boundaryState),
        input.known,
    };
}

RustImageDocumentPageActiveNavigationSnapshot rustImageDocumentPageActiveNavigationSnapshot(
    ImageDocumentPageActiveNavigationSnapshot snapshot)
{
    return RustImageDocumentPageActiveNavigationSnapshot { snapshot.known, snapshot.canOpenPrevious,
        snapshot.canOpenNext, snapshot.atKnownFirst, snapshot.atKnownLast, snapshot.currentNumber,
        snapshot.count };
}

RustActiveNavigationSnapshot rustActiveNavigationSnapshot(ActiveNavigationSnapshot snapshot)
{
    return RustActiveNavigationSnapshot { snapshot.available, snapshot.known, snapshot.editable,
        snapshot.canOpenPrevious, snapshot.canOpenNext, snapshot.atKnownFirst, snapshot.atKnownLast,
        snapshot.currentNumber, snapshot.count };
}

RustActiveNavigationDispatchRequest rustActiveNavigationDispatchRequest(
    ActiveNavigationDispatchRequest request)
{
    RustActiveNavigationDispatchRequest rustRequest {};
    rustRequest.number = request.number;
    switch (request.kind) {
    case ActiveNavigationDispatchRequestKind::Previous:
        rustRequest.kind = RustActiveNavigationDispatchRequestKind::Previous;
        break;
    case ActiveNavigationDispatchRequestKind::Next:
        rustRequest.kind = RustActiveNavigationDispatchRequestKind::Next;
        break;
    case ActiveNavigationDispatchRequestKind::First:
        rustRequest.kind = RustActiveNavigationDispatchRequestKind::First;
        break;
    case ActiveNavigationDispatchRequestKind::Last:
        rustRequest.kind = RustActiveNavigationDispatchRequestKind::Last;
        break;
    case ActiveNavigationDispatchRequestKind::Number:
        rustRequest.kind = RustActiveNavigationDispatchRequestKind::Number;
        break;
    }
    return rustRequest;
}

ActiveNavigationBoundaryScope activeNavigationBoundaryScopeFromRust(
    RustActiveNavigationBoundaryScope scope)
{
    switch (scope) {
    case RustActiveNavigationBoundaryScope::NoBoundary:
        return ActiveNavigationBoundaryScope::None;
    case RustActiveNavigationBoundaryScope::DirectMedia:
        return ActiveNavigationBoundaryScope::DirectMedia;
    case RustActiveNavigationBoundaryScope::ImageDocumentPage:
        return ActiveNavigationBoundaryScope::ImageDocumentPage;
    }

    return ActiveNavigationBoundaryScope::None;
}

ActiveNavigationSnapshot activeNavigationSnapshotFromRust(
    const RustActiveNavigationSnapshot &snapshot)
{
    return ActiveNavigationSnapshot { snapshot.available, snapshot.known, snapshot.editable,
        snapshot.can_open_previous, snapshot.can_open_next, snapshot.at_known_first,
        snapshot.at_known_last, snapshot.current_number, snapshot.count };
}

ActiveNavigationDispatchPlan activeNavigationDispatchPlanFromRust(
    const RustActiveNavigationDispatchPlan &plan)
{
    return ActiveNavigationDispatchPlan {
        activeNavigationDispatchOperationFromRust(plan.operation_kind, plan.operation_number),
        activeNavigationDispatchOutcomeFromRust(plan.outcome),
    };
}
}
