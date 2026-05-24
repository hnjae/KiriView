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
}

namespace KiriView {
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
}
