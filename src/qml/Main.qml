// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs as Dialogs
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    title: page.imageView.windowTitleFileName.length > 0 ? page.imageView.windowTitleFileName + " — KiriView" : "KiriView"
    visible: true

    property bool helpDialogOpen: false
    property url initialSourceUrl
    property int visibilityBeforeFullscreen: Window.Windowed

    function restoredVisibility(visibility) {
        switch (visibility) {
        case Window.Windowed:
        case Window.Maximized:
        case Window.Minimized:
            return visibility;
        default:
            return Window.Windowed;
        }
    }

    function toggleFullScreen() {
        if (visibility === Window.FullScreen) {
            visibility = restoredVisibility(visibilityBeforeFullscreen);
            return;
        }

        visibilityBeforeFullscreen = restoredVisibility(visibility);
        visibility = Window.FullScreen;
    }

    minimumWidth: Kirigami.Units.gridUnit * 28
    minimumHeight: Kirigami.Units.gridUnit * 20
    width: minimumWidth
    height: minimumHeight

    Shortcut {
        context: Qt.WindowShortcut
        sequence: "Esc"

        onActivated: {
            if (root.helpDialogOpen) {
                shortcutHelpDialog.close();
                return;
            }

            root.close();
        }
    }

    pageStack.initialPage: Kirigami.Page {
        id: page

        readonly property var imageView: imageViewport.imageView
        readonly property bool imageReady: imageView.status === KiriImageView.Ready
        readonly property bool imagePannable: imageViewport.imagePannable
        readonly property int keyboardPanDistance: 64
        readonly property int maximumManualZoomPercent: imageView.maximumManualZoomPercent
        readonly property int minimumManualZoomPercent: imageView.minimumManualZoomPercent
        readonly property int zoomStepPercent: imageView.zoomStepPercent

        function panBy(deltaX, deltaY) {
            return imageViewport.panBy(deltaX, deltaY);
        }

        function textInputFocused() {
            return imageToolBar.textInputFocused();
        }

        function zoomBy(deltaPercent, viewportX, viewportY) {
            return imageViewport.zoomBy(deltaPercent, viewportX, viewportY);
        }

        background: Rectangle {
            color: "#3c3c3c"
        }

        padding: 0
        globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None

        Controls.Action {
            id: openAction

            enabled: !root.helpDialogOpen
            icon.name: "document-open-symbolic"
            shortcut: StandardKey.Open
            text: "Open"

            onTriggered: fileDialog.open()
        }

        Controls.Action {
            id: previousImageAction

            enabled: page.imageReady && !root.helpDialogOpen
            icon.name: "go-up-symbolic"
            text: "Previous"

            onTriggered: page.imageView.openPreviousImage()
        }

        Controls.Action {
            id: nextImageAction

            enabled: page.imageReady && !root.helpDialogOpen
            icon.name: "go-down-symbolic"
            text: "Next"

            onTriggered: page.imageView.openNextImage()
        }

        Controls.Action {
            id: previousContainerAction

            enabled: page.imageView.containerNavigationAvailable && !root.helpDialogOpen
            icon.name: "go-previous-symbolic"
            text: "Previous Container"

            onTriggered: page.imageView.openPreviousContainer()
        }

        Controls.Action {
            id: nextContainerAction

            enabled: page.imageView.containerNavigationAvailable && !root.helpDialogOpen
            icon.name: "go-next-symbolic"
            text: "Next Container"

            onTriggered: page.imageView.openNextContainer()
        }

        Controls.Action {
            id: fitAction

            enabled: page.imageReady && !root.helpDialogOpen
            icon.name: "zoom-fit-best-symbolic"
            text: "Fit"

            onTriggered: page.imageView.resetZoom()
        }

        Controls.Action {
            id: fitHeightAction

            enabled: page.imageReady && !root.helpDialogOpen
            text: "Fit Height"

            onTriggered: page.imageView.setFitMode(KiriImageView.FitHeight)
        }

        Controls.Action {
            id: fitWidthAction

            enabled: page.imageReady && !root.helpDialogOpen
            text: "Fit Width"

            onTriggered: page.imageView.setFitMode(KiriImageView.FitWidth)
        }

        header: ImageToolBar {
            id: imageToolBar

            fitAction: fitAction
            fitHeightAction: fitHeightAction
            fitWidthAction: fitWidthAction
            helpDialogOpen: root.helpDialogOpen
            imageReady: page.imageReady
            imageView: page.imageView
            maximumManualZoomPercent: page.maximumManualZoomPercent
            minimumManualZoomPercent: page.minimumManualZoomPercent
            nextContainerAction: nextContainerAction
            nextImageAction: nextImageAction
            openAction: openAction
            previousContainerAction: previousContainerAction
            previousImageAction: previousImageAction
            zoomStepPercent: page.zoomStepPercent
        }

        ImageViewport {
            id: imageViewport

            anchors.fill: parent
            initialSourceUrl: root.initialSourceUrl
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: "Ctrl+="

            onActivated: page.zoomBy(page.zoomStepPercent, imageViewport.viewportWidth / 2, imageViewport.viewportHeight / 2)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: "Ctrl++"

            onActivated: page.zoomBy(page.zoomStepPercent, imageViewport.viewportWidth / 2, imageViewport.viewportHeight / 2)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: "Ctrl+-"

            onActivated: page.zoomBy(-page.zoomStepPercent, imageViewport.viewportWidth / 2, imageViewport.viewportHeight / 2)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imagePannable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "Left"

            onActivated: page.panBy(-page.keyboardPanDistance, 0)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imagePannable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "Right"

            onActivated: page.panBy(page.keyboardPanDistance, 0)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imagePannable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "Up"

            onActivated: page.panBy(0, -page.keyboardPanDistance)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imagePannable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "Down"

            onActivated: page.panBy(0, page.keyboardPanDistance)
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: StandardKey.MoveToPreviousPage

            onActivated: page.imageView.openPreviousImage()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageReady && !root.helpDialogOpen
            sequence: StandardKey.MoveToNextPage

            onActivated: page.imageView.openNextImage()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageView.containerNavigationAvailable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "["

            onActivated: page.imageView.openPreviousContainer()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: page.imageView.containerNavigationAvailable && !page.textInputFocused() && !root.helpDialogOpen
            sequence: "]"

            onActivated: page.imageView.openNextContainer()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: !page.textInputFocused() && !root.helpDialogOpen
            sequence: "F"

            onActivated: root.toggleFullScreen()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: !root.helpDialogOpen
            sequence: "F11"

            onActivated: root.toggleFullScreen()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: !page.textInputFocused() && !root.helpDialogOpen
            sequence: "?"

            onActivated: shortcutHelpDialog.open()
        }

        Shortcut {
            context: Qt.WindowShortcut
            enabled: !root.helpDialogOpen
            sequence: "F1"

            onActivated: shortcutHelpDialog.open()
        }

        ImageStateOverlay {
            anchors.fill: parent
            imageReady: page.imageReady
            imageView: page.imageView
            openAction: openAction
        }
    }

    ShortcutHelpDialog {
        id: shortcutHelpDialog

        windowHeight: root.height
        windowWidth: root.width

        onClosed: root.helpDialogOpen = false
        onOpened: root.helpDialogOpen = true
    }

    Dialogs.FileDialog {
        id: fileDialog

        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: page.imageView.openDialogNameFilters
        title: "Open Image or Comic Book"

        onAccepted: page.imageView.sourceUrl = selectedFile
    }
}
