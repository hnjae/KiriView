// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageshortcutnavigationpolicy.h"

namespace KiriView {
ImageShortcutNavigationPolicy::HorizontalArrowAction
ImageShortcutNavigationPolicy::horizontalArrowAction(
    bool leftArrow, bool horizontallyPannable, bool rightToLeftReadingActive) const
{
    if (horizontallyPannable) {
        return leftArrow ? HorizontalArrowAction::PanLeft : HorizontalArrowAction::PanRight;
    }

    if (leftArrow == rightToLeftReadingActive) {
        return HorizontalArrowAction::OpenNextImage;
    }

    return HorizontalArrowAction::OpenPreviousImage;
}

ImageShortcutNavigationPolicy::SinglePageArrowAction
ImageShortcutNavigationPolicy::singlePageArrowAction(
    bool leftArrow, bool rightToLeftReadingActive) const
{
    if (leftArrow == rightToLeftReadingActive) {
        return SinglePageArrowAction::OpenNextSinglePage;
    }

    return SinglePageArrowAction::OpenPreviousSinglePage;
}

ImageShortcutNavigationPolicy::ScanAction ImageShortcutNavigationPolicy::scanForwardAction(
    bool imagePannable, bool viewportMoved) const
{
    if (imagePannable && viewportMoved) {
        return ScanAction::NoScanAction;
    }

    return ScanAction::OpenNextImageFromScan;
}

ImageShortcutNavigationPolicy::ScanAction ImageShortcutNavigationPolicy::scanBackwardAction(
    bool imagePannable, bool viewportMoved, bool atFirstImage, int currentPageNumber) const
{
    if (!imagePannable) {
        return ScanAction::OpenPreviousImageFromScan;
    }

    if (viewportMoved) {
        return ScanAction::NoScanAction;
    }

    if (atFirstImage) {
        return ScanAction::ShowFirstImageBoundary;
    }

    if (currentPageNumber > 1) {
        return ScanAction::OpenPreviousPageFromFinalScanStart;
    }

    return ScanAction::OpenPreviousImageFromScan;
}
}
