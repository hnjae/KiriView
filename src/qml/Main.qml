// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
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

        background: Rectangle {
            color: "#3c3c3c"
        }

        padding: 0
        globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None

        ImageActions {
            id: imageActions

            helpDialogOpen: root.helpDialogOpen
            imageReady: page.imageReady
            imageView: page.imageView

            onOpenDialogRequested: fileDialog.open()
        }

        header: ImageToolBar {
            id: imageToolBar

            actions: imageActions
            imageReady: page.imageReady
            imageView: page.imageView
            maximumManualZoomPercent: page.imageView.maximumManualZoomPercent
            minimumManualZoomPercent: page.imageView.minimumManualZoomPercent
            zoomStepPercent: page.imageView.zoomStepPercent
        }

        ImageViewport {
            id: imageViewport

            anchors.fill: parent
            initialSourceUrl: root.initialSourceUrl
        }

        ImageShortcuts {
            helpDialogOpen: root.helpDialogOpen
            imageToolBar: imageToolBar
            imageView: page.imageView
            imageViewport: imageViewport

            onShortcutHelpRequested: shortcutHelpDialog.open()
            onToggleFullScreenRequested: root.toggleFullScreen()
        }

        ImageStateOverlay {
            anchors.fill: parent
            imageReady: page.imageReady
            imageView: page.imageView
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
        nameFilters: page.imageView.openDialogNameFilters
        title: "Open Image or Comic Book"

        onAccepted: page.imageView.sourceUrl = selectedFile
    }
}
