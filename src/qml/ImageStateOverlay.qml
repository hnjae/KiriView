// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Item {
    id: root

    required property KiriImageDocument imageDocument
    required property bool imageReady
    required property var openAction

    Kirigami.LoadingPlaceholder {
        anchors.centerIn: parent
        visible: root.imageDocument.status === KiriImageDocument.Loading
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 18)
    }

    Rectangle {
        anchors.left: parent.left
        anchors.margins: Kirigami.Units.largeSpacing
        anchors.top: parent.top
        color: Qt.rgba(0, 0, 0, 0.55)
        height: Kirigami.Units.gridUnit * 2
        radius: Kirigami.Units.smallSpacing
        visible: root.imageReady && root.imageDocument.loading
        width: height

        Controls.BusyIndicator {
            anchors.fill: parent
            anchors.margins: Kirigami.Units.smallSpacing
            running: visible
        }
    }

    Kirigami.InlineMessage {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.margins: Kirigami.Units.largeSpacing
        anchors.right: parent.right
        text: root.imageDocument.errorString
        type: Kirigami.MessageType.Error
        visible: root.imageReady && !root.imageDocument.loading && root.imageDocument.errorString.length > 0
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        helpfulAction: root.openAction
        icon.name: "image-x-generic-symbolic"
        text: "No image selected"
        visible: root.imageDocument.status === KiriImageDocument.Null
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 18)
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        explanation: root.imageDocument.errorString
        helpfulAction: root.openAction
        icon.name: "dialog-error-symbolic"
        text: "Unable to open image"
        visible: root.imageDocument.status === KiriImageDocument.Error
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 24)
    }
}
