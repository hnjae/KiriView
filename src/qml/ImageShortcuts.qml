// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import io.github.hnjae.kiriview
import org.kde.ki18n

Item {
    id: root

    objectName: "imageShortcuts"

    required property KiriViewApplication application
    required property ImageActionAvailability actionAvailability
    required property KiriDocumentSession documentSession
    required property KiriImageDocument imageDocument
    required property ImageViewportInteractionSurface imageInteractionSurface
    property bool applicationMenuShortcutEnabled: true
    property bool fullscreen: false
    property bool infoPanelVisible: false
    property bool showMenubarActionEnabled: true
    property bool thumbnailPanelVisible: false
    property bool videoFileDeletionInProgress: false
    property bool videoMode: false

    readonly property int keyboardPanDistance: 64
    readonly property bool activeNavigationActionsAvailable: root.documentSession.activeNavigationAvailable && root.documentSession.activeNavigationKnown && !root.documentSession.fileDeletionInProgress && root.actionAvailability.helpShortcutsEnabled
    readonly property bool horizontalArrowShortcutsEnabled: root.application.mediaHorizontalArrowShortcutsEnabled(root.videoMode, root.actionAvailability.readyViewerShortcutsEnabled, root.actionAvailability.viewerShortcutsEnabled, root.activeNavigationActionsAvailable, root.videoFileDeletionInProgress)
    readonly property bool imageMode: root.documentSession.documentKind === KiriDocumentSession.Image

    signal imageBoundaryReached(string message)
    signal unsupportedVideoActionRequested

    function publishActionState() {
        const zoomMode = root.imageDocument !== null && root.imageDocument !== undefined ? root.imageDocument.zoomMode : -1;
        root.application.updateActionState(root.actionAvailability.helpShortcutsEnabled, root.actionAvailability.canUseReadyActions, root.actionAvailability.canUseRotateActions, root.actionAvailability.canUseTwoPageModeActions, root.actionAvailability.canUseRightToLeftReadingActions, root.actionAvailability.containerShortcutsEnabled, root.documentSession.displayedMediaOpenWithAvailable, root.documentSession.displayedFileDeletionAvailable, root.documentSession.fileDeletionInProgress, root.documentSession.activeNavigationAvailable, root.documentSession.activeNavigationKnown, root.documentSession.activeNavigationCount > 0, root.documentSession.canOpenPreviousActiveNavigation, root.documentSession.canOpenNextActiveNavigation, root.imageMode && zoomMode === KiriImageDocument.Fit, root.imageMode && zoomMode === KiriImageDocument.FitHeight, root.imageMode && zoomMode === KiriImageDocument.FitWidth, root.actionAvailability.twoPageModeActive, root.actionAvailability.rightToLeftReadingActive, root.infoPanelVisible, root.thumbnailPanelVisible, root.fullscreen, root.applicationMenuShortcutEnabled, root.showMenubarActionEnabled, root.documentSession.activeNavigationBoundaryScope === KiriDocumentSession.DirectMediaNavigationBoundary, root.actionAvailability.viewerShortcutsEnabled, root.actionAvailability.readyShortcutsEnabled, root.actionAvailability.readyViewerShortcutsEnabled, root.actionAvailability.twoPageViewerShortcutsEnabled, root.actionAvailability.rightToLeftReadingShortcutsEnabled, root.actionAvailability.rightToLeftReadingViewerShortcutsEnabled, root.actionAvailability.rotateShortcutsEnabled, root.actionAvailability.rotateViewerShortcutsEnabled, root.actionAvailability.pannableShortcutsEnabled, root.actionAvailability.pannableViewerShortcutsEnabled, root.actionAvailability.containerViewerShortcutsEnabled, root.activeNavigationActionsAvailable, root.videoMode, root.videoFileDeletionInProgress);
    }

    function panBy(deltaX, deltaY) {
        return imageInteractionSurface.panBy(deltaX, deltaY);
    }

    function panToBottomRight() {
        return imageInteractionSurface.panToBottomRight();
    }

    function panToTopLeft() {
        return imageInteractionSurface.panToTopLeft();
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
            root.imageInteractionSurface.setNextDisplayedImageStartToFinalScanPosition();
            root.requestPreviousActiveNavigation();
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

        const action = navigationPolicy.horizontalArrowAction(leftArrow, root.imageInteractionSurface.imageHorizontallyPannable, root.actionAvailability.rightToLeftReadingActive);
        root.applyHorizontalArrowAction(action);
    }

    function handleSinglePageArrow(leftArrow) {
        const action = navigationPolicy.singlePageArrowAction(leftArrow, root.actionAvailability.rightToLeftReadingActive);
        root.applySinglePageArrowAction(action);
    }

    function scanForward() {
        const action = navigationPolicy.scanForwardAction(root.actionAvailability.imagePannable, root.actionAvailability.imagePannable && root.imageInteractionSurface.scanForward());
        root.applyScanAction(action);
    }

    function scanBackward() {
        const imageDocumentPageNavigationActive = root.documentSession.activeNavigationBoundaryScope === KiriDocumentSession.ImageDocumentPageNavigationBoundary;
        const action = navigationPolicy.scanBackwardAction(root.actionAvailability.imagePannable, root.actionAvailability.imagePannable && root.imageInteractionSurface.scanBackward(), imageDocumentPageNavigationActive, root.documentSession.atKnownFirstActiveNavigation, root.documentSession.canOpenPreviousActiveNavigation);
        root.applyScanAction(action);
    }

    function zoomByStep(stepCount, viewportX, viewportY) {
        return imageInteractionSurface.zoomByStep(stepCount, viewportX, viewportY);
    }

    function zoomByStepAtCenter(stepCount) {
        return root.imageInteractionSurface.zoomByStepAtCenter(stepCount);
    }

    function dispatchAction(actionId) {
        switch (actionId) {
        case KiriViewApplication.ViewZoomInAction:
            root.zoomByStepAtCenter(1);
            return;
        case KiriViewApplication.ViewZoomOutAction:
            root.zoomByStepAtCenter(-1);
            return;
        case KiriViewApplication.ViewPanTopLeftAction:
            root.panToTopLeft();
            return;
        case KiriViewApplication.ViewPanBottomRightAction:
            root.panToBottomRight();
            return;
        case KiriViewApplication.ViewScanForwardAction:
            root.scanForward();
            return;
        case KiriViewApplication.ViewScanBackwardAction:
            root.scanBackward();
            return;
        }
    }

    ImageShortcutNavigationPolicy {
        id: navigationPolicy
    }

    Component.onCompleted: {
        root.application.setShortcutHost(root.Window.window ? root.Window.window : root);
        root.publishActionState();
    }

    onApplicationMenuShortcutEnabledChanged: root.publishActionState()
    onFullscreenChanged: root.publishActionState()
    onInfoPanelVisibleChanged: root.publishActionState()
    onShowMenubarActionEnabledChanged: root.publishActionState()
    onThumbnailPanelVisibleChanged: root.publishActionState()
    onVideoFileDeletionInProgressChanged: root.publishActionState()
    onVideoModeChanged: root.publishActionState()

    Connections {
        target: root.actionAvailability

        function onAvailabilityChanged() {
            root.publishActionState();
        }
    }

    Connections {
        target: root.documentSession

        function onActiveNavigationChanged() {
            root.publishActionState();
        }

        function onDisplayedFileDeletionAvailabilityChanged() {
            root.publishActionState();
        }

        function onDisplayedMediaOpenWithAvailabilityChanged() {
            root.publishActionState();
        }

        function onDocumentKindChanged() {
            root.publishActionState();
        }

        function onFileDeletionInProgressChanged() {
            root.publishActionState();
        }
    }

    Connections {
        target: root.imageDocument

        function onZoomModeChanged() {
            root.publishActionState();
        }
    }

    Connections {
        target: root.application

        function onActionTriggered(actionId) {
            root.dispatchAction(actionId);
        }

        function onUnsupportedVideoActionTriggered(actionId) {
            root.unsupportedVideoActionRequested();
        }
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
}
