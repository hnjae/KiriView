// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Item {
    id: root

    required property KiriDocumentSession documentSession
    required property var openAction
    readonly property KiriImageDocument imageDocument: documentSession.imageDocument
    readonly property bool imageMode: documentSession.documentKind === KiriDocumentSession.Image
    readonly property bool videoMode: documentSession.documentKind === KiriDocumentSession.Video
    readonly property bool imageReady: imageMode && imageDocument.status === KiriImageDocument.Ready
    readonly property url activeDelegateSource: imageMode ? Qt.resolvedUrl("ImageViewport.qml") : videoMode ? Qt.resolvedUrl("VideoViewport.qml") : ""
    readonly property var activeDelegate: mediaViewportDelegateLoader.item
    readonly property var activeInteractionSurface: activeDelegate !== null && activeDelegate.interactionSurface !== null ? activeDelegate.interactionSurface : inactiveInteractionSurface
    readonly property bool imageHorizontallyPannable: activeInteractionSurface.imageHorizontallyPannable
    readonly property bool imagePannable: activeInteractionSurface.imagePannable
    readonly property real imageViewportWidth: activeInteractionSurface.viewportWidth
    readonly property real imageViewportHeight: activeInteractionSurface.viewportHeight

    signal viewerClicked
    signal viewerContextMenuRequested(var popupParent, point position)

    function forceActiveViewportFocus() {
        if (root.activeDelegate !== null) {
            root.activeDelegate.requestViewportFocus();
        }
    }

    function panBy(deltaX, deltaY) {
        return root.activeInteractionSurface.panBy(deltaX, deltaY);
    }

    function panToBottomRight() {
        return root.activeInteractionSurface.panToBottomRight();
    }

    function panToTopLeft() {
        return root.activeInteractionSurface.panToTopLeft();
    }

    function scanForward() {
        return root.activeInteractionSurface.scanForward();
    }

    function scanBackward() {
        return root.activeInteractionSurface.scanBackward();
    }

    function setNextDisplayedImageStartToFinalScanPosition() {
        root.activeInteractionSurface.setNextDisplayedImageStartToFinalScanPosition();
    }

    function zoomByStep(stepCount, viewportX, viewportY) {
        return root.activeInteractionSurface.zoomByStep(stepCount, viewportX, viewportY);
    }

    function zoomByStepAtCenter(stepCount) {
        return root.activeInteractionSurface.zoomByStepAtCenter(stepCount);
    }

    objectName: "mediaViewportSlot"
    clip: true
    Controls.SplitView.fillHeight: true
    Controls.SplitView.minimumHeight: Kirigami.Units.gridUnit * 6

    MediaViewportInteractionSurface {
        id: inactiveInteractionSurface
    }

    Loader {
        id: mediaViewportDelegateLoader

        anchors.fill: parent
        active: root.imageMode || root.videoMode

        function unloadActiveDelegate() {
            if (root.activeDelegate !== null) {
                root.activeDelegate.presentationActive = false;
            }
            if (source.toString().length > 0) {
                source = "";
            }
        }

        function ensureActiveDelegateLoaded() {
            if (!active || root.activeDelegateSource.toString().length === 0) {
                unloadActiveDelegate();
                return;
            }

            if (source.toString() === root.activeDelegateSource.toString()) {
                return;
            }

            unloadActiveDelegate();
            setSource(root.activeDelegateSource, {
                "documentSession": root.documentSession,
                "objectName": root.imageMode ? "imageViewport" : "videoViewport",
                "presentationActive": true
            });
        }

        Component.onCompleted: ensureActiveDelegateLoaded()
        onActiveChanged: ensureActiveDelegateLoaded()
    }

    onActiveDelegateSourceChanged: mediaViewportDelegateLoader.ensureActiveDelegateLoaded()

    Connections {
        target: root.activeDelegate

        function onViewerClicked() {
            root.viewerClicked();
        }

        function onViewerContextMenuRequested(popupParent, position) {
            root.viewerContextMenuRequested(popupParent, position);
        }
    }

    ImageStateOverlay {
        anchors.fill: parent
        imageDocument: root.imageDocument
        imageReady: root.imageReady
        openAction: root.openAction
        visible: !root.videoMode
    }
}
