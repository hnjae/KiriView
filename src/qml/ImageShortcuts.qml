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

    ShortcutDefinitions {
        id: shortcutDefinitions
    }

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

    ImageShortcutSet {
        enabled: root.readyShortcutsEnabled
        sequences: shortcutDefinitions.zoomInControlSequences

        onActivated: root.zoomByAtCenter(root.zoomStepPercent)
    }

    ImageShortcut {
        enabled: root.readyShortcutsEnabled
        sequence: shortcutDefinitions.zoomOutControlSequence

        onActivated: root.zoomByAtCenter(-root.zoomStepPercent)
    }

    ImageShortcutSet {
        enabled: root.readyCommandShortcutsEnabled
        sequences: shortcutDefinitions.zoomInCommandSequences

        onActivated: root.zoomByAtCenter(root.zoomStepPercent)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: shortcutDefinitions.zoomOutCommandSequence

        onActivated: root.zoomByAtCenter(-root.zoomStepPercent)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: shortcutDefinitions.fitImageSequence

        onActivated: root.imageDocument.resetZoom()
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: shortcutDefinitions.fitHeightSequence

        onActivated: root.imageDocument.setFitMode(KiriImageDocument.FitHeight)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: shortcutDefinitions.fitWidthSequence

        onActivated: root.imageDocument.setFitMode(KiriImageDocument.FitWidth)
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: shortcutDefinitions.actualSizeSequence

        onActivated: root.imageDocument.zoomPercent = 100
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: shortcutDefinitions.panLeftSequence

        onActivated: root.panBy(-root.keyboardPanDistance, 0)
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: shortcutDefinitions.panRightSequence

        onActivated: root.panBy(root.keyboardPanDistance, 0)
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: shortcutDefinitions.panUpSequence

        onActivated: root.panBy(0, -root.keyboardPanDistance)
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: shortcutDefinitions.panDownSequence

        onActivated: root.panBy(0, root.keyboardPanDistance)
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: shortcutDefinitions.panTopLeftSequence

        onActivated: root.panToTopLeft()
    }

    ImageShortcut {
        enabled: root.pannableCommandShortcutsEnabled
        sequence: shortcutDefinitions.panBottomRightSequence

        onActivated: root.panToBottomRight()
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: shortcutDefinitions.scanForwardSequence

        onActivated: root.scanForward()
    }

    ImageShortcut {
        enabled: root.readyCommandShortcutsEnabled
        sequence: shortcutDefinitions.scanBackwardSequence

        onActivated: root.scanBackward()
    }

    ImageShortcut {
        enabled: root.readyShortcutsEnabled
        sequence: shortcutDefinitions.previousPageSequence

        onActivated: root.openPreviousImage()
    }

    ImageShortcut {
        enabled: root.readyShortcutsEnabled
        sequence: shortcutDefinitions.nextPageSequence

        onActivated: root.openNextImage()
    }

    ImageShortcutSet {
        enabled: root.pageCommandShortcutsEnabled
        sequences: shortcutDefinitions.firstImageSequences

        onActivated: root.openFirstImage()
    }

    ImageShortcutSet {
        enabled: root.pageCommandShortcutsEnabled
        sequences: shortcutDefinitions.lastImageSequences

        onActivated: root.openLastImage()
    }

    ImageShortcut {
        enabled: root.containerCommandShortcutsEnabled
        sequence: shortcutDefinitions.previousContainerSequence

        onActivated: root.imageDocument.openPreviousContainer()
    }

    ImageShortcut {
        enabled: root.containerCommandShortcutsEnabled
        sequence: shortcutDefinitions.nextContainerSequence

        onActivated: root.imageDocument.openNextContainer()
    }

    ImageShortcut {
        enabled: root.commandShortcutsEnabled
        sequence: shortcutDefinitions.fullscreenCommandSequence

        onActivated: root.toggleFullScreenRequested()
    }

    ImageShortcut {
        enabled: root.helpShortcutsEnabled
        sequence: shortcutDefinitions.fullscreenFunctionSequence

        onActivated: root.toggleFullScreenRequested()
    }

    ImageShortcut {
        enabled: root.commandShortcutsEnabled
        sequence: shortcutDefinitions.helpCommandSequence

        onActivated: root.shortcutHelpRequested()
    }

    ImageShortcut {
        enabled: root.helpShortcutsEnabled
        sequence: shortcutDefinitions.helpFunctionSequence

        onActivated: root.shortcutHelpRequested()
    }
}
