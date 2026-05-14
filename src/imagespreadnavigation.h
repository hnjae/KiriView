// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADNAVIGATION_H
#define KIRIVIEW_IMAGESPREADNAVIGATION_H

#include "imagenavigationtypes.h"

namespace KiriView {
struct ImageSpreadNavigationState {
    bool twoPageModeActive = false;
    int currentPageNumber = 0;
    int imageCount = 0;
    bool secondaryPageVisible = false;
    bool previousPageIsWide = false;
};

struct ImageSpreadPageNavigationTarget {
    bool handledBySpread = false;
    int pageNumber = 0;
};

int imageSpreadPreviousPageTarget(
    int currentPageNumber, bool secondaryPageVisible, bool previousPageIsWide);
int imageSpreadCurrentLastPageNumber(int currentPageNumber, bool secondaryPageVisible);
int imageSpreadRelativePageTarget(int currentPageNumber, int imageCount, int offset);
int imageSpreadNextPageTarget(int currentLastPageNumber, int imageCount);
bool imageSpreadShouldBeginTransition(
    bool twoPageModeActive, int currentPageNumber, int targetPageNumber, int imageCount);
int imageSpreadNavigationCurrentLastPageNumber(const ImageSpreadNavigationState &state);
ImageSpreadPageNavigationTarget imageSpreadPageNavigationTarget(
    NavigationDirection direction, const ImageSpreadNavigationState &state);
int imageSpreadRelativePageNavigationTarget(const ImageSpreadNavigationState &state, int offset);
bool imageSpreadShouldBeginNavigationTransition(
    const ImageSpreadNavigationState &state, int targetPageNumber);
}

#endif
