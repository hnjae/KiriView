// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADNAVIGATION_H
#define KIRIVIEW_IMAGESPREADNAVIGATION_H

#include "navigation/imagenavigationtypes.h"

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

int imageSpreadNavigationCurrentLastPageNumber(const ImageSpreadNavigationState &state);
ImageSpreadPageNavigationTarget imageSpreadPageNavigationTarget(
    NavigationDirection direction, const ImageSpreadNavigationState &state);
int imageSpreadRelativePageNavigationTarget(const ImageSpreadNavigationState &state, int offset);
bool imageSpreadShouldBeginNavigationTransition(
    const ImageSpreadNavigationState &state, int targetPageNumber);
}

#endif
