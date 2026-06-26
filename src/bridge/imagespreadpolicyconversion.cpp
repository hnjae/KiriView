// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "bridge/imagespreadpolicyconversion.h"

namespace kiriview::Bridge {
RustImageSpreadReadingAvailability rustImageSpreadReadingAvailability(
    const ImageSpreadReadingAvailability& availability)
{
    return RustImageSpreadReadingAvailability { availability.hasImage,
        availability.hasDisplayedImage, availability.displayedDocumentIsComicBook };
}

ImageSpreadTwoPageModeChange imageSpreadTwoPageModeChangeFromRust(
    const RustImageSpreadTwoPageModeChange& change)
{
    return ImageSpreadTwoPageModeChange { change.changed, change.finish_transition,
        change.clear_secondary_page, change.refresh_secondary_page, change.notify_two_page_mode };
}

RustImageSpreadSecondaryPageRefreshState rustImageSpreadSecondaryPageRefreshState(
    const ImageSpreadSecondaryPageRefreshState& state)
{
    return RustImageSpreadSecondaryPageRefreshState { state.twoPageModeActive,
        state.currentPageNumber, state.pageCount, state.primaryPageIsWide, state.nextPageAvailable,
        state.nextPageIsWide, state.currentSecondaryMatchesNext };
}

ImageSpreadSecondaryPageDecision imageSpreadSecondaryPageDecisionFromRust(
    RustImageSpreadSecondaryPageDecision decision)
{
    switch (decision) {
    case RustImageSpreadSecondaryPageDecision::PrimaryOnly:
        return ImageSpreadSecondaryPageDecision::PrimaryOnly;
    case RustImageSpreadSecondaryPageDecision::LoadNext:
        return ImageSpreadSecondaryPageDecision::LoadNext;
    case RustImageSpreadSecondaryPageDecision::KeepCurrentSecondary:
        return ImageSpreadSecondaryPageDecision::KeepCurrentSecondary;
    }

    return ImageSpreadSecondaryPageDecision::PrimaryOnly;
}

ImageSpreadSecondaryPageRefreshPlan imageSpreadSecondaryPageRefreshPlanFromRust(
    const RustImageSpreadSecondaryPageRefreshPlan& plan)
{
    return ImageSpreadSecondaryPageRefreshPlan {
        imageSpreadSecondaryPageDecisionFromRust(plan.decision),
        plan.target_page_number,
    };
}

RustImageSpreadNavigationDirection rustImageSpreadNavigationDirection(NavigationDirection direction)
{
    switch (direction) {
    case NavigationDirection::Previous:
        return RustImageSpreadNavigationDirection::Previous;
    case NavigationDirection::Next:
        return RustImageSpreadNavigationDirection::Next;
    }

    return RustImageSpreadNavigationDirection::Next;
}

RustImageSpreadNavigationState rustImageSpreadNavigationState(
    const ImageSpreadNavigationState& state)
{
    return RustImageSpreadNavigationState { state.twoPageModeActive, state.currentPageNumber,
        state.pageCount, state.secondaryPageVisible, state.previousPageIsWide };
}

ImageSpreadPageNavigationTarget imageSpreadPageNavigationTargetFromRust(
    const RustImageSpreadPageNavigationTarget& target)
{
    return ImageSpreadPageNavigationTarget { target.handled_by_spread, target.page_number };
}
}
