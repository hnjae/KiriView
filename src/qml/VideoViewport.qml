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
    readonly property bool videoReady: root.documentSession.activeVideoReady
    readonly property bool videoControlsReady: root.documentSession.activeVideoControlsReady
    readonly property bool fixedControlsMode: Kirigami.Settings.isMobile || Kirigami.Settings.hasTransientTouchInput || Kirigami.Units.longDuration <= 0 || width < Kirigami.Units.gridUnit * 32 || height < Kirigami.Units.gridUnit * 16
    readonly property bool controlsReserveSpace: floatingControls.visible && root.fixedControlsMode
    readonly property bool controlsEffectivelyShown: floatingControls.visible && floatingControls.opacity > 0
    property int videoOutputClaimRevision: 0

    function shouldAttachVideoOutput() {
        return root.presentationActive && root.visible && root.videoDocument !== null;
    }

    function nextVideoOutputClaimRevision() {
        root.videoOutputClaimRevision += 1;
        return root.videoOutputClaimRevision;
    }

    function updateVideoOutputAttachment() {
        const attach = shouldAttachVideoOutput();
        root.documentSession.reportVideoOutputSurfaceClaim(root.nextVideoOutputClaimRevision(), root.documentSession.publicProjectionRevision, root, attach ? videoOutput : null, attach, videoOutput.contentRect, videoOutput.sourceRect);
    }

    function reportVideoOutputGeometry() {
        root.updateVideoOutputAttachment();
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

    Connections {
        target: root.documentSession

        function onPublicProjectionRevisionChanged() {
            root.updateVideoOutputAttachment();
        }
    }

    Component.onCompleted: {
        updateVideoOutputAttachment();
        reportVideoOutputGeometry();
    }
    Component.onDestruction: {
        root.documentSession.reportVideoOutputSurfaceClaim(root.nextVideoOutputClaimRevision(), root.documentSession.publicProjectionRevision, root, null, false, Qt.rect(0, 0, 0, 0), Qt.rect(0, 0, 0, 0));
    }

    Keys.onPressed: event => {
        if (root.handleSeekShortcut(event)) {
            event.accepted = true;
        }
    }

    Rectangle {
        id: videoSurface

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: root.controlsReserveSpace ? floatingControls.top : parent.bottom
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

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        z: 1

        onPositionChanged: floatingControls.revealControls()
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton

        onTapped: {
            floatingControls.revealControls();
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
        anchors.bottomMargin: root.fixedControlsMode ? 0 : Kirigami.Units.largeSpacing
        anchors.horizontalCenter: parent.horizontalCenter
        fixedMode: root.fixedControlsMode
        videoDocument: root.videoDocument
        visible: root.videoControlsReady
        z: 10
    }

    Kirigami.InlineMessage {
        anchors.bottom: root.controlsEffectivelyShown ? floatingControls.top : parent.bottom
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
