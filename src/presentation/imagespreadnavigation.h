// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADNAVIGATION_H
#define KIRIVIEW_IMAGESPREADNAVIGATION_H

#include "navigation/imagedocumentpagenavigationtypes.h"

namespace kiriview {
struct ImageSpreadNavigationState
{
    bool twoPageModeActive = false;
    int currentPageNumber = 0;
    int pageCount = 0;
    bool secondaryPageVisible = false;
    bool previousPageIsWide = false;
};

struct ImageSpreadPageNavigationTarget
{
    bool handledBySpread = false;
    int pageNumber = 0;
};

int imageSpreadNavigationCurrentLastPageNumber(ImageSpreadNavigationState state);
ImageSpreadPageNavigationTarget imageSpreadPageNavigationTarget(
    NavigationDirection direction, ImageSpreadNavigationState state);
int imageSpreadRelativePageNavigationTarget(ImageSpreadNavigationState state, int offset);
bool imageSpreadShouldBeginNavigationTransition(
    ImageSpreadNavigationState state, int targetPageNumber);
}

#endif
