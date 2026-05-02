// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Item {
    id: root

    required property KiriImageDocument imageDocument
    required property bool imageReady
    required property var openAction

    Controls.BusyIndicator {
        anchors.centerIn: parent
        running: visible
        visible: root.imageDocument.status === KiriImageDocument.Loading
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

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Kirigami.Units.largeSpacing
        visible: root.imageDocument.status === KiriImageDocument.Null
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 18)

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            implicitHeight: Kirigami.Units.iconSizes.huge
            implicitWidth: Kirigami.Units.iconSizes.huge
            source: "image-x-generic-symbolic"
        }

        Controls.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: "No image selected"
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }

        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            action: root.openAction
        }
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Kirigami.Units.largeSpacing
        visible: root.imageDocument.status === KiriImageDocument.Error
        width: Math.min(parent.width - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 24)

        Kirigami.Icon {
            Layout.alignment: Qt.AlignHCenter
            implicitHeight: Kirigami.Units.iconSizes.huge
            implicitWidth: Kirigami.Units.iconSizes.huge
            source: "dialog-error-symbolic"
        }

        Controls.Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            text: "Unable to open image"
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
        }

        Controls.Label {
            Layout.fillWidth: true
            color: Kirigami.Theme.disabledTextColor
            horizontalAlignment: Text.AlignHCenter
            text: root.imageDocument.errorString
            textFormat: Text.PlainText
            visible: text.length > 0
            wrapMode: Text.Wrap
        }

        Controls.Button {
            Layout.alignment: Qt.AlignHCenter
            action: root.openAction
        }
    }
}
