// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    title: "KiriView"
    visible: true

    minimumWidth: Kirigami.Units.gridUnit * 28
    minimumHeight: Kirigami.Units.gridUnit * 20
    width: minimumWidth
    height: minimumHeight

    pageStack.initialPage: Kirigami.Page {
        id: page

        title: "KiriView"

        actions: [
            Kirigami.Action {
                icon.name: "document-open-symbolic"
                shortcut: StandardKey.Open
                text: "Open"

                onTriggered: fileDialog.open()
            }
        ]

        KiriImageView {
            id: imageView

            anchors.fill: parent
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: StandardKey.MoveToPreviousPage

            onActivated: imageView.openPreviousImage()
        }

        Shortcut {
            context: Qt.WindowShortcut
            sequence: StandardKey.MoveToNextPage

            onActivated: imageView.openNextImage()
        }

        RowLayout {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing
            visible: imageView.status === KiriImageView.Ready

            Controls.Button {
                icon.name: "go-previous-symbolic"
                text: "Previous"

                onClicked: imageView.openPreviousImage()
            }

            Controls.Button {
                icon.name: "go-next-symbolic"
                text: "Next"

                onClicked: imageView.openNextImage()
            }
        }

        Controls.BusyIndicator {
            anchors.centerIn: parent
            running: visible
            visible: imageView.status === KiriImageView.Loading
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing
            visible: imageView.status === KiriImageView.Null
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
                icon.name: "document-open-symbolic"
                text: "Open"

                onClicked: fileDialog.open()
            }
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: Kirigami.Units.largeSpacing
            visible: imageView.status === KiriImageView.Error
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
                text: imageView.errorString
                textFormat: Text.PlainText
                visible: text.length > 0
                wrapMode: Text.Wrap
            }

            Controls.Button {
                Layout.alignment: Qt.AlignHCenter
                icon.name: "document-open-symbolic"
                text: "Open"

                onClicked: fileDialog.open()
            }
        }
    }

    Dialogs.FileDialog {
        id: fileDialog

        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: ["Image and comic book files (*.avif *.bmp *.cbz *.gif *.jpeg *.jpg *.png *.svg *.webp)", "All files (*)"]
        title: "Open Image or Comic Book"

        onAccepted: imageView.sourceUrl = selectedFile
    }
}
