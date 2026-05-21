// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadmodepolicy.h"

#include "kiriview/src/policy/imagespreadgeometry.cxx.h"

namespace {
KiriView::RustImageSpreadReadingAvailability rustReadingAvailability(
    const KiriView::ImageSpreadReadingAvailability &availability)
{
    return KiriView::RustImageSpreadReadingAvailability { availability.hasImage,
        availability.hasDisplayedImage, availability.displayedDocumentIsComicBook };
}
}

namespace KiriView {
bool imageSpreadReadingControlsAvailable(const ImageSpreadReadingAvailability &availability)
{
    return rustImageSpreadReadingControlsAvailable(rustReadingAvailability(availability));
}

ImageSpreadTwoPageModeChange imageSpreadTwoPageModeChange(
    bool currentEnabled, bool nextEnabled, bool secondaryPageVisible)
{
    const RustImageSpreadTwoPageModeChange change
        = rustImageSpreadTwoPageModeChange(currentEnabled, nextEnabled, secondaryPageVisible);
    return ImageSpreadTwoPageModeChange { change.changed, change.reset_spread_zoom,
        change.finish_transition, change.clear_secondary_page, change.restore_primary_zoom,
        change.refresh_secondary_page, change.notify_two_page_mode };
}
}
