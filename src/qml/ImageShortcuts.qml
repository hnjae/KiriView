// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import io.github.hnjae.kiriview

Item {
    id: root

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

    signal imageBoundaryReached(string message)
    signal shortcutHelpRequested
    signal toggleFullScreenRequested

    function openFirstImage() {
        if (root.imageDocument.imageCount > 0) {
            root.imageDocument.openImageAtPage(1);
        }
    }

    function openLastImage() {
        if (root.imageDocument.imageCount > 0) {
            root.imageDocument.openImageAtPage(root.imageDocument.imageCount);
        }
    }

    function openNextImage() {
        if (root.atLastImage) {
            root.imageBoundaryReached("Last image");
            return;
        }

        root.imageDocument.openNextImage();
    }

    function openPreviousImage() {
        if (root.atFirstImage) {
            root.imageBoundaryReached("First image");
            return;
        }

        root.imageDocument.openPreviousImage();
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

    function scanForward() {
        if (!root.imagePannable) {
            root.openNextImage();
            return;
        }

        if (root.imageViewport.scanForward()) {
            return;
        }

        root.openNextImage();
    }

    function scanBackward() {
        if (!root.imagePannable) {
            root.openPreviousImage();
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

        root.imageDocument.openPreviousImage();
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

    ImageShortcut {
        enabled: root.readyShortcutsEnabled
        sequence: "Ctrl+="

        onActivated: root.zoomByAtCenter(root.zoomStepPercent)
    }

    ImageShortcut {
        enabled: root.readyShortcutsEnabled
        sequence: "Ctrl++"

        onActivated: root.zoomByAtCenter(root.zoomStepPercent)
    }

    ImageShortcut {
        enabled: root.readyShortcutsEnabled
        sequence: "Ctrl+-"

        onActivated: root.zoomByAtCenter(-root.zoomStepPercent)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: "="

        onActivated: root.zoomByAtCenter(root.zoomStepPercent)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: "+"

        onActivated: root.zoomByAtCenter(root.zoomStepPercent)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: "-"

        onActivated: root.zoomByAtCenter(-root.zoomStepPercent)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: "1"

        onActivated: root.imageDocument.resetZoom()
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: "2"

        onActivated: root.imageDocument.setFitMode(KiriImageDocument.FitHeight)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: "3"

        onActivated: root.imageDocument.setFitMode(KiriImageDocument.FitWidth)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: "0"

        onActivated: root.imageDocument.zoomPercent = 100
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: "Left"

        onActivated: root.panBy(-root.keyboardPanDistance, 0)
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: "Right"

        onActivated: root.panBy(root.keyboardPanDistance, 0)
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: "Up"

        onActivated: root.panBy(0, -root.keyboardPanDistance)
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: "Down"

        onActivated: root.panBy(0, root.keyboardPanDistance)
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: "<"

        onActivated: root.panToTopLeft()
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: ">"

        onActivated: root.panToBottomRight()
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: "."

        onActivated: root.scanForward()
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: ","

        onActivated: root.scanBackward()
    }

    ImageShortcut {
        enabled: root.readyShortcutsEnabled
        sequence: StandardKey.MoveToPreviousPage

        onActivated: root.openPreviousImage()
    }

    ImageShortcut {
        enabled: root.readyShortcutsEnabled
        sequence: StandardKey.MoveToNextPage

        onActivated: root.openNextImage()
    }

    ImageShortcut {
        enabled: root.pageCommandShortcutsEnabled
        sequence: "Ctrl+Home"

        onActivated: root.openFirstImage()
    }

    ImageShortcut {
        enabled: root.pageCommandShortcutsEnabled
        sequence: "Home"

        onActivated: root.openFirstImage()
    }

    ImageShortcut {
        enabled: root.pageCommandShortcutsEnabled
        sequence: "Ctrl+End"

        onActivated: root.openLastImage()
    }

    ImageShortcut {
        enabled: root.pageCommandShortcutsEnabled
        sequence: "End"

        onActivated: root.openLastImage()
    }

    ImageShortcut {
        enabled: root.containerCommandShortcutsEnabled
        sequence: "["

        onActivated: root.imageDocument.openPreviousContainer()
    }

    ImageShortcut {
        enabled: root.containerCommandShortcutsEnabled
        sequence: "]"

        onActivated: root.imageDocument.openNextContainer()
    }

    ImageShortcut {
        enabled: root.commandShortcutsEnabled
        sequence: "F"

        onActivated: root.toggleFullScreenRequested()
    }

    ImageShortcut {
        enabled: root.helpShortcutsEnabled
        sequence: "F11"

        onActivated: root.toggleFullScreenRequested()
    }

    ImageShortcut {
        enabled: root.commandShortcutsEnabled
        sequence: "?"

        onActivated: root.shortcutHelpRequested()
    }

    ImageShortcut {
        enabled: root.helpShortcutsEnabled
        sequence: "F1"

        onActivated: root.shortcutHelpRequested()
    }
}
