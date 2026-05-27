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
    required property KiriDocumentSession documentSession
    required property KiriImageDocument imageDocument
    required property var imageViewport
    property bool handleMenuPresentationShortcut: true
    property bool videoFileDeletionInProgress: false
    property bool videoMode: false

    readonly property int keyboardPanDistance: 64
    readonly property bool activeNavigationActionsAvailable: root.documentSession.activeNavigationAvailable && root.documentSession.activeNavigationKnown && !root.documentSession.fileDeletionInProgress && root.actionAvailability.helpShortcutsEnabled
    readonly property bool horizontalArrowShortcutsEnabled: root.application.mediaHorizontalArrowShortcutsEnabled(root.videoMode, root.actionAvailability.readyViewerShortcutsEnabled, root.actionAvailability.viewerShortcutsEnabled, root.activeNavigationActionsAvailable, root.videoFileDeletionInProgress)

    readonly property var zoomInQAction: root.application.actionForId(KiriViewApplication.ViewZoomInAction)
    readonly property var zoomOutQAction: root.application.actionForId(KiriViewApplication.ViewZoomOutAction)
    readonly property var panTopLeftQAction: root.application.actionForId(KiriViewApplication.ViewPanTopLeftAction)
    readonly property var panBottomRightQAction: root.application.actionForId(KiriViewApplication.ViewPanBottomRightAction)
    readonly property var scanForwardQAction: root.application.actionForId(KiriViewApplication.ViewScanForwardAction)
    readonly property var scanBackwardQAction: root.application.actionForId(KiriViewApplication.ViewScanBackwardAction)
    readonly property var showMenubarQAction: root.application.actionForId(KiriViewApplication.OptionsShowMenubarAction)
    signal imageBoundaryReached(string message)
    signal unsupportedVideoActionRequested

    function activeNavigationShortcutsEnabledForScope(shortcutScope) {
        switch (shortcutScope) {
        case ImageActionAvailability.ImageSelectionShortcutScope:
        case ImageActionAvailability.PageShortcutScope:
            return root.activeNavigationActionsAvailable;
        case ImageActionAvailability.ImageSelectionViewerShortcutScope:
        case ImageActionAvailability.PageViewerShortcutScope:
            return root.activeNavigationActionsAvailable && root.actionAvailability.viewerShortcutsEnabled;
        default:
            return null;
        }
    }

    function shortcutsEnabledForScope(shortcutScope, availabilityRevision) {
        const activeNavigationEnabled = root.activeNavigationShortcutsEnabledForScope(shortcutScope);
        if (activeNavigationEnabled !== null) {
            return activeNavigationEnabled;
        }

        if (root.videoMode) {
            return root.application.videoShortcutsEnabledForScope(shortcutScope, root.actionAvailability.helpShortcutsEnabled, root.actionAvailability.viewerShortcutsEnabled, root.videoFileDeletionInProgress, root.activeNavigationActionsAvailable);
        }

        if (availabilityRevision < 0) {
            return false;
        }
        return root.actionAvailability.shortcutsEnabledForScope(shortcutScope);
    }

    function unsupportedVideoAction(actionId) {
        return root.application.videoActionUnsupported(actionId);
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

    function requestPreviousActiveNavigation() {
        const boundaryText = root.documentSession.requestPreviousActiveNavigationBoundaryText();
        if (boundaryText.length > 0) {
            root.imageBoundaryReached(boundaryText);
        }
    }

    function requestNextActiveNavigation() {
        const boundaryText = root.documentSession.requestNextActiveNavigationBoundaryText();
        if (boundaryText.length > 0) {
            root.imageBoundaryReached(boundaryText);
        }
    }

    function applyHorizontalArrowAction(action) {
        switch (action) {
        case ImageShortcutNavigationPolicy.PanLeft:
            root.panBy(-root.keyboardPanDistance, 0);
            return;
        case ImageShortcutNavigationPolicy.PanRight:
            root.panBy(root.keyboardPanDistance, 0);
            return;
        case ImageShortcutNavigationPolicy.RequestPreviousActiveNavigation:
            root.requestPreviousActiveNavigation();
            return;
        case ImageShortcutNavigationPolicy.RequestNextActiveNavigation:
            root.requestNextActiveNavigation();
            return;
        }
    }

    function applySinglePageArrowAction(action) {
        switch (action) {
        case ImageShortcutNavigationPolicy.OpenPreviousSinglePage:
            // Image-internal single-page stepping preserves spread-local movement without changing
            // the shared Previous/Next action route.
            root.imageDocument.openPreviousSinglePage();
            return;
        case ImageShortcutNavigationPolicy.OpenNextSinglePage:
            // Image-internal single-page stepping preserves spread-local movement without changing
            // the shared Previous/Next action route.
            root.imageDocument.openNextSinglePage();
            return;
        }
    }

    function applyScanAction(action) {
        switch (action) {
        case ImageShortcutNavigationPolicy.NoScanAction:
            return;
        case ImageShortcutNavigationPolicy.RequestPreviousActiveNavigationFromScan:
            root.requestPreviousActiveNavigation();
            return;
        case ImageShortcutNavigationPolicy.RequestNextActiveNavigationFromScan:
            root.requestNextActiveNavigation();
            return;
        case ImageShortcutNavigationPolicy.OpenPreviousPageFromFinalScanStart:
            // Image-internal scan fallback intentionally opens the previous document page and
            // hands off the viewport start. Shared active Previous/Next routing uses the session.
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
                root.requestPreviousActiveNavigation();
                return;
            }

            root.requestNextActiveNavigation();
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
        const action = navigationPolicy.scanBackwardAction(root.actionAvailability.imagePannable, root.actionAvailability.imagePannable && root.imageViewport.scanBackward(), root.actionAvailability.scanBackwardAtFirstImageBoundary, root.imageDocument.currentPageNumber);
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
        model: root.application.shortcutRouteModel

        ActionShortcutGroup {
            required property int shortcutScope

            application: root.application
            interceptShortcut: actionId => root.videoMode && root.unsupportedVideoAction(actionId)
            shortcutsEnabled: root.shortcutsEnabledForScope(shortcutScope, root.actionAvailability.availabilityRevision)

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
