// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import io.github.hnjae.kiriview
import org.kde.ki18n

Item {
    id: root

    required property KiriViewApplication application
    required property ImageActionAvailability actionAvailability
    required property KiriImageDocument imageDocument
    required property var imageViewport
    property bool handleMenuPresentationShortcut: true
    property bool videoFileDeletionInProgress: false
    property bool videoMediaNavigationActive: false
    property bool videoMode: false

    readonly property int keyboardPanDistance: 64
    readonly property bool horizontalArrowShortcutsEnabled: videoMode ? videoMediaNavigationActive && !videoFileDeletionInProgress && actionAvailability.viewerShortcutsEnabled : actionAvailability.readyViewerShortcutsEnabled

    readonly property var previousImageQAction: root.application.actionForId(KiriViewApplication.GoPreviousImageAction)
    readonly property var nextImageQAction: root.application.actionForId(KiriViewApplication.GoNextImageAction)
    readonly property var zoomInQAction: root.application.actionForId(KiriViewApplication.ViewZoomInAction)
    readonly property var zoomOutQAction: root.application.actionForId(KiriViewApplication.ViewZoomOutAction)
    readonly property var panTopLeftQAction: root.application.actionForId(KiriViewApplication.ViewPanTopLeftAction)
    readonly property var panBottomRightQAction: root.application.actionForId(KiriViewApplication.ViewPanBottomRightAction)
    readonly property var scanForwardQAction: root.application.actionForId(KiriViewApplication.ViewScanForwardAction)
    readonly property var scanBackwardQAction: root.application.actionForId(KiriViewApplication.ViewScanBackwardAction)
    readonly property var showMenubarQAction: root.application.actionForId(KiriViewApplication.OptionsShowMenubarAction)
    readonly property var actionShortcutRoutes: root.application.shortcutRoutes()

    signal imageBoundaryReached(string message)
    signal unsupportedVideoActionRequested

    function shortcutsEnabledForScope(shortcutScope, availabilityRevision) {
        if (root.videoMode) {
            return root.videoShortcutsEnabledForScope(shortcutScope);
        }

        if (availabilityRevision < 0) {
            return false;
        }
        return root.actionAvailability.shortcutsEnabledForScope(shortcutScope);
    }

    function unsupportedVideoAction(actionId) {
        switch (actionId) {
        case KiriViewApplication.GoPreviousArchiveAction:
        case KiriViewApplication.GoNextArchiveAction:
        case KiriViewApplication.ViewZoomInAction:
        case KiriViewApplication.ViewZoomOutAction:
        case KiriViewApplication.ViewFitAction:
        case KiriViewApplication.ViewFitHeightAction:
        case KiriViewApplication.ViewFitWidthAction:
        case KiriViewApplication.ViewActualSizeAction:
        case KiriViewApplication.ViewRotateClockwiseAction:
        case KiriViewApplication.ViewRotateCounterclockwiseAction:
        case KiriViewApplication.ViewToggleTwoPageModeAction:
        case KiriViewApplication.ViewToggleRightToLeftReadingAction:
        case KiriViewApplication.ViewPanTopLeftAction:
        case KiriViewApplication.ViewPanBottomRightAction:
        case KiriViewApplication.ViewScanForwardAction:
        case KiriViewApplication.ViewScanBackwardAction:
            return true;
        default:
            return false;
        }
    }

    function videoShortcutsEnabledForScope(shortcutScope) {
        const helpShortcutsEnabled = root.actionAvailability.helpShortcutsEnabled;
        const viewerShortcutsEnabled = root.actionAvailability.viewerShortcutsEnabled;
        const videoReadyShortcutsEnabled = helpShortcutsEnabled && !root.videoFileDeletionInProgress;
        const videoReadyViewerShortcutsEnabled = viewerShortcutsEnabled && !root.videoFileDeletionInProgress;
        const videoMediaShortcutsEnabled = root.videoMediaNavigationActive && videoReadyShortcutsEnabled;
        const videoMediaViewerShortcutsEnabled = root.videoMediaNavigationActive && videoReadyViewerShortcutsEnabled;

        switch (shortcutScope) {
        case ImageActionAvailability.HelpShortcutScope:
            return helpShortcutsEnabled;
        case ImageActionAvailability.ViewerShortcutScope:
            return viewerShortcutsEnabled;
        case ImageActionAvailability.ReadyShortcutScope:
        case ImageActionAvailability.RotateShortcutScope:
        case ImageActionAvailability.PannableShortcutScope:
        case ImageActionAvailability.ContainerShortcutScope:
        case ImageActionAvailability.RightToLeftReadingShortcutScope:
            return videoReadyShortcutsEnabled;
        case ImageActionAvailability.ReadyViewerShortcutScope:
        case ImageActionAvailability.RotateViewerShortcutScope:
        case ImageActionAvailability.PannableViewerShortcutScope:
        case ImageActionAvailability.ContainerViewerShortcutScope:
        case ImageActionAvailability.RightToLeftReadingViewerShortcutScope:
            return videoReadyViewerShortcutsEnabled;
        case ImageActionAvailability.ImageSelectionShortcutScope:
        case ImageActionAvailability.PageShortcutScope:
            return videoMediaShortcutsEnabled;
        case ImageActionAvailability.ImageSelectionViewerShortcutScope:
        case ImageActionAvailability.PageViewerShortcutScope:
            return videoMediaViewerShortcutsEnabled;
        default:
            return false;
        }
    }

    function panBy(deltaX, deltaY) {
        return imageViewport.panBy(deltaX, deltaY);
    }

    function panToBottomRight() {
        return imageViewport.panToBottomRight();
    }

    function panToTopLeft() {
        return imageViewport.panToTopLeft();
    }

    function applyHorizontalArrowAction(action) {
        switch (action) {
        case ImageShortcutNavigationPolicy.PanLeft:
            root.panBy(-root.keyboardPanDistance, 0);
            return;
        case ImageShortcutNavigationPolicy.PanRight:
            root.panBy(root.keyboardPanDistance, 0);
            return;
        case ImageShortcutNavigationPolicy.OpenPreviousImage:
            root.previousImageQAction.trigger();
            return;
        case ImageShortcutNavigationPolicy.OpenNextImage:
            root.nextImageQAction.trigger();
            return;
        }
    }

    function applySinglePageArrowAction(action) {
        switch (action) {
        case ImageShortcutNavigationPolicy.OpenPreviousSinglePage:
            root.imageDocument.openPreviousSinglePage();
            return;
        case ImageShortcutNavigationPolicy.OpenNextSinglePage:
            root.imageDocument.openNextSinglePage();
            return;
        }
    }

    function applyScanAction(action) {
        switch (action) {
        case ImageShortcutNavigationPolicy.NoScanAction:
            return;
        case ImageShortcutNavigationPolicy.OpenPreviousImageFromScan:
            root.previousImageQAction.trigger();
            return;
        case ImageShortcutNavigationPolicy.OpenNextImageFromScan:
            root.nextImageQAction.trigger();
            return;
        case ImageShortcutNavigationPolicy.OpenPreviousPageFromFinalScanStart:
            root.imageViewport.setNextDisplayedImageStartToFinalScanPosition();
            root.imageDocument.openImageAtPage(root.imageDocument.currentPageNumber - 1);
            return;
        case ImageShortcutNavigationPolicy.ShowFirstImageBoundary:
            root.imageBoundaryReached(KI18n.i18nc("@info:status", "First image"));
            return;
        }
    }

    function handleHorizontalArrow(leftArrow) {
        if (root.videoMode) {
            if (leftArrow) {
                root.previousImageQAction.trigger();
                return;
            }

            root.nextImageQAction.trigger();
            return;
        }

        const action = navigationPolicy.horizontalArrowAction(leftArrow, root.actionAvailability.imageHorizontallyPannable, root.actionAvailability.rightToLeftReadingActive);
        root.applyHorizontalArrowAction(action);
    }

    function handleSinglePageArrow(leftArrow) {
        const action = navigationPolicy.singlePageArrowAction(leftArrow, root.actionAvailability.rightToLeftReadingActive);
        root.applySinglePageArrowAction(action);
    }

    function scanForward() {
        const action = navigationPolicy.scanForwardAction(root.actionAvailability.imagePannable, root.actionAvailability.imagePannable && root.imageViewport.scanForward());
        root.applyScanAction(action);
    }

    function scanBackward() {
        const action = navigationPolicy.scanBackwardAction(root.actionAvailability.imagePannable, root.actionAvailability.imagePannable && root.imageViewport.scanBackward(), root.actionAvailability.atKnownFirstImage, root.imageDocument.currentPageNumber);
        root.applyScanAction(action);
    }

    function zoomByStep(stepCount, viewportX, viewportY) {
        return imageViewport.zoomByStep(stepCount, viewportX, viewportY);
    }

    function zoomByStepAtCenter(stepCount) {
        return root.zoomByStep(stepCount, root.imageViewport.viewportWidth / 2, root.imageViewport.viewportHeight / 2);
    }

    ImageShortcutNavigationPolicy {
        id: navigationPolicy
    }

    Repeater {
        model: root.actionShortcutRoutes

        ActionShortcutGroup {
            required property var modelData

            actionIds: modelData.actionIds
            application: root.application
            interceptShortcut: actionId => root.videoMode && root.unsupportedVideoAction(actionId)
            shortcutFilter: modelData.shortcutFilter
            shortcutsEnabled: root.shortcutsEnabledForScope(modelData.shortcutScope, root.actionAvailability.availabilityRevision)

            onShortcutIntercepted: root.unsupportedVideoActionRequested()
        }
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.handleMenuPresentationShortcut && root.actionAvailability.helpShortcutsEnabled
        sequence: "Ctrl+M"

        onActivated: root.showMenubarQAction.trigger()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.horizontalArrowShortcutsEnabled
        sequence: "Left"

        onActivated: root.handleHorizontalArrow(true)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.horizontalArrowShortcutsEnabled
        sequence: "Right"

        onActivated: root.handleHorizontalArrow(false)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.actionAvailability.twoPageViewerShortcutsEnabled
        sequence: "Shift+Left"

        onActivated: root.handleSinglePageArrow(true)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.actionAvailability.twoPageViewerShortcutsEnabled
        sequence: "Shift+Right"

        onActivated: root.handleSinglePageArrow(false)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.actionAvailability.pannableViewerShortcutsEnabled
        sequence: "Up"

        onActivated: root.panBy(0, -root.keyboardPanDistance)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.actionAvailability.pannableViewerShortcutsEnabled
        sequence: "Down"

        onActivated: root.panBy(0, root.keyboardPanDistance)
    }

    ActionTrigger {
        action: root.zoomInQAction
        handler: () => root.zoomByStepAtCenter(1)
    }

    ActionTrigger {
        action: root.zoomOutQAction
        handler: () => root.zoomByStepAtCenter(-1)
    }

    ActionTrigger {
        action: root.panTopLeftQAction
        handler: () => root.panToTopLeft()
    }

    ActionTrigger {
        action: root.panBottomRightQAction
        handler: () => root.panToBottomRight()
    }

    ActionTrigger {
        action: root.scanForwardQAction
        handler: () => root.scanForward()
    }

    ActionTrigger {
        action: root.scanBackwardQAction
        handler: () => root.scanBackward()
    }
}
