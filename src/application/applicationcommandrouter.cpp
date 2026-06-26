// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationcommandrouter.h"

#include "applicationzoompresets.h"

namespace {
constexpr double keyboardPanDistance = 64.0;

bool callBool(const std::function<bool()>& callback) { return callback && callback(); }

void callVoid(const std::function<void()>& callback)
{
    if (callback) {
        callback();
    }
}

void callDouble(const std::function<void(double)>& callback, double value)
{
    if (callback) {
        callback(value);
    }
}

qint64 callInt64(const std::function<qint64()>& callback) { return callback ? callback() : 0; }

void callInt64(const std::function<void(qint64)>& callback, qint64 value)
{
    if (callback) {
        callback(value);
    }
}

void panBy(
    const kiriview::ApplicationActions::ApplicationCommandRouterPorts& ports, double dx, double dy)
{
    if (ports.imagePresentation.requestViewportPanBy) {
        ports.imagePresentation.requestViewportPanBy(dx, dy);
    }
}
}

namespace kiriview::ApplicationActions {
void ApplicationCommandRouter::handleActionTriggered(ActionId actionId,
    const ApplicationCommandRouterInput& input, const ApplicationCommandRouterPorts& ports) const
{
    switch (actionId) {
    case ActionId::FileOpenAction:
        callVoid(ports.shell.requestOpenDialog);
        return;
    case ActionId::FileOpenWithAction:
        callVoid(ports.session.openCurrentMediaWith);
        return;
    case ActionId::FileMoveToTrashAction:
        callVoid(ports.session.moveDisplayedFileToTrash);
        return;
    case ActionId::FileDeleteAction:
        callVoid(ports.session.deleteDisplayedFilePermanently);
        return;
    case ActionId::GoPreviousArchiveAction:
        callVoid(ports.imageDocument.openPreviousContainer);
        return;
    case ActionId::GoNextArchiveAction:
        callVoid(ports.imageDocument.openNextContainer);
        return;
    case ActionId::GoPreviousImageAction:
        callVoid(ports.session.requestPreviousActiveNavigationWithBoundary);
        return;
    case ActionId::GoNextImageAction:
        callVoid(ports.session.requestNextActiveNavigationWithBoundary);
        return;
    case ActionId::GoFirstImageAction:
        callVoid(ports.session.openFirstActiveNavigation);
        return;
    case ActionId::GoLastImageAction:
        callVoid(ports.session.openLastActiveNavigation);
        return;
    case ActionId::ViewZoomInAction:
        callDouble(ports.imagePresentation.requestZoomByStepAtCenter, 1.0);
        return;
    case ActionId::ViewZoomOutAction:
        callDouble(ports.imagePresentation.requestZoomByStepAtCenter, -1.0);
        return;
    case ActionId::ViewZoom50PercentAction:
    case ActionId::ViewZoom100PercentAction:
    case ActionId::ViewZoom200PercentAction:
        if (const ZoomPresetDescriptor* preset = zoomPresetDescriptorForAction(actionId)) {
            callDouble(ports.imagePresentation.requestManualZoomPercent, preset->zoomPercent);
        }
        return;
    case ActionId::ViewFitAction:
        callVoid(ports.imagePresentation.requestFitMode);
        return;
    case ActionId::ViewFitHeightAction:
        callVoid(ports.imagePresentation.requestFitHeightMode);
        return;
    case ActionId::ViewFitWidthAction:
        callVoid(ports.imagePresentation.requestFitWidthMode);
        return;
    case ActionId::ViewRotateClockwiseAction:
        callVoid(ports.imageDocument.rotateClockwise);
        return;
    case ActionId::ViewRotateCounterclockwiseAction:
        callVoid(ports.imageDocument.rotateCounterclockwise);
        return;
    case ActionId::ViewToggleTwoPageModeAction:
        callVoid(ports.imageDocument.requestToggleTwoPageMode);
        return;
    case ActionId::ViewToggleRightToLeftReadingAction:
        callVoid(ports.imageDocument.requestToggleRightToLeftReading);
        return;
    case ActionId::ViewToggleInfoPanelAction:
        callVoid(ports.panel.toggleInfoPanel);
        return;
    case ActionId::ViewToggleThumbnailPanelAction:
        callVoid(ports.panel.toggleThumbnailPanel);
        return;
    case ActionId::ViewGoToContentStartAction:
        if (input.videoMode) {
            if (callBool(ports.video.videoAvailable) && callBool(ports.video.videoSeekable)) {
                callInt64(ports.video.setVideoPosition, 0);
            }
            return;
        }
        callVoid(ports.imagePresentation.requestViewportPanToInitialScanPosition);
        return;
    case ActionId::ViewGoToContentEndAction:
        if (input.videoMode) {
            const qint64 duration = callInt64(ports.video.videoDuration);
            if (callBool(ports.video.videoAvailable) && callBool(ports.video.videoSeekable)
                && duration > 0) {
                callInt64(ports.video.setVideoPosition, duration);
            }
            return;
        }
        callVoid(ports.imagePresentation.requestViewportPanToFinalScanPosition);
        return;
    case ActionId::ViewScanForwardAction:
        handleScanForwardAction(input, ports);
        return;
    case ActionId::ViewScanBackwardAction:
        handleScanBackwardAction(input, ports);
        return;
    case ActionId::ViewToggleVideoPlaybackAction:
        if (input.videoMode && callBool(ports.video.videoAvailable)) {
            callVoid(ports.video.toggleVideoPlayback);
        }
        return;
    case ActionId::WindowFullscreenAction:
        callVoid(ports.window.toggleFullScreen);
        return;
    case ActionId::HelpShortcutsAction:
        callVoid(ports.help.requestShortcutHelp);
        return;
    case ActionId::OpenApplicationMenuAction:
        callVoid(ports.shell.openApplicationMenu);
        return;
    case ActionId::OptionsConfigureKeybindingAction:
    case ActionId::OptionsShowMenubarAction:
    case ActionId::FileQuitAction:
    case ActionId::ActionCount:
        return;
    }
}

void ApplicationCommandRouter::handleScanForwardAction(
    const ApplicationCommandRouterInput& input, const ApplicationCommandRouterPorts& ports) const
{
    if (input.videoMode) {
        callVoid(ports.session.requestNextActiveNavigationWithBoundary);
        return;
    }

    const bool viewportMoved = callBool(ports.imagePresentation.requestViewportScanForward);
    const kiriview::ImageShortcutNavigationPolicy::ScanAction action
        = m_navigationPolicy.scanForwardAction(input.imagePannable, viewportMoved);
    switch (action) {
    case kiriview::ImageShortcutNavigationPolicy::ScanAction::RequestNextActiveNavigationFromScan:
        callVoid(ports.session.requestNextActiveNavigationWithBoundary);
        return;
    default:
        return;
    }
}

void ApplicationCommandRouter::handleScanBackwardAction(
    const ApplicationCommandRouterInput& input, const ApplicationCommandRouterPorts& ports) const
{
    if (input.videoMode) {
        callVoid(ports.session.requestPreviousActiveNavigationWithBoundary);
        return;
    }

    const bool viewportMoved = callBool(ports.imagePresentation.requestViewportScanBackward);
    const kiriview::ImageShortcutNavigationPolicy::ScanAction action
        = m_navigationPolicy.scanBackwardAction(input.imagePannable, viewportMoved,
            input.imageDocumentPageNavigationActive, input.atKnownFirstActiveNavigation,
            input.canOpenPreviousActiveNavigation);
    switch (action) {
    case kiriview::ImageShortcutNavigationPolicy::ScanAction::
        RequestPreviousActiveNavigationFromScan:
        callVoid(ports.session.requestPreviousActiveNavigationWithBoundary);
        return;
    case kiriview::ImageShortcutNavigationPolicy::ScanAction::OpenPreviousPageFromFinalScanStart:
        callVoid(ports.imagePresentation.requestNextDisplayedImageStartToFinalScanPosition);
        callVoid(ports.session.requestPreviousActiveNavigationWithBoundary);
        return;
    case kiriview::ImageShortcutNavigationPolicy::ScanAction::ShowFirstImageBoundary:
        callVoid(ports.session.showFirstImageBoundary);
        return;
    default:
        return;
    }
}

bool ApplicationCommandRouter::executeHorizontalArrowShortcut(
    const ApplicationCommandRouterInput& input, const ApplicationCommandRouterPorts& ports,
    bool leftArrow) const
{
    if (input.videoMode) {
        if (leftArrow) {
            callVoid(ports.session.requestPreviousActiveNavigationWithBoundary);
        } else {
            callVoid(ports.session.requestNextActiveNavigationWithBoundary);
        }
        return true;
    }

    if (!callBool(ports.imageDocument.imageAvailable)) {
        return false;
    }

    const kiriview::ImageShortcutNavigationPolicy::HorizontalArrowAction action
        = m_navigationPolicy.horizontalArrowAction(leftArrow,
            callBool(ports.imagePresentation.imageViewportHorizontallyPannable),
            input.rightToLeftReadingActive);
    switch (action) {
    case kiriview::ImageShortcutNavigationPolicy::HorizontalArrowAction::PanLeft:
        panBy(ports, -keyboardPanDistance, 0.0);
        return true;
    case kiriview::ImageShortcutNavigationPolicy::HorizontalArrowAction::PanRight:
        panBy(ports, keyboardPanDistance, 0.0);
        return true;
    case kiriview::ImageShortcutNavigationPolicy::HorizontalArrowAction::
        RequestPreviousActiveNavigation:
        callVoid(ports.session.requestPreviousActiveNavigationWithBoundary);
        return true;
    case kiriview::ImageShortcutNavigationPolicy::HorizontalArrowAction::
        RequestNextActiveNavigation:
        callVoid(ports.session.requestNextActiveNavigationWithBoundary);
        return true;
    }

    return false;
}

bool ApplicationCommandRouter::executeSinglePageArrowShortcut(
    const ApplicationCommandRouterInput& input, const ApplicationCommandRouterPorts& ports,
    bool leftArrow) const
{
    if (!callBool(ports.imageDocument.imageAvailable)) {
        return false;
    }

    const kiriview::ImageShortcutNavigationPolicy::SinglePageArrowAction action
        = m_navigationPolicy.singlePageArrowAction(leftArrow, input.rightToLeftReadingActive);
    switch (action) {
    case kiriview::ImageShortcutNavigationPolicy::SinglePageArrowAction::OpenPreviousSinglePage:
        callVoid(ports.imageDocument.openPreviousSinglePage);
        return true;
    case kiriview::ImageShortcutNavigationPolicy::SinglePageArrowAction::OpenNextSinglePage:
        callVoid(ports.imageDocument.openNextSinglePage);
        return true;
    }

    return false;
}

bool ApplicationCommandRouter::executeVerticalPanShortcut(
    const ApplicationCommandRouterInput&, const ApplicationCommandRouterPorts& ports, bool up) const
{
    if (!callBool(ports.imageDocument.imageAvailable)) {
        return false;
    }

    panBy(ports, 0.0, up ? -keyboardPanDistance : keyboardPanDistance);
    return true;
}

bool ApplicationCommandRouter::executeVideoSeekShortcut(const ApplicationCommandRouterInput& input,
    const ApplicationCommandRouterPorts& ports, qint64 deltaMilliseconds) const
{
    if (!input.videoMode || !callBool(ports.video.videoAvailable)) {
        return false;
    }

    if (callBool(ports.video.videoSeekable) && ports.video.seekVideoBy) {
        ports.video.seekVideoBy(deltaMilliseconds);
    }
    return true;
}
}
