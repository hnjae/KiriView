// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageshortcutnavigationpolicy.h"

ImageShortcutNavigationPolicy::ImageShortcutNavigationPolicy(QObject *parent)
    : QObject(parent)
{
}

ImageShortcutNavigationPolicy::HorizontalArrowAction
ImageShortcutNavigationPolicy::horizontalArrowAction(
    bool leftArrow, bool horizontallyPannable, bool rightToLeftReadingActive) const
{
    if (horizontallyPannable) {
        return leftArrow ? PanLeft : PanRight;
    }

    if (leftArrow == rightToLeftReadingActive) {
        return OpenNextImage;
    }

    return OpenPreviousImage;
}

ImageShortcutNavigationPolicy::SinglePageArrowAction
ImageShortcutNavigationPolicy::singlePageArrowAction(
    bool leftArrow, bool rightToLeftReadingActive) const
{
    if (leftArrow == rightToLeftReadingActive) {
        return OpenNextSinglePage;
    }

    return OpenPreviousSinglePage;
}

ImageShortcutNavigationPolicy::ScanAction ImageShortcutNavigationPolicy::scanForwardAction(
    bool imagePannable, bool viewportMoved) const
{
    if (imagePannable && viewportMoved) {
        return NoScanAction;
    }

    return OpenNextImageFromScan;
}

ImageShortcutNavigationPolicy::ScanAction ImageShortcutNavigationPolicy::scanBackwardAction(
    bool imagePannable, bool viewportMoved, bool atFirstImage, int currentPageNumber) const
{
    if (!imagePannable) {
        return OpenPreviousImageFromScan;
    }

    if (viewportMoved) {
        return NoScanAction;
    }

    if (atFirstImage) {
        return ShowFirstImageBoundary;
    }

    if (currentPageNumber > 1) {
        return OpenPreviousPageFromFinalScanStart;
    }

    return OpenPreviousImageFromScan;
}
