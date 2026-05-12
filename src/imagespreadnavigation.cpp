// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagespreadnavigation.h"

#include "imagespreadgeometry.h"

namespace KiriView {
int imageSpreadNavigationCurrentLastPageNumber(const ImageSpreadNavigationState &state)
{
    return imageSpreadCurrentLastPageNumber(state.currentPageNumber, state.secondaryPageVisible);
}

ImageSpreadPageNavigationTarget imageSpreadPageNavigationTarget(
    NavigationDirection direction, const ImageSpreadNavigationState &state)
{
    if (!state.twoPageModeActive || state.currentPageNumber <= 0) {
        return {};
    }

    if (direction == NavigationDirection::Next) {
        return ImageSpreadPageNavigationTarget { true,
            imageSpreadNextPageTarget(
                imageSpreadNavigationCurrentLastPageNumber(state), state.imageCount) };
    }

    return ImageSpreadPageNavigationTarget { true,
        imageSpreadPreviousPageTarget(
            state.currentPageNumber, state.secondaryPageVisible, state.previousPageIsWide) };
}

int imageSpreadRelativePageNavigationTarget(const ImageSpreadNavigationState &state, int offset)
{
    return imageSpreadRelativePageTarget(state.currentPageNumber, state.imageCount, offset);
}

bool imageSpreadShouldBeginNavigationTransition(
    const ImageSpreadNavigationState &state, int targetPageNumber)
{
    return imageSpreadShouldBeginTransition(
        state.twoPageModeActive, state.currentPageNumber, targetPageNumber, state.imageCount);
}
}
