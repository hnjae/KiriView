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
    required property ImageViewportInteractionSurface imageInteractionSurface
    property bool applicationMenuShortcutEnabled: false
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

    readonly property var zoomInQAction: root.application.actionForId(KiriViewApplication.ViewZoomInAction)
    readonly property var zoomOutQAction: root.application.actionForId(KiriViewApplication.ViewZoomOutAction)
    readonly property var panTopLeftQAction: root.application.actionForId(KiriViewApplication.ViewPanTopLeftAction)
    readonly property var panBottomRightQAction: root.application.actionForId(KiriViewApplication.ViewPanBottomRightAction)
    readonly property var scanForwardQAction: root.application.actionForId(KiriViewApplication.ViewScanForwardAction)
    readonly property var scanBackwardQAction: root.application.actionForId(KiriViewApplication.ViewScanBackwardAction)
    signal imageBoundaryReached(string message)
    signal unsupportedVideoActionRequested

    function shortcutsEnabledForScope(shortcutScope, availabilityRevision) {
        if (availabilityRevision < 0) {
            return false;
        }
        return root.actionAvailability.mediaShortcutsEnabledForScope(shortcutScope, root.videoMode, root.activeNavigationActionsAvailable, root.videoFileDeletionInProgress);
    }

    function unsupportedVideoAction(actionId) {
        return root.application.videoActionUnsupported(actionId);
    }

    function publishActionState() {
        const zoomMode = root.imageDocument !== null && root.imageDocument !== undefined ? root.imageDocument.zoomMode : -1;
        root.application.updateActionState(root.actionAvailability.helpShortcutsEnabled, root.actionAvailability.canUseReadyActions, root.actionAvailability.canUseRotateActions, root.actionAvailability.canUseTwoPageModeActions, root.actionAvailability.canUseRightToLeftReadingActions, root.actionAvailability.containerShortcutsEnabled, root.documentSession.displayedMediaOpenWithAvailable, root.documentSession.displayedFileDeletionAvailable, root.documentSession.fileDeletionInProgress, root.documentSession.activeNavigationAvailable, root.documentSession.activeNavigationKnown, root.documentSession.activeNavigationCount > 0, root.documentSession.canOpenPreviousActiveNavigation, root.documentSession.canOpenNextActiveNavigation, root.imageMode && zoomMode === KiriImageDocument.Fit, root.imageMode && zoomMode === KiriImageDocument.FitHeight, root.imageMode && zoomMode === KiriImageDocument.FitWidth, root.actionAvailability.twoPageModeActive, root.actionAvailability.rightToLeftReadingActive, root.infoPanelVisible, root.thumbnailPanelVisible, root.fullscreen, root.applicationMenuShortcutEnabled, root.showMenubarActionEnabled);
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

    ImageShortcutNavigationPolicy {
        id: navigationPolicy
    }

    Component.onCompleted: root.publishActionState()

    onApplicationMenuShortcutEnabledChanged: root.publishActionState()
    onFullscreenChanged: root.publishActionState()
    onInfoPanelVisibleChanged: root.publishActionState()
    onShowMenubarActionEnabledChanged: root.publishActionState()
    onThumbnailPanelVisibleChanged: root.publishActionState()
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
