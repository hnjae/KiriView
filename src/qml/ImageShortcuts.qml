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

    function textInputFocused() {
        return imageToolBar.textInputFocused();
    }

    function zoomBy(deltaPercent, viewportX, viewportY) {
        return imageViewport.zoomBy(deltaPercent, viewportX, viewportY);
    }

    function zoomByAtCenter(deltaPercent) {
        return root.zoomBy(deltaPercent, root.imageViewport.viewportWidth / 2, root.imageViewport.viewportHeight / 2);
    }

    ImageActionShortcut {
        action: root.openQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.zoomInQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.zoomOutQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.fitQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.fitHeightQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.fitWidthQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.actualSizeQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.panLeftQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.panRightQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.panUpQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.panDownQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.panTopLeftQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.panBottomRightQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.scanForwardQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.scanBackwardQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.previousImageQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.nextImageQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.readyShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.firstImageQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.pageCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.lastImageQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.pageCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.previousContainerQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.containerCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.nextContainerQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.containerCommandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.fullscreenQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.commandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.shortcutHelpQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.commandShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.configureQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.configureShortcutsQAction
        shortcutRevision: root.shortcutRevision
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ImageActionShortcut {
        action: root.showMenubarQAction
        shortcutRevision: root.shortcutRevision
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
