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
    readonly property int shortcutRevision: root.application.shortcutRevision

    readonly property var openQAction: root.application.action("file_open")
    readonly property var previousImageQAction: root.application.action("go_previous_image")
    readonly property var nextImageQAction: root.application.action("go_next_image")
    readonly property var firstImageQAction: root.application.action("go_first_image")
    readonly property var lastImageQAction: root.application.action("go_last_image")
    readonly property var previousContainerQAction: root.application.action("go_previous_archive")
    readonly property var nextContainerQAction: root.application.action("go_next_archive")
    readonly property var zoomInQAction: root.application.action("view_zoom_in")
    readonly property var zoomOutQAction: root.application.action("view_zoom_out")
    readonly property var fitQAction: root.application.action("view_fit")
    readonly property var fitHeightQAction: root.application.action("view_fit_height")
    readonly property var fitWidthQAction: root.application.action("view_fit_width")
    readonly property var actualSizeQAction: root.application.action("view_actual_size")
    readonly property var panLeftQAction: root.application.action("view_pan_left")
    readonly property var panRightQAction: root.application.action("view_pan_right")
    readonly property var panUpQAction: root.application.action("view_pan_up")
    readonly property var panDownQAction: root.application.action("view_pan_down")
    readonly property var panTopLeftQAction: root.application.action("view_pan_top_left")
    readonly property var panBottomRightQAction: root.application.action("view_pan_bottom_right")
    readonly property var scanForwardQAction: root.application.action("view_scan_forward")
    readonly property var scanBackwardQAction: root.application.action("view_scan_backward")
    readonly property var fullscreenQAction: root.application.action("window_fullscreen")
    readonly property var shortcutHelpQAction: root.application.action("help_shortcuts")
    readonly property var configureQAction: root.application.action("options_configure")
    readonly property var configureShortcutsQAction: root.application.action("options_configure_keybinding")
    readonly property var showMenubarQAction: root.application.action("options_show_menubar")

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

    function actionShortcuts(actionName) {
        root.shortcutRevision;
        return root.application.shortcuts(actionName);
    }

    function actionShortcutsEnabled(shortcutsEnabled, action) {
        return shortcutsEnabled && action !== null && action !== undefined && action.enabled;
    }

    function textInputFocused() {
        return imageToolBar.textInputFocused();
    }

    function triggerAction(action) {
        if (action && action.enabled) {
            action.trigger();
        }
    }

    function zoomBy(deltaPercent, viewportX, viewportY) {
        return imageViewport.zoomBy(deltaPercent, viewportX, viewportY);
    }

    function zoomByAtCenter(deltaPercent) {
        return root.zoomBy(deltaPercent, root.imageViewport.viewportWidth / 2, root.imageViewport.viewportHeight / 2);
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("file_open")
        shortcutsEnabled: root.actionShortcutsEnabled(root.helpShortcutsEnabled, root.openQAction)

        onActivated: root.triggerAction(root.openQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_zoom_in")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyCommandShortcutsEnabled, root.zoomInQAction)

        onActivated: root.triggerAction(root.zoomInQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_zoom_out")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyCommandShortcutsEnabled, root.zoomOutQAction)

        onActivated: root.triggerAction(root.zoomOutQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_fit")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyCommandShortcutsEnabled, root.fitQAction)

        onActivated: root.triggerAction(root.fitQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_fit_height")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyCommandShortcutsEnabled, root.fitHeightQAction)

        onActivated: root.triggerAction(root.fitHeightQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_fit_width")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyCommandShortcutsEnabled, root.fitWidthQAction)

        onActivated: root.triggerAction(root.fitWidthQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_actual_size")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyCommandShortcutsEnabled, root.actualSizeQAction)

        onActivated: root.triggerAction(root.actualSizeQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_pan_left")
        shortcutsEnabled: root.actionShortcutsEnabled(root.pannableCommandShortcutsEnabled, root.panLeftQAction)

        onActivated: root.triggerAction(root.panLeftQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_pan_right")
        shortcutsEnabled: root.actionShortcutsEnabled(root.pannableCommandShortcutsEnabled, root.panRightQAction)

        onActivated: root.triggerAction(root.panRightQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_pan_up")
        shortcutsEnabled: root.actionShortcutsEnabled(root.pannableCommandShortcutsEnabled, root.panUpQAction)

        onActivated: root.triggerAction(root.panUpQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_pan_down")
        shortcutsEnabled: root.actionShortcutsEnabled(root.pannableCommandShortcutsEnabled, root.panDownQAction)

        onActivated: root.triggerAction(root.panDownQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_pan_top_left")
        shortcutsEnabled: root.actionShortcutsEnabled(root.pannableCommandShortcutsEnabled, root.panTopLeftQAction)

        onActivated: root.triggerAction(root.panTopLeftQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_pan_bottom_right")
        shortcutsEnabled: root.actionShortcutsEnabled(root.pannableCommandShortcutsEnabled, root.panBottomRightQAction)

        onActivated: root.triggerAction(root.panBottomRightQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_scan_forward")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyCommandShortcutsEnabled, root.scanForwardQAction)

        onActivated: root.triggerAction(root.scanForwardQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("view_scan_backward")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyCommandShortcutsEnabled, root.scanBackwardQAction)

        onActivated: root.triggerAction(root.scanBackwardQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("go_previous_image")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyShortcutsEnabled, root.previousImageQAction)

        onActivated: root.triggerAction(root.previousImageQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("go_next_image")
        shortcutsEnabled: root.actionShortcutsEnabled(root.readyShortcutsEnabled, root.nextImageQAction)

        onActivated: root.triggerAction(root.nextImageQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("go_first_image")
        shortcutsEnabled: root.actionShortcutsEnabled(root.pageCommandShortcutsEnabled, root.firstImageQAction)

        onActivated: root.triggerAction(root.firstImageQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("go_last_image")
        shortcutsEnabled: root.actionShortcutsEnabled(root.pageCommandShortcutsEnabled, root.lastImageQAction)

        onActivated: root.triggerAction(root.lastImageQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("go_previous_archive")
        shortcutsEnabled: root.actionShortcutsEnabled(root.containerCommandShortcutsEnabled, root.previousContainerQAction)

        onActivated: root.triggerAction(root.previousContainerQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("go_next_archive")
        shortcutsEnabled: root.actionShortcutsEnabled(root.containerCommandShortcutsEnabled, root.nextContainerQAction)

        onActivated: root.triggerAction(root.nextContainerQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("window_fullscreen")
        shortcutsEnabled: root.actionShortcutsEnabled(root.commandShortcutsEnabled, root.fullscreenQAction)

        onActivated: root.triggerAction(root.fullscreenQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("help_shortcuts")
        shortcutsEnabled: root.actionShortcutsEnabled(root.commandShortcutsEnabled, root.shortcutHelpQAction)

        onActivated: root.triggerAction(root.shortcutHelpQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("options_configure")
        shortcutsEnabled: root.actionShortcutsEnabled(root.helpShortcutsEnabled, root.configureQAction)

        onActivated: root.triggerAction(root.configureQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("options_configure_keybinding")
        shortcutsEnabled: root.actionShortcutsEnabled(root.helpShortcutsEnabled, root.configureShortcutsQAction)

        onActivated: root.triggerAction(root.configureShortcutsQAction)
    }

    ImageActionShortcut {
        sequences: root.actionShortcuts("options_show_menubar")
        shortcutsEnabled: root.actionShortcutsEnabled(root.helpShortcutsEnabled, root.showMenubarQAction)

        onActivated: root.triggerAction(root.showMenubarQAction)
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
