// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import io.github.hnjae.kiriview

Item {
    id: root

    required property KiriViewApplication application
    required property KiriImageDocument imageDocument
    required property var imageViewport
    required property var imageToolBar
    required property bool helpDialogOpen

    readonly property bool imageReady: imageDocument.status === KiriImageDocument.Ready
    readonly property bool imagePannable: imageViewport.imagePannable
    readonly property bool atFirstImage: imageReady && imageDocument.currentPageNumber === 1 && imageDocument.imageCount > 0
    readonly property bool atLastImage: imageReady && imageDocument.currentPageNumber > 0 && imageDocument.currentPageNumber === imageDocument.imageCount
    readonly property int keyboardPanDistance: 64
    readonly property bool commandShortcutsEnabled: !root.textInputFocused() && !root.helpDialogOpen
    readonly property bool helpShortcutsEnabled: !root.helpDialogOpen
    readonly property bool readyShortcutsEnabled: root.imageReady && root.helpShortcutsEnabled
    readonly property bool readyCommandShortcutsEnabled: root.imageReady && root.commandShortcutsEnabled
    readonly property bool pageCommandShortcutsEnabled: readyCommandShortcutsEnabled && root.imageDocument.imageCount > 0
    readonly property bool pannableCommandShortcutsEnabled: root.imagePannable && root.commandShortcutsEnabled
    readonly property bool containerCommandShortcutsEnabled: root.imageDocument.containerNavigationAvailable && root.commandShortcutsEnabled
    readonly property int zoomStepPercent: imageDocument.zoomStepPercent

    readonly property var previousImageQAction: root.application.actionForId(KiriViewApplication.GoPreviousImageAction)
    readonly property var nextImageQAction: root.application.actionForId(KiriViewApplication.GoNextImageAction)
    readonly property var zoomInQAction: root.application.actionForId(KiriViewApplication.ViewZoomInAction)
    readonly property var zoomOutQAction: root.application.actionForId(KiriViewApplication.ViewZoomOutAction)
    readonly property var panLeftQAction: root.application.actionForId(KiriViewApplication.ViewPanLeftAction)
    readonly property var panRightQAction: root.application.actionForId(KiriViewApplication.ViewPanRightAction)
    readonly property var panUpQAction: root.application.actionForId(KiriViewApplication.ViewPanUpAction)
    readonly property var panDownQAction: root.application.actionForId(KiriViewApplication.ViewPanDownAction)
    readonly property var panTopLeftQAction: root.application.actionForId(KiriViewApplication.ViewPanTopLeftAction)
    readonly property var panBottomRightQAction: root.application.actionForId(KiriViewApplication.ViewPanBottomRightAction)
    readonly property var scanForwardQAction: root.application.actionForId(KiriViewApplication.ViewScanForwardAction)
    readonly property var scanBackwardQAction: root.application.actionForId(KiriViewApplication.ViewScanBackwardAction)

    signal imageBoundaryReached(string message)

    function panBy(deltaX, deltaY) {
        return imageViewport.panBy(deltaX, deltaY);
    }

    function panToBottomRight() {
        return imageViewport.panToBottomRight();
    }

    function panToTopLeft() {
        return imageViewport.panToTopLeft();
    }

    function scanForward() {
        if (!root.imagePannable) {
            root.nextImageQAction.trigger();
            return;
        }

        if (root.imageViewport.scanForward()) {
            return;
        }

        root.nextImageQAction.trigger();
    }

    function scanBackward() {
        if (!root.imagePannable) {
            root.previousImageQAction.trigger();
            return;
        }

        if (root.imageViewport.scanBackward()) {
            return;
        }

        if (root.atFirstImage) {
            root.imageBoundaryReached("First image");
            return;
        }

        if (root.imageDocument.currentPageNumber > 1) {
            root.imageViewport.setNextDisplayedImageStartToFinalScanPosition();
            root.imageDocument.openImageAtPage(root.imageDocument.currentPageNumber - 1);
            return;
        }

        root.previousImageQAction.trigger();
    }

    function textInputFocused() {
        return imageToolBar.textInputFocused();
    }

    function zoomBy(deltaPercent, viewportX, viewportY) {
        return imageViewport.zoomBy(deltaPercent, viewportX, viewportY);
    }

    function zoomByAtCenter(deltaPercent) {
        return root.zoomBy(deltaPercent, root.imageViewport.viewportWidth / 2, root.imageViewport.viewportHeight / 2);
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileOpenAction]
        application: root.application
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileQuitAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.commandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileQuitAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewZoomInAction, KiriViewApplication.ViewZoomOutAction, KiriViewApplication.ViewFitAction, KiriViewApplication.ViewFitHeightAction, KiriViewApplication.ViewFitWidthAction, KiriViewApplication.ViewActualSizeAction]
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewPanLeftAction, KiriViewApplication.ViewPanRightAction, KiriViewApplication.ViewPanUpAction, KiriViewApplication.ViewPanDownAction, KiriViewApplication.ViewPanTopLeftAction, KiriViewApplication.ViewPanBottomRightAction]
        application: root.application
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewScanForwardAction, KiriViewApplication.ViewScanBackwardAction]
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousImageAction, KiriViewApplication.GoNextImageAction]
        application: root.application
        shortcutsEnabled: root.readyShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoFirstImageAction, KiriViewApplication.GoLastImageAction]
        application: root.application
        shortcutsEnabled: root.pageCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousArchiveAction, KiriViewApplication.GoNextArchiveAction]
        application: root.application
        shortcutsEnabled: root.containerCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.WindowFullscreenAction, KiriViewApplication.HelpShortcutsAction]
        application: root.application
        shortcutsEnabled: root.commandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.OptionsConfigureAction, KiriViewApplication.OptionsConfigureKeybindingAction, KiriViewApplication.OptionsShowMenubarAction]
        application: root.application
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    Connections {
        target: root.zoomInQAction

        function onTriggered() {
            root.zoomByAtCenter(root.zoomStepPercent);
        }
    }

    Connections {
        target: root.zoomOutQAction

        function onTriggered() {
            root.zoomByAtCenter(-root.zoomStepPercent);
        }
    }

    Connections {
        target: root.panLeftQAction

        function onTriggered() {
            root.panBy(-root.keyboardPanDistance, 0);
        }
    }

    Connections {
        target: root.panRightQAction

        function onTriggered() {
            root.panBy(root.keyboardPanDistance, 0);
        }
    }

    Connections {
        target: root.panUpQAction

        function onTriggered() {
            root.panBy(0, -root.keyboardPanDistance);
        }
    }

    Connections {
        target: root.panDownQAction

        function onTriggered() {
            root.panBy(0, root.keyboardPanDistance);
        }
    }

    Connections {
        target: root.panTopLeftQAction

        function onTriggered() {
            root.panToTopLeft();
        }
    }

    Connections {
        target: root.panBottomRightQAction

        function onTriggered() {
            root.panToBottomRight();
        }
    }

    Connections {
        target: root.scanForwardQAction

        function onTriggered() {
            root.scanForward();
        }
    }

    Connections {
        target: root.scanBackwardQAction

        function onTriggered() {
            root.scanBackward();
        }
    }
}
