// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtMultimedia
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

MediaViewportDelegate {
    id: root

    property alias controls: floatingControls
    property alias videoOutput: videoOutput
    readonly property var videoDocument: root.documentSession.videoDocument
    readonly property bool videoReady: videoDocument.status === KiriVideoDocument.Ready
    property KiriVideoDocument attachedVideoDocument: null

    function shouldAttachVideoOutput() {
        return root.presentationActive && root.visible && root.videoDocument !== null;
    }

    function detachVideoOutput(document) {
        if (document !== null && document.videoOutput === videoOutput) {
            document.videoOutput = null;
        }
    }

    function updateVideoOutputAttachment() {
        const nextDocument = shouldAttachVideoOutput() ? root.videoDocument : null;

        if (root.attachedVideoDocument !== null && root.attachedVideoDocument !== nextDocument) {
            detachVideoOutput(root.attachedVideoDocument);
        }

        root.attachedVideoDocument = nextDocument;

        if (nextDocument !== null && nextDocument.videoOutput !== videoOutput) {
            nextDocument.videoOutput = videoOutput;
        }
        reportVideoOutputGeometry();
    }

    function reportVideoOutputGeometry() {
        if (root.videoDocument !== null) {
            root.videoDocument.setVideoOutputGeometry(videoOutput.contentRect, videoOutput.sourceRect);
        }
    }

    function seekByShortcut(deltaMilliseconds) {
        if (root.videoDocument.seekable) {
            root.videoDocument.seekBy(deltaMilliseconds);
        }
    }

    function handleSeekShortcut(event) {
        if (event.modifiers !== Qt.AltModifier) {
            return false;
        }

        switch (event.key) {
        case Qt.Key_Left:
            root.seekByShortcut(-5000);
            return true;
        case Qt.Key_Right:
            root.seekByShortcut(5000);
            return true;
        case Qt.Key_Up:
            root.seekByShortcut(45000);
            return true;
        case Qt.Key_Down:
            root.seekByShortcut(-45000);
            return true;
        default:
            return false;
        }
    }

    onPresentationActiveChanged: updateVideoOutputAttachment()
    onVideoDocumentChanged: updateVideoOutputAttachment()
    onVisibleChanged: updateVideoOutputAttachment()

    Component.onCompleted: {
        updateVideoOutputAttachment();
        reportVideoOutputGeometry();
    }
    Component.onDestruction: {
        detachVideoOutput(root.attachedVideoDocument);
        root.attachedVideoDocument = null;
    }

    Keys.onPressed: event => {
        if (root.handleSeekShortcut(event)) {
            event.accepted = true;
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "black"

        VideoOutput {
            id: videoOutput

            anchors.fill: parent
            endOfStreamPolicy: VideoOutput.KeepLastFrame
            fillMode: VideoOutput.PreserveAspectFit
            visible: root.videoReady && root.videoDocument.hasVideo

            onContentRectChanged: root.reportVideoOutputGeometry()
            onSourceRectChanged: root.reportVideoOutputGeometry()
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton

        onTapped: {
            root.requestViewportFocus();
            root.viewerClicked();
        }
    }

    Kirigami.LoadingPlaceholder {
        anchors.centerIn: parent
        visible: root.videoDocument.status === KiriVideoDocument.Loading
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 18)
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        icon.name: "video-x-generic-symbolic"
        text: KI18n.i18nc("@info:placeholder", "No video selected")
        visible: root.videoDocument.status === KiriVideoDocument.Null
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 18)
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        icon.name: "video-x-generic-symbolic"
        text: KI18n.i18nc("@info:placeholder", "No video track")
        visible: root.videoReady && !root.videoDocument.hasVideo
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 18)
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        explanation: root.videoDocument.errorString
        icon.name: "dialog-error-symbolic"
        text: KI18n.i18nc("@info:placeholder", "Unable to open video")
        visible: root.videoDocument.status === KiriVideoDocument.Error
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 24)
    }

    VideoFloatingControls {
        id: floatingControls

        anchors.bottom: parent.bottom
        anchors.bottomMargin: Kirigami.Units.largeSpacing
        anchors.horizontalCenter: parent.horizontalCenter
        enabled: visible
        videoDocument: root.videoDocument
        visible: root.videoReady
        z: 10
    }

    Kirigami.InlineMessage {
        anchors.bottom: floatingControls.visible ? floatingControls.top : parent.bottom
        anchors.bottomMargin: Kirigami.Units.largeSpacing
        anchors.left: parent.left
        anchors.leftMargin: Kirigami.Units.largeSpacing
        anchors.right: parent.right
        anchors.rightMargin: Kirigami.Units.largeSpacing
        text: root.videoDocument.errorString
        type: Kirigami.MessageType.Error
        visible: root.videoReady && root.videoDocument.errorString.length > 0
    }
}
