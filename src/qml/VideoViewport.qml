// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtMultimedia
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

FocusScope {
    id: root

    required property KiriVideoDocument videoDocument
    property bool active: true
    property alias controls: floatingControls
    property alias videoOutput: videoOutput
    readonly property bool videoReady: videoDocument.status === KiriVideoDocument.Ready
    readonly property real displayDevicePixelRatio: {
        const window = root.Window.window;
        const ratio = window ? window.devicePixelRatio : 1;
        return Number.isFinite(ratio) && ratio > 0 ? ratio : 1;
    }
    readonly property bool zoomPercentKnown: root.videoReady && videoOutput.sourceRect.width > 0 && videoOutput.sourceRect.height > 0 && videoOutput.contentRect.width > 0 && videoOutput.contentRect.height > 0
    readonly property int zoomPercent: zoomPercentKnown ? videoZoomPercentForRects(videoOutput.contentRect, videoOutput.sourceRect, displayDevicePixelRatio) : 0
    property KiriVideoDocument attachedVideoDocument: null

    signal viewerClicked

    function videoZoomPercentForRects(contentRect, sourceRect, devicePixelRatio) {
        if (sourceRect.width <= 0 || sourceRect.height <= 0 || contentRect.width <= 0 || contentRect.height <= 0 || !Number.isFinite(devicePixelRatio) || devicePixelRatio <= 0) {
            return 0;
        }

        const widthZoom = contentRect.width * devicePixelRatio / sourceRect.width * 100;
        const heightZoom = contentRect.height * devicePixelRatio / sourceRect.height * 100;
        const zoom = Math.min(widthZoom, heightZoom);
        return Number.isFinite(zoom) && zoom > 0 ? Math.round(zoom) : 0;
    }

    function shouldAttachVideoOutput() {
        return root.active && root.visible && root.videoDocument !== null;
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

    activeFocusOnTab: true
    focus: true

    onActiveChanged: updateVideoOutputAttachment()
    onVideoDocumentChanged: updateVideoOutputAttachment()
    onVisibleChanged: updateVideoOutputAttachment()

    Component.onCompleted: updateVideoOutputAttachment()
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
            fillMode: VideoOutput.PreserveAspectFit
            visible: root.videoReady && root.videoDocument.hasVideo
        }
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton

        onTapped: {
            root.forceActiveFocus();
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
