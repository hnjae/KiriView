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

    readonly property var previousImageQAction: root.application.action("go_previous_image")
    readonly property var nextImageQAction: root.application.action("go_next_image")
    readonly property var zoomInQAction: root.application.action("view_zoom_in")
    readonly property var zoomOutQAction: root.application.action("view_zoom_out")
    readonly property var panLeftQAction: root.application.action("view_pan_left")
    readonly property var panRightQAction: root.application.action("view_pan_right")
    readonly property var panUpQAction: root.application.action("view_pan_up")
    readonly property var panDownQAction: root.application.action("view_pan_down")
    readonly property var panTopLeftQAction: root.application.action("view_pan_top_left")
    readonly property var panBottomRightQAction: root.application.action("view_pan_bottom_right")
    readonly property var scanForwardQAction: root.application.action("view_scan_forward")
    readonly property var scanBackwardQAction: root.application.action("view_scan_backward")

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

    ConfiguredActionShortcut {
        actionName: "file_open"
        application: root.application
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "file_quit"
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.commandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "file_quit"
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_zoom_in"
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_zoom_out"
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_fit"
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_fit_height"
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_fit_width"
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_actual_size"
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_pan_left"
        application: root.application
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_pan_right"
        application: root.application
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_pan_up"
        application: root.application
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_pan_down"
        application: root.application
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_pan_top_left"
        application: root.application
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_pan_bottom_right"
        application: root.application
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_scan_forward"
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "view_scan_backward"
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "go_previous_image"
        application: root.application
        shortcutsEnabled: root.readyShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "go_next_image"
        application: root.application
        shortcutsEnabled: root.readyShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "go_first_image"
        application: root.application
        shortcutsEnabled: root.pageCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "go_last_image"
        application: root.application
        shortcutsEnabled: root.pageCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "go_previous_archive"
        application: root.application
        shortcutsEnabled: root.containerCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "go_next_archive"
        application: root.application
        shortcutsEnabled: root.containerCommandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "window_fullscreen"
        application: root.application
        shortcutsEnabled: root.commandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "help_shortcuts"
        application: root.application
        shortcutsEnabled: root.commandShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "options_configure"
        application: root.application
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "options_configure_keybinding"
        application: root.application
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ConfiguredActionShortcut {
        actionName: "options_show_menubar"
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
