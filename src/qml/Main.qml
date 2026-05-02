// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Dialogs as Dialogs
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    title: page.imageDocument.windowTitleFileName.length > 0 ? page.imageDocument.windowTitleFileName + " — KiriView" : "KiriView"
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

        readonly property var imageDocument: imageViewport.imageDocument
        readonly property bool imageReady: imageDocument.status === KiriImageDocument.Ready

        background: Rectangle {
            color: "#3c3c3c"
        }

        padding: 0
        globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None

        ImageActions {
            id: imageActions

            helpDialogOpen: root.helpDialogOpen
            imageDocument: page.imageDocument
            imageReady: page.imageReady

            onOpenDialogRequested: fileDialog.open()
        }

        header: ImageToolBar {
            id: imageToolBar

            actions: imageActions
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            maximumManualZoomPercent: page.imageDocument.maximumManualZoomPercent
            minimumManualZoomPercent: page.imageDocument.minimumManualZoomPercent
            zoomStepPercent: page.imageDocument.zoomStepPercent
        }

        ImageViewport {
            id: imageViewport

            anchors.fill: parent
            initialSourceUrl: root.initialSourceUrl
        }

        ImageShortcuts {
            helpDialogOpen: root.helpDialogOpen
            imageDocument: page.imageDocument
            imageToolBar: imageToolBar
            imageViewport: imageViewport

            onShortcutHelpRequested: shortcutHelpDialog.open()
            onToggleFullScreenRequested: root.toggleFullScreen()
        }

        ImageStateOverlay {
            anchors.fill: parent
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            openAction: imageActions.openAction
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
        nameFilters: page.imageDocument.openDialogNameFilters
        title: "Open Image or Comic Book"

        onAccepted: page.imageDocument.sourceUrl = selectedFile
    }
}
