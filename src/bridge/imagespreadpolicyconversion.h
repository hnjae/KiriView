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
    ImageSpreadReadingAvailability availability);
ImageSpreadTwoPageModeChange imageSpreadTwoPageModeChangeFromRust(
    RustImageSpreadTwoPageModeChange change);
RustImageSpreadSecondaryPageRefreshState rustImageSpreadSecondaryPageRefreshState(
    ImageSpreadSecondaryPageRefreshState state);
ImageSpreadSecondaryPageDecision imageSpreadSecondaryPageDecisionFromRust(
    RustImageSpreadSecondaryPageDecision decision);
ImageSpreadSecondaryPageRefreshPlan imageSpreadSecondaryPageRefreshPlanFromRust(
    RustImageSpreadSecondaryPageRefreshPlan plan);
RustImageSpreadNavigationDirection rustImageSpreadNavigationDirection(
    NavigationDirection direction);
RustImageSpreadNavigationState rustImageSpreadNavigationState(ImageSpreadNavigationState state);
ImageSpreadPageNavigationTarget imageSpreadPageNavigationTargetFromRust(
    RustImageSpreadPageNavigationTarget target);
}

#endif
