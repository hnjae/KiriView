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
    return facadeAction(
        m_policy.horizontalArrowAction(leftArrow, horizontallyPannable, rightToLeftReadingActive));
}

ImageShortcutNavigationPolicy::SinglePageArrowAction
ImageShortcutNavigationPolicy::singlePageArrowAction(
    bool leftArrow, bool rightToLeftReadingActive) const
{
    return facadeAction(m_policy.singlePageArrowAction(leftArrow, rightToLeftReadingActive));
}

ImageShortcutNavigationPolicy::ScanAction ImageShortcutNavigationPolicy::scanForwardAction(
    bool imagePannable, bool viewportMoved) const
{
    return facadeAction(m_policy.scanForwardAction(imagePannable, viewportMoved));
}

ImageShortcutNavigationPolicy::ScanAction ImageShortcutNavigationPolicy::scanBackwardAction(
    bool imagePannable, bool viewportMoved, bool imageDocumentPageNavigationActive,
    bool atKnownFirstActiveNavigation, bool canOpenPreviousActiveNavigation) const
{
    return facadeAction(
        m_policy.scanBackwardAction(imagePannable, viewportMoved, imageDocumentPageNavigationActive,
            atKnownFirstActiveNavigation, canOpenPreviousActiveNavigation));
}

ImageShortcutNavigationPolicy::HorizontalArrowAction ImageShortcutNavigationPolicy::facadeAction(
    KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction action)
{
    using DomainAction = KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction;
    switch (action) {
    case DomainAction::PanLeft:
        return PanLeft;
    case DomainAction::PanRight:
        return PanRight;
    case DomainAction::RequestPreviousActiveNavigation:
        return RequestPreviousActiveNavigation;
    case DomainAction::RequestNextActiveNavigation:
        return RequestNextActiveNavigation;
    }

    return RequestPreviousActiveNavigation;
}

ImageShortcutNavigationPolicy::SinglePageArrowAction ImageShortcutNavigationPolicy::facadeAction(
    KiriView::ImageShortcutNavigationPolicy::SinglePageArrowAction action)
{
    using DomainAction = KiriView::ImageShortcutNavigationPolicy::SinglePageArrowAction;
    switch (action) {
    case DomainAction::OpenPreviousSinglePage:
        return OpenPreviousSinglePage;
    case DomainAction::OpenNextSinglePage:
        return OpenNextSinglePage;
    }

    return OpenPreviousSinglePage;
}

ImageShortcutNavigationPolicy::ScanAction ImageShortcutNavigationPolicy::facadeAction(
    KiriView::ImageShortcutNavigationPolicy::ScanAction action)
{
    using DomainAction = KiriView::ImageShortcutNavigationPolicy::ScanAction;
    switch (action) {
    case DomainAction::NoScanAction:
        return NoScanAction;
    case DomainAction::RequestPreviousActiveNavigationFromScan:
        return RequestPreviousActiveNavigationFromScan;
    case DomainAction::RequestNextActiveNavigationFromScan:
        return RequestNextActiveNavigationFromScan;
    case DomainAction::OpenPreviousPageFromFinalScanStart:
        return OpenPreviousPageFromFinalScanStart;
    case DomainAction::ShowFirstImageBoundary:
        return ShowFirstImageBoundary;
    }

    return NoScanAction;
}
