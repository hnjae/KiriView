// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_BRIDGE_IMAGESPREADPOLICYCONVERSION_H
#define KIRIVIEW_BRIDGE_IMAGESPREADPOLICYCONVERSION_H

#include "kiriview/src/policy/imagespreadnavigation.cxx.h"
#include "kiriview/src/policy/imagespreadpolicy.cxx.h"
#include "navigation/imagedocumentpagenavigationtypes.h"
#include "presentation/imagespreadmodepolicy.h"
#include "presentation/imagespreadnavigation.h"
#include "presentation/imagespreadsecondarypagerefreshpolicy.h"

namespace kiriview::Bridge {
RustImageSpreadReadingAvailability rustImageSpreadReadingAvailability(
    const ImageSpreadReadingAvailability &availability);
ImageSpreadTwoPageModeChange imageSpreadTwoPageModeChangeFromRust(
    const RustImageSpreadTwoPageModeChange &change);
RustImageSpreadSecondaryPageRefreshState rustImageSpreadSecondaryPageRefreshState(
    const ImageSpreadSecondaryPageRefreshState &state);
ImageSpreadSecondaryPageDecision imageSpreadSecondaryPageDecisionFromRust(
    RustImageSpreadSecondaryPageDecision decision);
ImageSpreadSecondaryPageRefreshPlan imageSpreadSecondaryPageRefreshPlanFromRust(
    const RustImageSpreadSecondaryPageRefreshPlan &plan);
RustImageSpreadNavigationDirection rustImageSpreadNavigationDirection(
    NavigationDirection direction);
RustImageSpreadNavigationState rustImageSpreadNavigationState(
    const ImageSpreadNavigationState &state);
ImageSpreadPageNavigationTarget imageSpreadPageNavigationTargetFromRust(
    const RustImageSpreadPageNavigationTarget &target);
}

#endif
