// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Dialogs as Dialogs
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.statefulapp as StatefulApp

StatefulApp.StatefulWindow {
    id: root

    application: KiriViewApplication {
        id: kiriApplication
    }
    title: page.imageDocument.windowTitleFileName.length > 0 ? KI18n.i18nc("@title:window", "%1 — KiriView", page.imageDocument.windowTitleFileName) : "KiriView"
    visible: true
    windowName: "Main"

    property bool helpDialogOpen: false
    property url initialSourceUrl
    property int visibilityBeforeFullscreen: Window.Windowed
    readonly property bool fullscreen: visibility === Window.FullScreen
    readonly property bool menuBarMode: kiriApplication.menuPresentation === KiriViewApplication.MenuBar
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

    function activeImageToolBar() {
        return root.fullscreen ? fullscreenImageToolBar : headerImageToolBar;
    }

    function focusImageViewport() {
        imageViewport.forceActiveFocus();
    }

    function toolbarTextInputFocused() {
        return activeImageToolBar().textInputFocused();
    }

    minimumWidth: Kirigami.Units.gridUnit * 14
    minimumHeight: Kirigami.Units.gridUnit * 12
    width: Kirigami.Units.gridUnit * 24
    height: Kirigami.Units.gridUnit * 20

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
        enabled: root.toolbarTextInputFocused() && !root.helpDialogOpen
        sequence: "Esc"

        onActivated: root.activeImageToolBar().cancelTextInputEditing(true)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.fullscreen && !root.helpDialogOpen && !root.toolbarTextInputFocused()
        sequence: "Esc"

        onActivated: root.toggleFullScreen()
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

    MenuAccessKeyRouter {
        enabled: true
        rootObject: root
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

            application: kiriApplication
            fullscreen: root.fullscreen
            helpDialogOpen: root.helpDialogOpen
            imageDocument: page.imageDocument
            imageReady: page.imageReady

            onImageBoundaryReached: function (message) {
                toastNotification.show(message, "image-boundary");
            }
            onOpenDialogRequested: fileDialog.open()
            onShortcutHelpRequested: shortcutHelpDialog.open()
            onToggleFullScreenRequested: root.toggleFullScreen()
        }

        header: ImageDocumentToolBar {
            id: headerImageToolBar

            actions: imageActions
            enabled: !root.fullscreen
            height: root.fullscreen ? 0 : implicitHeight
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            applicationMenuActions: applicationMenuDrawer.actions
            showApplicationMenuActions: !root.menuBarMode && !root.fullscreen
            visible: !root.fullscreen

            onTextInputFocusReturnRequested: root.focusImageViewport()
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

            onViewerClicked: {
                if (root.activeImageToolBar().commitTextInputEditing(true)) {
                    return;
                }

                root.focusImageViewport();
            }
        }

        Connections {
            target: page.imageDocument

            function onDisplayedUrlChanged() {
                toastNotification.dismissIfScope("image-boundary");
            }

            function onFileDeletionFailed(errorString) {
                toastNotification.show(errorString, "general");
            }
        }

        ImageShortcuts {
            application: kiriApplication
            helpDialogOpen: root.helpDialogOpen
            imageDocument: page.imageDocument
            imageToolBar: root.fullscreen ? fullscreenImageToolBar : headerImageToolBar
            imageViewport: imageViewport

            onImageBoundaryReached: function (message) {
                toastNotification.show(message, "image-boundary");
            }
        }

        ImageStateOverlay {
            anchors.fill: parent
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            openAction: imageActions.openAction
        }

        ToastNotification {
            id: toastNotification

            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            z: 999
        }

        ImageDocumentToolBar {
            id: fullscreenImageToolBar

            actions: imageActions
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            enabled: visible
            height: implicitHeight
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            applicationMenuActions: applicationMenuDrawer.actions
            showApplicationMenuActions: !root.menuBarMode && !root.fullscreen
            transientOverlay: true
            visible: root.fullscreen && root.fullscreenToolBarRevealed
            z: 20

            onTextInputFocusReturnRequested: root.focusImageViewport()

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

    globalDrawer: Kirigami.GlobalDrawer {
        id: applicationMenuDrawer

        actions: imageActions.applicationMenuActions
        enabled: !root.menuBarMode && !root.fullscreen
        isMenu: true
    }

    menuBar: ApplicationMenuBar {
        actions: imageActions
        fullscreen: root.fullscreen
        imageDocument: page.imageDocument
        visible: root.menuBarMode && !root.fullscreen
    }

    ShortcutHelpDialog {
        id: shortcutHelpDialog

        application: kiriApplication

        onClosed: root.helpDialogOpen = false
        onOpened: root.helpDialogOpen = true
    }

    Dialogs.FileDialog {
        id: fileDialog

        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: page.imageDocument.openDialogNameFilters
        title: KI18n.i18nc("@title:window", "Open Image or Comic Book")

        onAccepted: page.imageDocument.sourceUrl = selectedFile
    }
}
