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
    readonly property KiriVideoDocument videoDocument: documentSession.videoDocument
    readonly property bool imageMode: documentSession.documentKind === KiriDocumentSession.Image
    readonly property bool videoMode: documentSession.documentKind === KiriDocumentSession.Video
    readonly property bool imageReady: imageMode && imageDocument.status === KiriImageDocument.Ready
    readonly property bool imageHorizontallyPannable: imageViewportProperty("imageHorizontallyPannable", false)
    readonly property bool imagePannable: imageViewportProperty("imagePannable", false)
    readonly property real imageViewportWidth: imageViewportProperty("viewportWidth", 0)
    readonly property real imageViewportHeight: imageViewportProperty("viewportHeight", 0)

    signal viewerClicked
    signal viewerContextMenuRequested(var popupParent, point position)

    function forceActiveViewportFocus() {
        if (root.videoMode) {
            videoViewportLoader.forceVideoViewportFocus();
            return;
        }

        root.callImageViewport("forceActiveFocus", undefined, []);
    }

    function imageViewportProperty(propertyName, fallbackValue) {
        if (imageViewportLoader.item === null) {
            return fallbackValue;
        }

        return imageViewportLoader.item[propertyName];
    }

    function imageViewportFunction(functionName) {
        if (imageViewportLoader.item === null) {
            return null;
        }

        return imageViewportLoader.item[functionName];
    }

    function callImageViewport(functionName, fallbackValue, args) {
        const fn = root.imageViewportFunction(functionName);
        if (fn === null) {
            return fallbackValue;
        }

        return fn.apply(imageViewportLoader.item, args);
    }

    function panBy(deltaX, deltaY) {
        return root.callImageViewport("panBy", false, [deltaX, deltaY]);
    }

    function panToBottomRight() {
        return root.callImageViewport("panToBottomRight", false, []);
    }

    function panToTopLeft() {
        return root.callImageViewport("panToTopLeft", false, []);
    }

    function scanForward() {
        return root.callImageViewport("scanForward", false, []);
    }

    function scanBackward() {
        return root.callImageViewport("scanBackward", false, []);
    }

    function setNextDisplayedImageStartToFinalScanPosition() {
        root.callImageViewport("setNextDisplayedImageStartToFinalScanPosition", undefined, []);
    }

    function zoomByStep(stepCount, viewportX, viewportY) {
        return root.callImageViewport("zoomByStep", false, [stepCount, viewportX, viewportY]);
    }

    function zoomByStepAtCenter(stepCount) {
        return root.zoomByStep(stepCount, root.imageViewportWidth / 2, root.imageViewportHeight / 2);
    }

    objectName: "mediaViewportSlot"
    clip: true
    Controls.SplitView.fillHeight: true
    Controls.SplitView.minimumHeight: Kirigami.Units.gridUnit * 6

    Loader {
        id: imageViewportLoader

        anchors.fill: parent
        active: root.imageMode

        function ensureImageViewportLoaded() {
            if (!active) {
                source = "";
                return;
            }

            if (source.toString().length === 0) {
                setSource(Qt.resolvedUrl("ImageViewport.qml"), {
                    "imageDocument": root.imageDocument,
                    "objectName": "imageViewport",
                    "presentationActive": true
                });
            }
        }

        Component.onCompleted: ensureImageViewportLoaded()
        onActiveChanged: ensureImageViewportLoaded()
    }

    Connections {
        target: imageViewportLoader.item

        function onViewerClicked() {
            root.viewerClicked();
        }
    }

    Loader {
        id: videoViewportLoader

        anchors.fill: parent
        active: root.videoMode

        function ensureVideoViewportLoaded() {
            if (!active) {
                source = "";
                return;
            }

            if (source.toString().length === 0) {
                setSource(Qt.resolvedUrl("VideoViewport.qml"), {
                    "objectName": "videoViewport",
                    "videoDocument": root.videoDocument
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

    ImageStateOverlay {
        anchors.fill: parent
        imageDocument: root.imageDocument
        imageReady: root.imageReady
        openAction: root.openAction
        visible: !root.videoMode
    }

    TapHandler {
        id: viewerContextMenuTapHandler

        acceptedButtons: Qt.RightButton
        enabled: root.imageMode || root.videoMode

        onTapped: root.viewerContextMenuRequested(root, viewerContextMenuTapHandler.point.position)
    }
}
