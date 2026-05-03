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
    readonly property bool fullscreen: visibility === Window.FullScreen
    property bool fullscreenToolBarRevealed: false

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

    function canOpenDroppedUrls(dropEvent) {
        return dropEvent.hasUrls && dropEvent.urls.length > 0;
    }

    function openDroppedUrls(urls) {
        if (urls.length <= 0) {
            return;
        }

        page.imageDocument.sourceUrl = urls[0];
    }

    function revealFullscreenToolBar() {
        if (!fullscreen || helpDialogOpen) {
            return;
        }

        fullscreenToolBarRevealed = true;
        scheduleFullscreenToolBarHide();
    }

    function scheduleFullscreenToolBarHide() {
        if (!fullscreen || !fullscreenToolBarRevealed) {
            fullscreenToolBarHideTimer.stop();
            return;
        }

        if (fullscreenImageToolBar.interactionActive) {
            fullscreenToolBarHideTimer.stop();
            return;
        }

        fullscreenToolBarHideTimer.restart();
    }

    minimumWidth: Kirigami.Units.gridUnit * 28
    minimumHeight: Kirigami.Units.gridUnit * 20
    width: minimumWidth
    height: minimumHeight

    onFullscreenChanged: {
        if (fullscreen) {
            revealFullscreenToolBar();
            return;
        }

        fullscreenToolBarHideTimer.stop();
        fullscreenToolBarRevealed = false;
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: !root.helpDialogOpen
        sequence: "Esc"

        onActivated: root.close()
    }

    Timer {
        id: fullscreenToolBarHideTimer

        interval: 1000
        repeat: false

        onTriggered: {
            if (root.fullscreen && !fullscreenImageToolBar.interactionActive) {
                root.fullscreenToolBarRevealed = false;
            }
        }
    }

    DropArea {
        anchors.fill: parent
        enabled: !root.helpDialogOpen
        z: 100

        onDropped: drop => {
            if (!root.canOpenDroppedUrls(drop)) {
                drop.accepted = false;
                return;
            }

            root.openDroppedUrls(drop.urls);
            drop.acceptProposedAction();
        }
        onEntered: drag => {
            drag.accepted = root.canOpenDroppedUrls(drag);
        }
        onPositionChanged: drag => {
            drag.accepted = root.canOpenDroppedUrls(drag);
        }
    }

    pageStack.initialPage: Kirigami.Page {
        id: page

        readonly property var imageDocument: imageViewport.imageDocument
        readonly property bool imageReady: imageDocument.status === KiriImageDocument.Ready
        readonly property point fullscreenPointerPosition: fullscreenRevealHandler.point.position

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

        header: ImageDocumentToolBar {
            id: headerImageToolBar

            actions: imageActions
            enabled: !root.fullscreen
            height: root.fullscreen ? 0 : implicitHeight
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            visible: !root.fullscreen
        }

        onFullscreenPointerPositionChanged: {
            if (root.fullscreen && fullscreenRevealHandler.hovered) {
                root.revealFullscreenToolBar();
            }
        }

        HoverHandler {
            id: fullscreenRevealHandler

            enabled: root.fullscreen && !root.helpDialogOpen

            onHoveredChanged: {
                if (hovered) {
                    root.revealFullscreenToolBar();
                }
            }
        }

        ImageViewport {
            id: imageViewport

            anchors.fill: parent
            initialSourceUrl: root.initialSourceUrl
        }

        ImageShortcuts {
            helpDialogOpen: root.helpDialogOpen
            imageDocument: page.imageDocument
            imageToolBar: root.fullscreen ? fullscreenImageToolBar : headerImageToolBar
            imageViewport: imageViewport

            onImageBoundaryReached: function (message) {
                root.showPassiveNotification(message);
            }
            onShortcutHelpRequested: shortcutHelpDialog.open()
            onToggleFullScreenRequested: root.toggleFullScreen()
        }

        ImageStateOverlay {
            anchors.fill: parent
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            openAction: imageActions.openAction
        }

        ImageDocumentToolBar {
            id: fullscreenImageToolBar

            actions: imageActions
            anchors.left: parent.left
            anchors.leftMargin: Kirigami.Units.smallSpacing
            anchors.right: parent.right
            anchors.rightMargin: Kirigami.Units.smallSpacing
            anchors.top: parent.top
            anchors.topMargin: Kirigami.Units.smallSpacing
            enabled: visible
            floating: true
            height: implicitHeight
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            visible: root.fullscreen && root.fullscreenToolBarRevealed
            z: 20

            onInteractionActiveChanged: {
                if (!root.fullscreen) {
                    return;
                }

                if (interactionActive) {
                    root.revealFullscreenToolBar();
                    return;
                }

                root.scheduleFullscreenToolBarHide();
            }
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
