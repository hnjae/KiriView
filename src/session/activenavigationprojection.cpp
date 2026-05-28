// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "activenavigationprojection.h"

#include <utility>

namespace {
KiriView::ActiveNavigationSnapshot unavailableActiveNavigation() { return {}; }

KiriView::ActiveNavigationSnapshot unknownActiveNavigation()
{
    return KiriView::ActiveNavigationSnapshot { true };
}

KiriView::ActiveNavigationSnapshot normalizedActiveNavigation(
    KiriView::ActiveNavigationSnapshot snapshot)
{
    if (!snapshot.available || !snapshot.known || snapshot.currentNumber < 1 || snapshot.count < 1
        || snapshot.currentNumber > snapshot.count) {
        return snapshot.available ? unknownActiveNavigation() : unavailableActiveNavigation();
    }

    snapshot.known = true;
    snapshot.editable = true;
    return snapshot;
}

KiriView::ActiveNavigationSnapshot directMediaActiveNavigationSnapshot(
    KiriView::DirectMediaActiveNavigationInput input)
{
    if (!input.known) {
        return unknownActiveNavigation();
    }

    return normalizedActiveNavigation(KiriView::ActiveNavigationSnapshot {
        true,
        true,
        true,
        input.boundaryState.canOpenPrevious,
        input.boundaryState.canOpenNext,
        input.boundaryState.atKnownFirst,
        input.boundaryState.atKnownLast,
        input.boundaryState.currentNumber,
        input.boundaryState.count,
    });
}

KiriView::ActiveNavigationSnapshot imageDocumentPageActiveNavigationSnapshot(
    KiriView::ImageDocumentPageActiveNavigationSnapshot input)
{
    return normalizedActiveNavigation(KiriView::ActiveNavigationSnapshot {
        true,
        input.known,
        true,
        input.canOpenPrevious,
        input.canOpenNext,
        input.atKnownFirst,
        input.atKnownLast,
        input.currentNumber,
        input.count,
    });
}

KiriView::ActiveNavigationDispatchOperation previousOperationForSource(
    KiriView::ActiveNavigationSourceKind sourceKind)
{
    switch (sourceKind) {
    case KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia:
        return KiriView::OpenPreviousDirectMediaNavigationOperation {};
    case KiriView::ActiveNavigationSourceKind::ImageDocumentPages:
        return KiriView::OpenPreviousImageDocumentPageOperation {};
    case KiriView::ActiveNavigationSourceKind::None:
        return std::monostate {};
    }

    return std::monostate {};
}

KiriView::ActiveNavigationDispatchOperation nextOperationForSource(
    KiriView::ActiveNavigationSourceKind sourceKind)
{
    switch (sourceKind) {
    case KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia:
        return KiriView::OpenNextDirectMediaNavigationOperation {};
    case KiriView::ActiveNavigationSourceKind::ImageDocumentPages:
        return KiriView::OpenNextImageDocumentPageOperation {};
    case KiriView::ActiveNavigationSourceKind::None:
        return std::monostate {};
    }

    return std::monostate {};
}

KiriView::ActiveNavigationDispatchOperation numberedOperationForSource(
    KiriView::ActiveNavigationSourceKind sourceKind, int number)
{
    switch (sourceKind) {
    case KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia:
        return KiriView::OpenDirectMediaNavigationAtNumberOperation { number };
    case KiriView::ActiveNavigationSourceKind::ImageDocumentPages:
        return KiriView::OpenImageDocumentPageAtNumberOperation { number };
    case KiriView::ActiveNavigationSourceKind::None:
        return std::monostate {};
    }

    return std::monostate {};
}

KiriView::ActiveNavigationDispatchPlan dispatchOperation(
    KiriView::ActiveNavigationDispatchOperation operation)
{
    if (std::holds_alternative<std::monostate>(operation)) {
        return {};
    }

    return KiriView::ActiveNavigationDispatchPlan {
        std::move(operation),
        KiriView::ActiveNavigationDispatchOutcome::Dispatch,
    };
}

KiriView::ActiveNavigationDispatchPlan boundaryOutcome(
    KiriView::ActiveNavigationDispatchOutcome outcome)
{
    return KiriView::ActiveNavigationDispatchPlan {
        std::monostate {},
        outcome,
    };
}
}

