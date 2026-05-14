// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadnavigation.h"

#include "kiriview/src/imagespreadnavigation.cxx.h"

namespace {
KiriView::RustImageSpreadNavigationDirection rustNavigationDirection(
    KiriView::NavigationDirection direction)
{
    switch (direction) {
    case KiriView::NavigationDirection::Previous:
        return KiriView::RustImageSpreadNavigationDirection::Previous;
    case KiriView::NavigationDirection::Next:
        return KiriView::RustImageSpreadNavigationDirection::Next;
    }

    return KiriView::RustImageSpreadNavigationDirection::Next;
}

KiriView::RustImageSpreadNavigationState rustNavigationState(
    const KiriView::ImageSpreadNavigationState &state)
{
    return KiriView::RustImageSpreadNavigationState { state.twoPageModeActive,
        state.currentPageNumber, state.imageCount, state.secondaryPageVisible,
        state.previousPageIsWide };
}

KiriView::ImageSpreadPageNavigationTarget imageSpreadPageNavigationTargetFromRust(
    KiriView::RustImageSpreadPageNavigationTarget target)
{
    return KiriView::ImageSpreadPageNavigationTarget { target.handled_by_spread,
        target.page_number };
}
}

namespace KiriView {
int imageSpreadPreviousPageTarget(
    int currentPageNumber, bool secondaryPageVisible, bool previousPageIsWide)
{
    return rustImageSpreadPreviousPageTarget(
        currentPageNumber, secondaryPageVisible, previousPageIsWide);
}

int imageSpreadCurrentLastPageNumber(int currentPageNumber, bool secondaryPageVisible)
{
    return rustImageSpreadCurrentLastPageNumber(currentPageNumber, secondaryPageVisible);
}

int imageSpreadRelativePageTarget(int currentPageNumber, int imageCount, int offset)
{
    return rustImageSpreadRelativePageTarget(currentPageNumber, imageCount, offset);
}

int imageSpreadNextPageTarget(int currentLastPageNumber, int imageCount)
{
    return rustImageSpreadNextPageTarget(currentLastPageNumber, imageCount);
}

bool imageSpreadShouldBeginTransition(
    bool twoPageModeActive, int currentPageNumber, int targetPageNumber, int imageCount)
{
    return rustImageSpreadShouldBeginTransition(
        twoPageModeActive, currentPageNumber, targetPageNumber, imageCount);
}

int imageSpreadNavigationCurrentLastPageNumber(const ImageSpreadNavigationState &state)
{
    return rustImageSpreadNavigationCurrentLastPageNumber(rustNavigationState(state));
}

ImageSpreadPageNavigationTarget imageSpreadPageNavigationTarget(
    NavigationDirection direction, const ImageSpreadNavigationState &state)
{
    return imageSpreadPageNavigationTargetFromRust(rustImageSpreadPageNavigationTarget(
        rustNavigationDirection(direction), rustNavigationState(state)));
}

int imageSpreadRelativePageNavigationTarget(const ImageSpreadNavigationState &state, int offset)
{
    return rustImageSpreadRelativePageNavigationTarget(rustNavigationState(state), offset);
}

bool imageSpreadShouldBeginNavigationTransition(
    const ImageSpreadNavigationState &state, int targetPageNumber)
{
    return rustImageSpreadShouldBeginNavigationTransition(
        rustNavigationState(state), targetPageNumber);
}
}
