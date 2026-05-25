// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "activenavigationprojection.h"

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

KiriView::ActiveNavigationSnapshot mediaActiveNavigationSnapshot(
    KiriView::MediaActiveNavigationInput input)
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

KiriView::ActiveNavigationSnapshot imageDocumentActiveNavigationSnapshot(
    KiriView::ImageDocumentActiveNavigationInput input)
{
    if (input.currentNumber < 1 || input.currentLastNumber < input.currentNumber || input.count < 1
        || input.currentLastNumber > input.count) {
        return unknownActiveNavigation();
    }

    return normalizedActiveNavigation(KiriView::ActiveNavigationSnapshot {
        true,
        true,
        true,
        input.currentNumber > 1,
        input.currentLastNumber < input.count,
        input.currentNumber == 1,
        input.currentLastNumber >= input.count,
        input.currentNumber,
        input.count,
    });
}

KiriView::ActiveNavigationDispatchTarget dispatchTargetForSource(
    KiriView::ActiveNavigationSourceKind sourceKind)
{
    switch (sourceKind) {
    case KiriView::ActiveNavigationSourceKind::OrdinaryDirectMedia:
        return KiriView::ActiveNavigationDispatchTarget::OrdinaryDirectMedia;
    case KiriView::ActiveNavigationSourceKind::ImageDocumentPages:
        return KiriView::ActiveNavigationDispatchTarget::ImageDocumentPages;
    case KiriView::ActiveNavigationSourceKind::None:
        return KiriView::ActiveNavigationDispatchTarget::None;
    }

    return KiriView::ActiveNavigationDispatchTarget::None;
}

KiriView::ActiveNavigationDispatchPlan dispatchToSource(
    KiriView::ActiveNavigationSourceKind sourceKind,
    KiriView::ActiveNavigationDispatchOperation operation, int number)
{
    return KiriView::ActiveNavigationDispatchPlan {
        dispatchTargetForSource(sourceKind),
        operation,
        number,
    };
}
}

namespace KiriView {
bool ActiveNavigationDispatchPlan::shouldDispatch() const
{
    return target != ActiveNavigationDispatchTarget::None
        && operation != ActiveNavigationDispatchOperation::None;
}

ActiveNavigationSnapshot projectActiveNavigation(ActiveNavigationSourceKind sourceKind,
    MediaActiveNavigationInput mediaInput, ImageDocumentActiveNavigationInput imageInput,
    bool fileDeletionInProgress)
{
    ActiveNavigationSnapshot snapshot;
    switch (sourceKind) {
    case ActiveNavigationSourceKind::OrdinaryDirectMedia:
        snapshot = mediaActiveNavigationSnapshot(mediaInput);
        break;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        snapshot = imageDocumentActiveNavigationSnapshot(imageInput);
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
        return ActiveNavigationBoundaryScope::Media;
    case ActiveNavigationSourceKind::ImageDocumentPages:
        return ActiveNavigationBoundaryScope::ImageDocument;
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
        return snapshot.canOpenPrevious
            ? dispatchToSource(sourceKind, ActiveNavigationDispatchOperation::OpenPrevious, 0)
            : ActiveNavigationDispatchPlan {};
    case ActiveNavigationDispatchRequestKind::Next:
        return snapshot.canOpenNext
            ? dispatchToSource(sourceKind, ActiveNavigationDispatchOperation::OpenNext, 0)
            : ActiveNavigationDispatchPlan {};
    case ActiveNavigationDispatchRequestKind::First:
        return snapshot.known && snapshot.editable && !snapshot.atKnownFirst
            ? dispatchToSource(
                  sourceKind, ActiveNavigationDispatchOperation::OpenNumber, request.number)
            : ActiveNavigationDispatchPlan {};
    case ActiveNavigationDispatchRequestKind::Last:
        return snapshot.known && snapshot.editable && !snapshot.atKnownLast
            ? dispatchToSource(
                  sourceKind, ActiveNavigationDispatchOperation::OpenNumber, snapshot.count)
            : ActiveNavigationDispatchPlan {};
    case ActiveNavigationDispatchRequestKind::Number:
        return snapshot.known && snapshot.editable
            ? dispatchToSource(
                  sourceKind, ActiveNavigationDispatchOperation::OpenNumber, request.number)
            : ActiveNavigationDispatchPlan {};
    }

    return {};
}
}
