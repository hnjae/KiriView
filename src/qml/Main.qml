// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

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
    title: documentSession.windowTitleSubject.length > 0 ? KI18n.i18nc("@title:window", "%1 — KiriView", documentSession.windowTitleSubject) : "KiriView"
    visible: true
    windowName: "Main"

    property bool helpDialogOpen: false
    property url initialSourceUrl
    property int visibilityBeforeFullscreen: Window.Windowed
    readonly property bool fullscreen: visibility === Window.FullScreen
    readonly property bool menuBarMode: kiriApplication.menuPresentation === KiriViewApplication.MenuBar
    readonly property bool applicationMenuShortcutEnabled: !root.menuBarMode && !root.fullscreen && !root.helpDialogOpen
    readonly property var showMenubarAction: kiriApplication.actionForId(KiriViewApplication.OptionsShowMenubarAction)
    readonly property var openApplicationMenuAction: kiriApplication.actionForId(KiriViewApplication.OpenApplicationMenuAction)
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

        documentSession.sourceUrl = urls[0];
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

        if (mainImageToolBar.interactionActive) {
            fullscreenToolBarHideTimer.stop();
            return;
        }

        fullscreenToolBarHideTimer.restart();
    }

    function activeImageToolBar() {
        return mainImageToolBar;
    }

    function activeMenuHost() {
        return root.activeImageToolBar();
    }

    function focusActiveViewport() {
        mediaWorkspaceHost.forceActiveViewportFocus();
    }

    function openApplicationMenu() {
        return root.activeMenuHost().openApplicationMenu();
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

    Binding {
        property: "enabled"
        target: root.openApplicationMenuAction
        value: root.applicationMenuShortcutEnabled
        when: root.openApplicationMenuAction !== null && root.openApplicationMenuAction !== undefined
    }

    Binding {
        property: "enabled"
        target: root.showMenubarAction
        value: !root.helpDialogOpen
        when: root.showMenubarAction !== null && root.showMenubarAction !== undefined
    }

    ConfiguredActionShortcut {
        actionId: KiriViewApplication.OpenApplicationMenuAction
        application: kiriApplication
        shortcutFilter: ConfiguredActionShortcut.AllShortcuts
        shortcutsEnabled: root.applicationMenuShortcutEnabled
    }

    ConfiguredActionShortcut {
        actionId: KiriViewApplication.OptionsShowMenubarAction
        application: kiriApplication
        shortcutFilter: ConfiguredActionShortcut.AllShortcuts
        shortcutsEnabled: !root.helpDialogOpen
    }

    ActionTrigger {
        action: root.openApplicationMenuAction
        handler: () => root.openApplicationMenu()
    }

    Timer {
        id: fullscreenToolBarHideTimer

        interval: 1000
        repeat: false

        onTriggered: {
            if (root.fullscreen && !mainImageToolBar.interactionActive) {
                root.fullscreenToolBarRevealed = false;
            }
        }
    }

    KiriDocumentSession {
        id: documentSession

        objectName: "documentSession"

        Component.onCompleted: {
            if (root.initialSourceUrl.toString().length > 0) {
                sourceUrl = root.initialSourceUrl;
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

        readonly property var imageDocument: documentSession.imageDocument
        readonly property var videoDocument: documentSession.videoDocument
        readonly property bool imageMode: documentSession.documentKind === KiriDocumentSession.Image
        readonly property bool videoMode: documentSession.documentKind === KiriDocumentSession.Video
        readonly property bool imageReady: imageMode && imageDocument.status === KiriImageDocument.Ready && !imageDocument.unsupportedOpenedCollectionVideo
        readonly property point fullscreenPointerPosition: fullscreenRevealHandler.point.position
        property bool documentDeletionWasInProgress: false

        background: Rectangle {
            color: imageViewTheme.darkBackgroundColor
        }

        padding: 0
        globalToolBarStyle: Kirigami.ApplicationHeaderStyle.None

        Item {
            id: imageViewTheme

            Kirigami.Theme.colorSet: Kirigami.Theme.View
            Kirigami.Theme.inherit: false

            readonly property color viewBackgroundColor: Kirigami.Theme.backgroundColor
            readonly property color viewTextColor: Kirigami.Theme.textColor
            readonly property bool lightColorScheme: viewBackgroundColor.hslLightness > viewTextColor.hslLightness
            readonly property color darkBackgroundColor: lightColorScheme ? viewTextColor : viewBackgroundColor
        }

        MediaWorkspaceHost {
            id: mediaWorkspaceHost

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: root.fullscreen ? parent.top : mainImageToolBar.bottom

            documentSession: documentSession
            openAction: imageActions.openAction

            onViewerClicked: {
                if (root.activeImageToolBar().commitTextInputEditing(true)) {
                    return;
                }

                root.focusActiveViewport();
            }
            onViewerContextMenuRequested: function (popupParent, position) {
                root.activeImageToolBar().commitTextInputEditing(true);
                root.focusActiveViewport();
                viewerContextMenu.popup(popupParent, position.x, position.y);
            }
        }

        ImageActionAvailability {
            id: actionAvailability

            containerNavigationAvailable: page.imageMode && page.imageDocument.containerNavigationAvailable
            fileDeletionInProgress: documentSession.fileDeletionInProgress
            helpDialogOpen: root.helpDialogOpen
            imagePannable: page.imageMode && mediaWorkspaceHost.imageInteractionSurface.imagePannable
            imageReady: page.imageReady
            rightToLeftReadingAvailable: page.imageMode && page.imageDocument.rightToLeftReadingAvailable
            rightToLeftReadingEnabled: page.imageMode && page.imageDocument.rightToLeftReadingEnabled
            twoPageModeAvailable: page.imageMode && page.imageDocument.twoPageModeAvailable
            twoPageModeEnabled: page.imageMode && page.imageDocument.twoPageModeEnabled
        }

        ImageActions {
            id: imageActions

            application: kiriApplication
            actionAvailability: actionAvailability
            documentSession: documentSession
            fullscreen: root.fullscreen
            imageDocument: page.imageDocument
            infoPanelVisible: mediaWorkspaceHost.infoPanelVisible
            thumbnailPanelVisible: mediaWorkspaceHost.thumbnailPanelVisible

            onImageBoundaryReached: function (message) {
                toastNotification.show(message, "image-boundary");
            }
            onOpenDialogRequested: fileDialog.open()
            onShortcutHelpRequested: shortcutHelpDialog.open()
            onToggleFullScreenRequested: root.toggleFullScreen()
            onToggleInfoPanelRequested: mediaWorkspaceHost.toggleInfoPanel()
            onToggleThumbnailPanelRequested: mediaWorkspaceHost.toggleThumbnailPanel()
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

        Connections {
            target: page.imageDocument

            function onDisplayedUrlChanged() {
                toastNotification.dismissIfScope("image-boundary");
            }

            function onFileDeletionFailed(errorString) {
                toastNotification.show(errorString, "general");
            }

            function onUnsupportedOpenedCollectionVideoEntered(message) {
                toastNotification.show(message, "unsupported-document-video");
            }
        }

        Connections {
            target: documentSession

            function onErrorStringChanged() {
                if (page.documentDeletionWasInProgress && documentSession.errorString.length > 0) {
                    toastNotification.show(documentSession.errorString, "general");
                    page.documentDeletionWasInProgress = false;
                }
            }

            function onFileDeletionInProgressChanged() {
                if (documentSession.fileDeletionInProgress) {
                    page.documentDeletionWasInProgress = true;
                    return;
                }

                Qt.callLater(function () {
                    if (!documentSession.fileDeletionInProgress && documentSession.errorString.length <= 0) {
                        page.documentDeletionWasInProgress = false;
                    }
                });
            }

            function onOpenWithFailed(errorString) {
                toastNotification.show(errorString.length > 0 ? errorString : KI18n.i18nc("@info:status", "Could not open media with another application"), "general");
            }

            function onSourceUrlChanged() {
                toastNotification.dismissIfScope("image-boundary");
            }
        }

        Loader {
            active: page.imageMode || page.videoMode
            sourceComponent: ImageShortcuts {
                application: kiriApplication
                actionAvailability: actionAvailability
                documentSession: documentSession
                imageDocument: page.imageDocument
                imageInteractionSurface: mediaWorkspaceHost.imageInteractionSurface
                videoFileDeletionInProgress: documentSession.fileDeletionInProgress
                videoMode: page.videoMode

                onImageBoundaryReached: function (message) {
                    toastNotification.show(message, "image-boundary");
                }

                onUnsupportedVideoActionRequested: {
                    toastNotification.show(KI18n.i18nc("@info:status", "This action is not available for videos"), "unsupported-video-action");
                }
            }
        }

        ToastNotification {
            id: toastNotification

            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            z: 999
        }

        ContextActionMenu {
            id: viewerContextMenu

            objectName: "viewerContextMenu"
            actions: imageActions.contextMenuActions
        }

        ImageDocumentToolBar {
            id: mainImageToolBar

            objectName: "mainImageToolBar"

            actions: imageActions
            activeNavigationAvailable: documentSession.activeNavigationAvailable
            activeNavigationCount: documentSession.activeNavigationCount
            activeNavigationCurrentNumber: documentSession.activeNavigationCurrentNumber
            activeNavigationEditable: documentSession.activeNavigationEditable
            activeNavigationKnown: documentSession.activeNavigationKnown
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            enabled: visible
            height: implicitHeight
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            applicationMenuActions: imageActions.applicationMenuActions
            openActiveNavigationAtNumber: function (number) {
                documentSession.openActiveNavigationAtNumber(number);
            }
            rightToLeftReadingActive: imageActions.rightToLeftReadingActive
            rightToLeftReadingControlVisible: page.imageMode && page.imageDocument.openedCollectionScopeActive
            showApplicationMenuActions: !root.menuBarMode && !root.fullscreen
            transientOverlay: root.fullscreen
            twoPageModeControlVisible: page.imageMode && page.imageDocument.openedCollectionScopeActive
            videoMode: page.videoMode
            visible: !root.fullscreen || root.fullscreenToolBarRevealed
            zoomEditable: documentSession.activeZoomEditable
            zoomPercent: documentSession.activeZoomPercent
            zoomPercentAvailable: documentSession.activeZoomPercentAvailable
            zoomPercentKnown: documentSession.activeZoomPercentKnown
            z: 20

            onTextInputFocusReturnRequested: root.focusActiveViewport()

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

        Binding {
            target: actionAvailability
            property: "textInputFocused"
            value: root.toolbarTextInputFocused()
        }
    }

    menuBar: ApplicationMenuBar {
        actions: imageActions
        fullscreen: root.fullscreen
        imageDocument: page.imageDocument
        imageMode: page.imageMode
        mediaMode: page.imageMode || page.videoMode
        rightToLeftReadingActive: imageActions.rightToLeftReadingActive
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
        nameFilters: documentSession.openDialogNameFilters
        title: KI18n.i18nc("@title:window", "Open Image, Video, or Comic Book")

        onAccepted: documentSession.sourceUrl = selectedFile
    }
}
