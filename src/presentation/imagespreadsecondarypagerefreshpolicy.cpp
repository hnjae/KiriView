// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadsecondarypagerefreshpolicy.h"

#include "bridge/imagespreadpolicyconversion.h"
#include "kiriview/src/policy/imagespreadpolicy.cxx.h"

namespace kiriview {
ImageSpreadSecondaryPageRefreshPlan imageSpreadSecondaryPageRefreshPlan(
    ImageSpreadSecondaryPageRefreshState state)
{
    const RustImageSpreadSecondaryPageRefreshPlan plan = rustImageSpreadSecondaryPageRefreshPlan(
        Bridge::rustImageSpreadSecondaryPageRefreshState(state));
    return Bridge::imageSpreadSecondaryPageRefreshPlanFromRust(plan);
}
}
