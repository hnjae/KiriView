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
    title: documentSession.windowTitleFileName.length > 0 ? KI18n.i18nc("@title:window", "%1 — KiriView", documentSession.windowTitleFileName) : "KiriView"
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
        if (page.videoMode) {
            videoViewportLoader.forceVideoViewportFocus();
            return;
        }

        imageViewport.forceActiveFocus();
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

    Shortcut {
        context: Qt.WindowShortcut
        enabled: !root.menuBarMode && !root.fullscreen && !root.helpDialogOpen
        sequence: "F10"

        onActivated: root.openApplicationMenu()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: !root.helpDialogOpen
        sequence: "Ctrl+M"

        onActivated: kiriApplication.actionForId(KiriViewApplication.OptionsShowMenubarAction).trigger()
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

        readonly property var imageDocument: imageViewport.imageDocument
        readonly property var videoDocument: documentSession.videoDocument
        readonly property bool imageMode: documentSession.documentKind === KiriDocumentSession.Image
        readonly property bool videoMode: documentSession.documentKind === KiriDocumentSession.Video
        readonly property bool imageReady: imageMode && imageDocument.status === KiriImageDocument.Ready
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

        ImageViewport {
            id: imageViewport

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: root.fullscreen ? parent.top : mainImageToolBar.bottom
            imageDocument: documentSession.imageDocument
            presentationActive: page.imageMode
            visible: !page.videoMode

            onViewerClicked: {
                if (root.activeImageToolBar().commitTextInputEditing(true)) {
                    return;
                }

                root.focusActiveViewport();
            }
        }

        Loader {
            id: videoViewportLoader

            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: root.fullscreen ? parent.top : mainImageToolBar.bottom
            active: page.videoMode

            function ensureVideoViewportLoaded() {
                if (!active) {
                    source = "";
                    return;
                }

                if (source.toString().length === 0) {
                    setSource(Qt.resolvedUrl("VideoViewport.qml"), {
                        "videoDocument": page.videoDocument
                    });
                }
            }

            function forceVideoViewportFocus() {
                // Dynamic VideoViewport access keeps QtMultimedia out of image startup.
                // qmllint disable missing-property
                if (item !== null) {
                    item["forceActiveFocus"]();
                }
                // qmllint enable missing-property
            }

            Component.onCompleted: ensureVideoViewportLoaded()
            onActiveChanged: ensureVideoViewportLoaded()
        }

        ImageActionAvailability {
            id: actionAvailability

            containerNavigationAvailable: page.imageMode && page.imageDocument.containerNavigationAvailable
            currentLastPageNumber: page.imageMode ? page.imageDocument.currentLastPageNumber : 0
            currentPageNumber: page.imageMode ? page.imageDocument.currentPageNumber : 0
            fileDeletionInProgress: documentSession.fileDeletionInProgress
            helpDialogOpen: root.helpDialogOpen
            imageCount: page.imageMode ? page.imageDocument.imageCount : 0
            imageHorizontallyPannable: page.imageMode && imageViewport.imageHorizontallyPannable
            imagePannable: page.imageMode && imageViewport.imagePannable
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

            onImageBoundaryReached: function (message) {
                toastNotification.show(message, "image-boundary");
            }
            onOpenDialogRequested: fileDialog.open()
            onShortcutHelpRequested: shortcutHelpDialog.open()
            onToggleFullScreenRequested: root.toggleFullScreen()
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

            function onSourceUrlChanged() {
                toastNotification.dismissIfScope("image-boundary");
            }
        }

        Loader {
            active: page.imageMode || page.videoMode
            sourceComponent: ImageShortcuts {
                application: kiriApplication
                actionAvailability: actionAvailability
                handleMenuPresentationShortcut: false
                imageDocument: page.imageDocument
                imageViewport: imageViewport
                videoFileDeletionInProgress: documentSession.fileDeletionInProgress
                videoMediaNavigationActive: documentSession.mediaNavigationActive
                videoMode: page.videoMode

                onImageBoundaryReached: function (message) {
                    toastNotification.show(message, "image-boundary");
                }

                onUnsupportedVideoActionRequested: {
                    toastNotification.show(KI18n.i18nc("@info:status", "This action is not available for videos"), "unsupported-video-action");
                }
            }
        }

        ImageStateOverlay {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: root.fullscreen ? parent.top : mainImageToolBar.bottom
            imageDocument: page.imageDocument
            imageReady: page.imageReady
            openAction: imageActions.openAction
            visible: !page.videoMode
        }

        ToastNotification {
            id: toastNotification

            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            z: 999
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
            showApplicationMenuActions: !root.menuBarMode && !root.fullscreen
            transientOverlay: root.fullscreen
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
