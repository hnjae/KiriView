// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadnavigation.h"

#include "bridge/imagespreadpolicyconversion.h"
#include "kiriview/src/policy/imagespreadnavigation.cxx.h"

namespace kiriview {
int imageSpreadNavigationCurrentLastPageNumber(ImageSpreadNavigationState state)
{
    return rustImageSpreadNavigationCurrentLastPageNumber(
        Bridge::rustImageSpreadNavigationState(state));
}

ImageSpreadPageNavigationTarget imageSpreadPageNavigationTarget(
    NavigationDirection direction, ImageSpreadNavigationState state)
{
    return Bridge::imageSpreadPageNavigationTargetFromRust(
        rustImageSpreadPageNavigationTarget(Bridge::rustImageSpreadNavigationDirection(direction),
            Bridge::rustImageSpreadNavigationState(state)));
}

int imageSpreadRelativePageNavigationTarget(ImageSpreadNavigationState state, int offset)
{
    return rustImageSpreadRelativePageNavigationTarget(
        Bridge::rustImageSpreadNavigationState(state), offset);
}

bool imageSpreadShouldBeginNavigationTransition(
    ImageSpreadNavigationState state, int targetPageNumber)
{
    return rustImageSpreadShouldBeginNavigationTransition(
        Bridge::rustImageSpreadNavigationState(state), targetPageNumber);
}
}
