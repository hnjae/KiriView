// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadmodepolicy.h"

#include "bridge/imagespreadpolicyconversion.h"
#include "kiriview/src/policy/imagespreadpolicy.cxx.h"

namespace kiriview {
bool imageSpreadReadingControlsAvailable(ImageSpreadReadingAvailability availability)
{
    return rustImageSpreadReadingControlsAvailable(
        Bridge::rustImageSpreadReadingAvailability(availability));
}

ImageSpreadTwoPageModeChange imageSpreadTwoPageModeChange(
    bool currentEnabled, bool nextEnabled, bool secondaryPageVisible)
{
    const RustImageSpreadTwoPageModeChange change
        = rustImageSpreadTwoPageModeChange(currentEnabled, nextEnabled, secondaryPageVisible);
    return Bridge::imageSpreadTwoPageModeChangeFromRust(change);
}
}
