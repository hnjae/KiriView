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
    property alias imageViewport: imageViewport
    readonly property KiriImageDocument imageDocument: documentSession.imageDocument
    readonly property KiriVideoDocument videoDocument: documentSession.videoDocument
    readonly property bool imageMode: documentSession.documentKind === KiriDocumentSession.Image
    readonly property bool videoMode: documentSession.documentKind === KiriDocumentSession.Video
    readonly property bool imageReady: imageMode && imageDocument.status === KiriImageDocument.Ready

    signal viewerClicked
    signal viewerContextMenuRequested(var popupParent, point position)

    function forceActiveViewportFocus() {
        if (root.videoMode) {
            videoViewportLoader.forceVideoViewportFocus();
            return;
        }

        imageViewport.forceActiveFocus();
    }

    objectName: "mediaViewportSlot"
    clip: true
    Controls.SplitView.fillHeight: true
    Controls.SplitView.minimumHeight: Kirigami.Units.gridUnit * 6

    ImageViewport {
        id: imageViewport

        anchors.fill: parent
        imageDocument: root.imageDocument
        presentationActive: root.imageMode
        visible: !root.videoMode

        onViewerClicked: root.viewerClicked()
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
