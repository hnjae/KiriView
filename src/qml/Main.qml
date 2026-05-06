// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs as Dialogs
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.settings as KirigamiSettings
import org.kde.kirigamiaddons.statefulapp as StatefulApp

StatefulApp.StatefulWindow {
    id: root

    application: KiriViewApplication {
        id: kiriApplication

        configurationView: settingsView
    }
    title: page.imageDocument.windowTitleFileName.length > 0 ? page.imageDocument.windowTitleFileName + " — KiriView" : "KiriView"
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

    minimumWidth: Kirigami.Units.gridUnit * 16
    minimumHeight: Kirigami.Units.gridUnit * 10
    width: Kirigami.Units.gridUnit * 24
    height: Kirigami.Units.gridUnit * 20

    ShortcutDefinitions {
        id: shortcutDefinitions
    }

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
        sequence: shortcutDefinitions.closeSequence

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

            application: kiriApplication
            fullscreen: root.fullscreen
            helpDialogOpen: root.helpDialogOpen
            imageDocument: page.imageDocument
            imageReady: page.imageReady

            onImageBoundaryReached: function (message) {
                root.showPassiveNotification(message);
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
            showApplicationMenuActions: !root.menuBarMode
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
            application: kiriApplication
            helpDialogOpen: root.helpDialogOpen
            imageDocument: page.imageDocument
            imageToolBar: root.fullscreen ? fullscreenImageToolBar : headerImageToolBar
            imageViewport: imageViewport

            onImageBoundaryReached: function (message) {
                root.showPassiveNotification(message);
            }
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
            anchors.right: parent.right
            anchors.top: parent.top
            enabled: visible
            height: implicitHeight
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            showApplicationMenuActions: !root.menuBarMode
            transientOverlay: true
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

    menuBar: Controls.MenuBar {
        visible: root.menuBarMode && !root.fullscreen

        Controls.Menu {
            title: "File"

            Controls.MenuItem {
                action: imageActions.openAction
            }

            Controls.MenuSeparator {}

            Controls.MenuItem {
                action: imageActions.quitAction
            }
        }

        Controls.Menu {
            title: "Go"

            Controls.MenuItem {
                action: imageActions.previousImageAction
            }

            Controls.MenuItem {
                action: imageActions.nextImageAction
            }

            Controls.MenuSeparator {}

            Controls.MenuItem {
                action: imageActions.firstImageAction
            }

            Controls.MenuItem {
                action: imageActions.lastImageAction
            }

            Controls.MenuSeparator {}

            Controls.MenuItem {
                action: imageActions.previousContainerAction
            }

            Controls.MenuItem {
                action: imageActions.nextContainerAction
            }
        }

        Controls.Menu {
            title: "View"

            Controls.MenuItem {
                action: imageActions.zoomInAction
            }

            Controls.MenuItem {
                action: imageActions.zoomOutAction
            }

            Controls.MenuSeparator {}

            Controls.Menu {
                title: "Fit"

                Controls.MenuItem {
                    action: imageActions.fitAction
                    checkable: true
                    checked: page.imageDocument.zoomMode === KiriImageDocument.Fit
                }

                Controls.MenuItem {
                    action: imageActions.fitHeightAction
                    checkable: true
                    checked: page.imageDocument.zoomMode === KiriImageDocument.FitHeight
                }

                Controls.MenuItem {
                    action: imageActions.fitWidthAction
                    checkable: true
                    checked: page.imageDocument.zoomMode === KiriImageDocument.FitWidth
                }
            }

            Controls.MenuItem {
                action: imageActions.actualSizeAction
            }

            Controls.MenuSeparator {}

            Controls.MenuItem {
                action: imageActions.fullscreenAction
                checkable: true
                checked: root.fullscreen
            }
        }

        Controls.Menu {
            title: "Settings"

            Controls.MenuItem {
                action: imageActions.showMenubarAction
            }

            Controls.MenuSeparator {}

            Controls.MenuItem {
                action: imageActions.configureAction
            }

            Controls.MenuItem {
                action: imageActions.configureShortcutsAction
            }
        }

        Controls.Menu {
            title: "Help"

            Controls.MenuItem {
                action: imageActions.shortcutHelpAction
            }
        }
    }

    KirigamiSettings.ConfigurationView {
        id: settingsView

        window: root

        modules: [
            KirigamiSettings.ConfigurationModule {
                moduleId: "interface"
                text: "Interface"
                icon.name: "preferences-desktop-symbolic"
                page: () => Qt.createComponent("io.github.hnjae.kiriview", "InterfaceSettingsPage")
                initialProperties: () => {
                    return {
                        "application": kiriApplication
                    };
                }
            },
            KirigamiSettings.ShortcutsConfigurationModule {
                application: root.application
            }
        ]
    }

    ShortcutHelpDialog {
        id: shortcutHelpDialog

        application: kiriApplication
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
