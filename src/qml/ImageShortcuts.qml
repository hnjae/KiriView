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
    readonly property int keyboardPanDistance: 64
    readonly property int zoomStepPercent: imageDocument.zoomStepPercent

    signal shortcutHelpRequested
    signal toggleFullScreenRequested

    function panBy(deltaX, deltaY) {
        return imageViewport.panBy(deltaX, deltaY);
    }

    function textInputFocused() {
        return imageToolBar.textInputFocused();
    }

    function zoomBy(deltaPercent, viewportX, viewportY) {
        return imageViewport.zoomBy(deltaPercent, viewportX, viewportY);
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageReady && !root.helpDialogOpen
        sequence: "Ctrl+="

        onActivated: root.zoomBy(root.zoomStepPercent, root.imageViewport.viewportWidth / 2, root.imageViewport.viewportHeight / 2)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageReady && !root.helpDialogOpen
        sequence: "Ctrl++"

        onActivated: root.zoomBy(root.zoomStepPercent, root.imageViewport.viewportWidth / 2, root.imageViewport.viewportHeight / 2)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageReady && !root.helpDialogOpen
        sequence: "Ctrl+-"

        onActivated: root.zoomBy(-root.zoomStepPercent, root.imageViewport.viewportWidth / 2, root.imageViewport.viewportHeight / 2)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageReady && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "1"

        onActivated: root.imageDocument.resetZoom()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageReady && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "2"

        onActivated: root.imageDocument.setFitMode(KiriImageDocument.FitHeight)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageReady && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "3"

        onActivated: root.imageDocument.setFitMode(KiriImageDocument.FitWidth)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageReady && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "0"

        onActivated: root.imageDocument.zoomPercent = 100
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imagePannable && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "Left"

        onActivated: root.panBy(-root.keyboardPanDistance, 0)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imagePannable && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "Right"

        onActivated: root.panBy(root.keyboardPanDistance, 0)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imagePannable && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "Up"

        onActivated: root.panBy(0, -root.keyboardPanDistance)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imagePannable && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "Down"

        onActivated: root.panBy(0, root.keyboardPanDistance)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageReady && !root.helpDialogOpen
        sequence: StandardKey.MoveToPreviousPage

        onActivated: root.imageDocument.openPreviousImage()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageReady && !root.helpDialogOpen
        sequence: StandardKey.MoveToNextPage

        onActivated: root.imageDocument.openNextImage()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageDocument.containerNavigationAvailable && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "["

        onActivated: root.imageDocument.openPreviousContainer()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.imageDocument.containerNavigationAvailable && !root.textInputFocused() && !root.helpDialogOpen
        sequence: "]"

        onActivated: root.imageDocument.openNextContainer()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: !root.textInputFocused() && !root.helpDialogOpen
        sequence: "F"

        onActivated: root.toggleFullScreenRequested()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: !root.helpDialogOpen
        sequence: "F11"

        onActivated: root.toggleFullScreenRequested()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: !root.textInputFocused() && !root.helpDialogOpen
        sequence: "?"

        onActivated: root.shortcutHelpRequested()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: !root.helpDialogOpen
        sequence: "F1"

        onActivated: root.shortcutHelpRequested()
    }
}
