// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/activenavigationprojectionconversion.h"

namespace {
kiriview::ActiveNavigationDispatchOperation activeNavigationDispatchOperationFromRust(
    kiriview::RustActiveNavigationDispatchOperationKind kind, int number)
{
    switch (kind) {
    case kiriview::RustActiveNavigationDispatchOperationKind::NoOperation:
        return std::monostate {};
    case kiriview::RustActiveNavigationDispatchOperationKind::OpenPreviousDirectMedia:
        return kiriview::OpenPreviousDirectMediaNavigationOperation {};
    case kiriview::RustActiveNavigationDispatchOperationKind::OpenNextDirectMedia:
        return kiriview::OpenNextDirectMediaNavigationOperation {};
    case kiriview::RustActiveNavigationDispatchOperationKind::OpenDirectMediaAtNumber:
        return kiriview::OpenDirectMediaNavigationAtNumberOperation { number };
    case kiriview::RustActiveNavigationDispatchOperationKind::OpenPreviousImageDocumentPage:
        return kiriview::OpenPreviousImageDocumentPageOperation {};
    case kiriview::RustActiveNavigationDispatchOperationKind::OpenNextImageDocumentPage:
        return kiriview::OpenNextImageDocumentPageOperation {};
    case kiriview::RustActiveNavigationDispatchOperationKind::OpenImageDocumentPageAtNumber:
        return kiriview::OpenImageDocumentPageAtNumberOperation { number };
    }

    return std::monostate {};
}

kiriview::ActiveNavigationDispatchOutcome activeNavigationDispatchOutcomeFromRust(
    kiriview::RustActiveNavigationDispatchOutcome outcome)
{
    switch (outcome) {
    case kiriview::RustActiveNavigationDispatchOutcome::NoOp:
        return kiriview::ActiveNavigationDispatchOutcome::NoOp;
    case kiriview::RustActiveNavigationDispatchOutcome::Dispatch:
        return kiriview::ActiveNavigationDispatchOutcome::Dispatch;
    case kiriview::RustActiveNavigationDispatchOutcome::FirstBoundary:
        return kiriview::ActiveNavigationDispatchOutcome::FirstBoundary;
    case kiriview::RustActiveNavigationDispatchOutcome::LastBoundary:
        return kiriview::ActiveNavigationDispatchOutcome::LastBoundary;
    }

    return kiriview::ActiveNavigationDispatchOutcome::NoOp;
}
}

namespace kiriview::Bridge {
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