namespace KiriView {
bool ActiveNavigationDispatchPlan::shouldDispatch() const
{
    return outcome == ActiveNavigationDispatchOutcome::Dispatch
        && !std::holds_alternative<std::monostate>(operation);
}

ActiveNavigationSnapshot projectActiveNavigation(ActiveNavigationSourceKind sourceKind,
    DirectMediaActiveNavigationInput directMediaInput,
    ImageDocumentPageActiveNavigationSnapshot imageDocumentPageSnapshot,
    bool fileDeletionInProgress)
{
    ActiveNavigationSnapshot snapshot;
    switch (sourceKind) {
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        snapshot = directMediaActiveNavigationSnapshot(directMediaInput);
        break;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        snapshot = imageDocumentPageActiveNavigationSnapshot(imageDocumentPageSnapshot);
        break;
    case ActiveNavigationSourceKind::None:
        snapshot = unavailableActiveNavigation();
        break;
    }

    return fileDeletionInProgress ? maskActiveNavigationDuringDeletion(snapshot) : snapshot;
}

ActiveNavigationSnapshot maskActiveNavigationDuringDeletion(ActiveNavigationSnapshot snapshot)
{
    snapshot.editable = false;
    snapshot.canOpenPrevious = false;
    snapshot.canOpenNext = false;
    return snapshot;
}

ActiveNavigationBoundaryScope activeNavigationBoundaryScopeForSource(
    ActiveNavigationSourceKind sourceKind)
{
    switch (sourceKind) {
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        return ActiveNavigationBoundaryScope::DirectMedia;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        return ActiveNavigationBoundaryScope::ImageDocumentPage;
    case ActiveNavigationSourceKind::None:
        return ActiveNavigationBoundaryScope::None;
    }

    return ActiveNavigationBoundaryScope::None;
}

ActiveNavigationDispatchRequest previousActiveNavigationDispatchRequest()
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::Previous, 0 };
}

ActiveNavigationDispatchRequest nextActiveNavigationDispatchRequest()
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::Next, 0 };
}

ActiveNavigationDispatchRequest firstActiveNavigationDispatchRequest()
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::First, 1 };
}

ActiveNavigationDispatchRequest lastActiveNavigationDispatchRequest()
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::Last, 0 };
}

ActiveNavigationDispatchRequest numberedActiveNavigationDispatchRequest(int number)
{
    return ActiveNavigationDispatchRequest { ActiveNavigationDispatchRequestKind::Number, number };
}

ActiveNavigationDispatchPlan activeNavigationDispatchPlan(ActiveNavigationSourceKind sourceKind,
    ActiveNavigationSnapshot snapshot, ActiveNavigationDispatchRequest request)
{
    if (!snapshot.available) {
        return {};
    }

    switch (request.kind) {
    case ActiveNavigationDispatchRequestKind::Previous:
        if (snapshot.canOpenPrevious) {
            return dispatchOperation(previousOperationForSource(sourceKind));
        }
        return snapshot.known && snapshot.editable && snapshot.atKnownFirst
            ? boundaryOutcome(ActiveNavigationDispatchOutcome::FirstBoundary)
            : ActiveNavigationDispatchPlan {};
    case ActiveNavigationDispatchRequestKind::Next:
        if (snapshot.canOpenNext) {
            return dispatchOperation(nextOperationForSource(sourceKind));
        }
        return snapshot.known && snapshot.editable && snapshot.atKnownLast
            ? boundaryOutcome(ActiveNavigationDispatchOutcome::LastBoundary)
            : ActiveNavigationDispatchPlan {};
    case ActiveNavigationDispatchRequestKind::First:
        return snapshot.known && snapshot.editable && !snapshot.atKnownFirst
            ? dispatchOperation(numberedOperationForSource(sourceKind, request.number))
            : ActiveNavigationDispatchPlan {};
    case ActiveNavigationDispatchRequestKind::Last:
        return snapshot.known && snapshot.editable && !snapshot.atKnownLast
            ? dispatchOperation(numberedOperationForSource(sourceKind, snapshot.count))
            : ActiveNavigationDispatchPlan {};
    case ActiveNavigationDispatchRequestKind::Number:
        return snapshot.known && snapshot.editable
            ? dispatchOperation(numberedOperationForSource(sourceKind, request.number))
            : ActiveNavigationDispatchPlan {};
    }

    return {};
}
}
