// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Dialogs as Dialogs
import org.hnjae.kiriview
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
    property bool fullscreenPointerHidden: false
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

    function handleFullscreenPointerMovement(position) {
        if (!fullscreen) {
            return;
        }

        fullscreenPointerHidden = false;
        fullscreenPointerIdleTimer.restart();

        if (position.y >= 0 && position.y <= fullscreenToolBarRevealArea.height) {
            revealFullscreenToolBar();
            return;
        }

        scheduleFullscreenToolBarHide();
    }

    function scheduleFullscreenToolBarHide() {
        if (!fullscreen || !fullscreenToolBarRevealed) {
            fullscreenToolBarHideTimer.stop();
            return;
        }

        if (fullscreenToolBarInteractionActive()) {
            fullscreenToolBarHideTimer.stop();
            return;
        }

        fullscreenToolBarHideTimer.restart();
    }

    function fullscreenToolBarInteractionActive() {
        return mainImageToolBar.interactionActive;
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

    function publishActionUiState() {
        kiriApplication.updateActionUiGateSnapshot(root.helpDialogOpen, root.toolbarTextInputFocused(), mediaWorkspaceHost.infoPanelVisible, mediaWorkspaceHost.thumbnailPanelVisible, root.fullscreen, root.applicationMenuShortcutEnabled, !root.helpDialogOpen);
    }

    minimumWidth: Kirigami.Units.gridUnit * 14
    minimumHeight: Kirigami.Units.gridUnit * 12
    width: Kirigami.Units.gridUnit * 24
    height: Kirigami.Units.gridUnit * 20

    onFullscreenChanged: {
        if (fullscreen) {
            fullscreenPointerHidden = true;
            fullscreenPointerIdleTimer.stop();
            revealFullscreenToolBar();
            return;
        }

        fullscreenPointerIdleTimer.stop();
        fullscreenPointerHidden = false;
        fullscreenToolBarHideTimer.stop();
        fullscreenToolBarRevealed = false;
    }

    Component.onCompleted: {
        kiriApplication.setDocumentSession(documentSession);
        kiriApplication.setShortcutHost(root);
        root.publishActionUiState();
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.toolbarTextInputFocused() && !root.helpDialogOpen
        sequence: "Esc"

        onActivated: root.activeImageToolBar().cancelTextInputEditing(true)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: mediaWorkspaceHost.infoPanelVisible && !root.helpDialogOpen && !root.toolbarTextInputFocused()
        sequence: "Esc"

        onActivated: mediaWorkspaceHost.closeInfoPanel()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.fullscreen && !root.helpDialogOpen && !root.toolbarTextInputFocused() && !mediaWorkspaceHost.infoPanelVisible
        sequence: "Esc"

        onActivated: root.toggleFullScreen()
    }

    Connections {
        target: kiriApplication

        function onImageBoundaryReached(message) {
            toastNotification.show(message, "image-boundary");
        }

        function onOpenApplicationMenuRequested() {
            root.openApplicationMenu();
        }

        function onOpenDialogRequested() {
            fileDialog.open();
        }

        function onShortcutHelpRequested() {
            shortcutHelpDialog.open();
        }

        function onToggleFullScreenRequested() {
            root.toggleFullScreen();
        }

        function onToggleInfoPanelRequested() {
            mediaWorkspaceHost.toggleInfoPanel();
        }

        function onToggleThumbnailPanelRequested() {
            mediaWorkspaceHost.toggleThumbnailPanel();
        }

        function onUnsupportedVideoActionTriggered(actionId) {
            toastNotification.show(KI18n.i18nc("@info:status", "This action is not available for videos"), "unsupported-video-action");
        }

        function onUnsupportedImageActionTriggered(actionId) {
            toastNotification.show(KI18n.i18nc("@info:status", "This action is not available for images"), "unsupported-image-action");
        }
    }

    Timer {
        id: fullscreenToolBarHideTimer

        interval: 1000
        repeat: false

        onTriggered: {
            if (root.fullscreen && !root.fullscreenToolBarInteractionActive()) {
                root.fullscreenToolBarRevealed = false;
            }
        }
    }

    Timer {
        id: fullscreenPointerIdleTimer

        interval: 1000
        repeat: false

        onTriggered: {
            if (!root.fullscreen) {
                return;
            }

            root.fullscreenPointerHidden = true;
            if (root.fullscreenToolBarRevealed && !root.fullscreenToolBarInteractionActive()) {
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
        readonly property bool imageReady: documentSession.activeImageReady
        readonly property string actionUiGateFingerprint: [root.helpDialogOpen, root.fullscreen, root.applicationMenuShortcutEnabled, root.toolbarTextInputFocused(), mediaWorkspaceHost.infoPanelVisible, mediaWorkspaceHost.thumbnailPanelVisible].join("|")
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
            readonly property color darkForegroundColor: lightColorScheme ? viewBackgroundColor : viewTextColor
        }

        MediaWorkspaceHost {
            id: mediaWorkspaceHost

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: root.fullscreen ? parent.top : mainImageToolBar.bottom

            documentSession: documentSession
            openAction: imageActions.openAction
            viewerForegroundColor: imageViewTheme.darkForegroundColor
            viewerSurfaceColor: imageViewTheme.darkBackgroundColor

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

        Item {
            id: fullscreenPointerTrackingArea

            anchors.fill: parent
            enabled: root.fullscreen
            visible: enabled
            z: 999

            HoverHandler {
                id: fullscreenPointerTrackingHoverHandler

                cursorShape: root.fullscreenPointerHidden ? Qt.BlankCursor : Qt.ArrowCursor
                enabled: fullscreenPointerTrackingArea.enabled

                onPointChanged: root.handleFullscreenPointerMovement(point.position)
            }
        }

        ImageActions {
            id: imageActions

            application: kiriApplication
            documentSession: documentSession
            imageDocument: page.imageDocument
            videoMode: page.videoMode
        }

        onActionUiGateFingerprintChanged: root.publishActionUiState()

        Connections {
            target: page.imageDocument

            function onDisplayedUrlChanged() {
                toastNotification.dismissIfScope("image-boundary");
                toastNotification.dismissIfScope("collection-boundary");
            }

            function onFileDeletionFailed(errorString) {
                toastNotification.show(errorString, "general");
            }

            function onContainerNavigationBoundaryReached(message) {
                toastNotification.show(message, "collection-boundary");
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

                if (documentSession.errorString.length <= 0) {
                    page.documentDeletionWasInProgress = false;
                }
            }

            function onOpenWithFailed(errorString) {
                toastNotification.show(errorString.length > 0 ? errorString : KI18n.i18nc("@info:status", "Could not open media with another application"), "general");
            }

            function onSourceUrlChanged() {
                toastNotification.dismissIfScope("image-boundary");
                toastNotification.dismissIfScope("collection-boundary");
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

        Item {
            id: fullscreenToolBarRevealArea

            readonly property bool hovered: fullscreenToolBarRevealHoverHandler.hovered

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            enabled: root.fullscreen && !root.helpDialogOpen
            height: mainImageToolBar.implicitHeight + Kirigami.Units.largeSpacing
            visible: enabled
            z: 19

            HoverHandler {
                id: fullscreenToolBarRevealHoverHandler

                enabled: fullscreenToolBarRevealArea.enabled

                onHoveredChanged: {
                    if (hovered) {
                        root.revealFullscreenToolBar();
                        return;
                    }

                    root.scheduleFullscreenToolBarHide();
                }
            }
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
            navigationPresentationProvider: kiriApplication
            applicationMenuActions: imageActions.applicationMenuActions
            openActiveNavigationAtNumber: function (number) {
                documentSession.openActiveNavigationAtNumber(number);
            }
            rightToLeftReadingControlVisible: documentSession.activeImageOpenedCollectionScopeActive
            showApplicationMenuActions: !root.menuBarMode && !root.fullscreen
            transientOverlay: root.fullscreen
            twoPageModeControlVisible: documentSession.activeImageOpenedCollectionScopeActive
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
    }

    menuBar: ApplicationMenuBar {
        actions: imageActions
        imageMode: page.imageMode
        mediaMode: page.imageMode || page.videoMode
        navigationPresentationProvider: kiriApplication
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
