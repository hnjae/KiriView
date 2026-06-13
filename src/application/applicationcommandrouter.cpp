// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationcommandrouter.h"

namespace {
constexpr double keyboardPanDistance = 64.0;

bool callBool(const std::function<bool()> &callback) { return callback && callback(); }

void callVoid(const std::function<void()> &callback)
{
    if (callback) {
        callback();
    }
}

void callDouble(const std::function<void(double)> &callback, double value)
{
    if (callback) {
        callback(value);
    }
}

void panBy(
    const KiriView::ApplicationActions::ApplicationCommandRouterPorts &ports, double dx, double dy)
{
    if (ports.requestViewportPanBy) {
        ports.requestViewportPanBy(dx, dy);
    }
}
}

namespace KiriView::ApplicationActions {
void ApplicationCommandRouter::handleActionTriggered(ActionId actionId,
    const ApplicationCommandRouterInput &input, const ApplicationCommandRouterPorts &ports) const
{
    switch (actionId) {
    case ActionId::FileOpenAction:
        callVoid(ports.requestOpenDialog);
        return;
    case ActionId::FileOpenWithAction:
        callVoid(ports.openCurrentMediaWith);
        return;
    case ActionId::FileMoveToTrashAction:
        callVoid(ports.moveDisplayedFileToTrash);
        return;
    case ActionId::FileDeleteAction:
        callVoid(ports.deleteDisplayedFilePermanently);
        return;
    case ActionId::GoPreviousArchiveAction:
        callVoid(ports.openPreviousContainer);
        return;
    case ActionId::GoNextArchiveAction:
        callVoid(ports.openNextContainer);
        return;
    case ActionId::GoPreviousImageAction:
        callVoid(ports.requestPreviousActiveNavigationWithBoundary);
        return;
    case ActionId::GoNextImageAction:
        callVoid(ports.requestNextActiveNavigationWithBoundary);
        return;
    case ActionId::GoFirstImageAction:
        callVoid(ports.openFirstActiveNavigation);
        return;
    case ActionId::GoLastImageAction:
        callVoid(ports.openLastActiveNavigation);
        return;
    case ActionId::ViewZoomInAction:
        callDouble(ports.requestZoomByStepAtCenter, 1.0);
        return;
    case ActionId::ViewZoomOutAction:
        callDouble(ports.requestZoomByStepAtCenter, -1.0);
        return;
    case ActionId::ViewZoom50PercentAction:
        callDouble(ports.requestManualZoomPercent, 50.0);
        return;
    case ActionId::ViewZoom100PercentAction:
        callDouble(ports.requestManualZoomPercent, 100.0);
        return;
    case ActionId::ViewZoom200PercentAction:
        callDouble(ports.requestManualZoomPercent, 200.0);
        return;
    case ActionId::ViewFitAction:
        callVoid(ports.requestFitMode);
        return;
    case ActionId::ViewFitHeightAction:
        callVoid(ports.requestFitHeightMode);
        return;
    case ActionId::ViewFitWidthAction:
        callVoid(ports.requestFitWidthMode);
        return;
    case ActionId::ViewRotateClockwiseAction:
        callVoid(ports.rotateClockwise);
        return;
    case ActionId::ViewRotateCounterclockwiseAction:
        callVoid(ports.rotateCounterclockwise);
        return;
    case ActionId::ViewToggleTwoPageModeAction:
        callVoid(ports.requestToggleTwoPageMode);
        return;
    case ActionId::ViewToggleRightToLeftReadingAction:
        callVoid(ports.requestToggleRightToLeftReading);
        return;
    case ActionId::ViewToggleInfoPanelAction:
        callVoid(ports.toggleInfoPanel);
        return;
    case ActionId::ViewToggleThumbnailPanelAction:
        callVoid(ports.toggleThumbnailPanel);
        return;
    case ActionId::ViewPanTopLeftAction:
        callVoid(ports.requestViewportPanToInitialScanPosition);
        return;
    case ActionId::ViewPanBottomRightAction:
        callVoid(ports.requestViewportPanToFinalScanPosition);
        return;
    case ActionId::ViewScanForwardAction:
        handleScanForwardAction(input, ports);
        return;
    case ActionId::ViewScanBackwardAction:
        handleScanBackwardAction(input, ports);
        return;
    case ActionId::ViewToggleVideoPlaybackAction:
        if (input.videoMode && callBool(ports.videoAvailable)) {
            callVoid(ports.toggleVideoPlayback);
        }
        return;
    case ActionId::WindowFullscreenAction:
        callVoid(ports.toggleFullScreen);
        return;
    case ActionId::HelpShortcutsAction:
        callVoid(ports.requestShortcutHelp);
        return;
    case ActionId::OpenApplicationMenuAction:
        callVoid(ports.openApplicationMenu);
        return;
    case ActionId::OptionsConfigureKeybindingAction:
    case ActionId::OptionsShowMenubarAction:
    case ActionId::FileQuitAction:
    case ActionId::ActionCount:
        return;
    }
}

void ApplicationCommandRouter::handleScanForwardAction(
    const ApplicationCommandRouterInput &input, const ApplicationCommandRouterPorts &ports) const
{
    if (input.videoMode) {
        callVoid(ports.requestNextActiveNavigationWithBoundary);
        return;
    }

    const bool viewportMoved = callBool(ports.requestViewportScanForward);
    const KiriView::ImageShortcutNavigationPolicy::ScanAction action
        = m_navigationPolicy.scanForwardAction(input.imagePannable, viewportMoved);
    switch (action) {
    case KiriView::ImageShortcutNavigationPolicy::ScanAction::RequestNextActiveNavigationFromScan:
        callVoid(ports.requestNextActiveNavigationWithBoundary);
        return;
    default:
        return;
    }
}

void ApplicationCommandRouter::handleScanBackwardAction(
    const ApplicationCommandRouterInput &input, const ApplicationCommandRouterPorts &ports) const
{
    if (input.videoMode) {
        callVoid(ports.requestPreviousActiveNavigationWithBoundary);
        return;
    }

    const bool viewportMoved = callBool(ports.requestViewportScanBackward);
    const KiriView::ImageShortcutNavigationPolicy::ScanAction action
        = m_navigationPolicy.scanBackwardAction(input.imagePannable, viewportMoved,
            input.imageDocumentPageNavigationActive, input.atKnownFirstActiveNavigation,
            input.canOpenPreviousActiveNavigation);
    switch (action) {
    case KiriView::ImageShortcutNavigationPolicy::ScanAction::
        RequestPreviousActiveNavigationFromScan:
        callVoid(ports.requestPreviousActiveNavigationWithBoundary);
        return;
    case KiriView::ImageShortcutNavigationPolicy::ScanAction::OpenPreviousPageFromFinalScanStart:
        callVoid(ports.requestNextDisplayedImageStartToFinalScanPosition);
        callVoid(ports.requestPreviousActiveNavigationWithBoundary);
        return;
    case KiriView::ImageShortcutNavigationPolicy::ScanAction::ShowFirstImageBoundary:
        callVoid(ports.showFirstImageBoundary);
        return;
    default:
        return;
    }
}

bool ApplicationCommandRouter::executeHorizontalArrowShortcut(
    const ApplicationCommandRouterInput &input, const ApplicationCommandRouterPorts &ports,
    bool leftArrow) const
{
    if (input.videoMode) {
        if (leftArrow) {
            callVoid(ports.requestPreviousActiveNavigationWithBoundary);
        } else {
            callVoid(ports.requestNextActiveNavigationWithBoundary);
        }
        return true;
    }

    if (!callBool(ports.imageAvailable)) {
        return false;
    }

    const KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction action
        = m_navigationPolicy.horizontalArrowAction(leftArrow,
            callBool(ports.imageViewportHorizontallyPannable), input.rightToLeftReadingActive);
    switch (action) {
    case KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction::PanLeft:
        panBy(ports, -keyboardPanDistance, 0.0);
        return true;
    case KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction::PanRight:
        panBy(ports, keyboardPanDistance, 0.0);
        return true;
    case KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction::
        RequestPreviousActiveNavigation:
        callVoid(ports.requestPreviousActiveNavigationWithBoundary);
        return true;
    case KiriView::ImageShortcutNavigationPolicy::HorizontalArrowAction::
        RequestNextActiveNavigation:
        callVoid(ports.requestNextActiveNavigationWithBoundary);
        return true;
    }

    return false;
}

bool ApplicationCommandRouter::executeSinglePageArrowShortcut(
    const ApplicationCommandRouterInput &input, const ApplicationCommandRouterPorts &ports,
    bool leftArrow) const
{
    if (!callBool(ports.imageAvailable)) {
        return false;
    }

    const KiriView::ImageShortcutNavigationPolicy::SinglePageArrowAction action
        = m_navigationPolicy.singlePageArrowAction(leftArrow, input.rightToLeftReadingActive);
    switch (action) {
    case KiriView::ImageShortcutNavigationPolicy::SinglePageArrowAction::OpenPreviousSinglePage:
        callVoid(ports.openPreviousSinglePage);
        return true;
    case KiriView::ImageShortcutNavigationPolicy::SinglePageArrowAction::OpenNextSinglePage:
        callVoid(ports.openNextSinglePage);
        return true;
    }

    return false;
}

bool ApplicationCommandRouter::executeVerticalPanShortcut(const ApplicationCommandRouterInput &,
    const ApplicationCommandRouterPorts &ports, bool up) const
{
    if (!callBool(ports.imageAvailable)) {
        return false;
    }

    panBy(ports, 0.0, up ? -keyboardPanDistance : keyboardPanDistance);
    return true;
}

bool ApplicationCommandRouter::executeVideoSeekShortcut(const ApplicationCommandRouterInput &input,
    const ApplicationCommandRouterPorts &ports, qint64 deltaMilliseconds) const
{
    if (!input.videoMode || !callBool(ports.videoAvailable)) {
        return false;
    }

    if (callBool(ports.videoSeekable) && ports.seekVideoBy) {
        ports.seekVideoBy(deltaMilliseconds);
    }
    return true;
}
}
