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

    Shortcut {
        context: Qt.WindowShortcut
        sequence: "Esc"

        onActivated: root.close()
    }

    pageStack.initialPage: Kirigami.Page {
        id: page

        background: Rectangle {
            color: "#3c3c3c"
        }

        padding: 0

        readonly property bool imageReady: imageView.status === KiriImageView.Ready

        actions: [
            Kirigami.Action {
                icon.name: "document-open-symbolic"
                shortcut: StandardKey.Open
                text: "Open"

                onTriggered: fileDialog.open()
            },
            Kirigami.Action {
                enabled: page.imageReady
                icon.name: "go-previous-symbolic"
                text: "Previous"

                onTriggered: imageView.openPreviousImage()
            },
            Kirigami.Action {
                enabled: page.imageReady
                icon.name: "go-next-symbolic"
                text: "Next"

                onTriggered: imageView.openNextImage()
            },
            Kirigami.Action {
                enabled: page.imageReady
                icon.name: "zoom-fit-best-symbolic"
                text: "Fit"

                onTriggered: imageView.resetZoom()
            },
            Kirigami.Action {
                enabled: page.imageReady
                text: "Zoom"

                displayComponent: Controls.SpinBox {
                    id: zoomSpinBox

                    editable: true
                    enabled: page.imageReady
                    from: Math.min(10, Math.floor(imageView.zoomPercent))
                    implicitWidth: Kirigami.Units.gridUnit * 5
                    stepSize: 10
                    to: 800
                    value: Math.round(imageView.zoomPercent)

                    textFromValue: function (value, locale) {
                        return Number(value).toLocaleString(locale, "f", 0) + "%";
                    }

                    valueFromText: function (text, locale) {
                        const withoutPercent = text.toString().replace("%", "").trim();
                        const parsedValue = Number.fromLocaleString(locale, withoutPercent);
                        return Number.isFinite(parsedValue) ? Math.round(parsedValue) : zoomSpinBox.value;
                    }

                    validator: IntValidator {
                        bottom: zoomSpinBox.from
                        top: zoomSpinBox.to
                    }

                    onValueModified: imageView.zoomPercent = value
                }
            }
        ]

        Flickable {
            id: imageFlickable

            anchors.fill: parent
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            contentHeight: Math.max(height, imageView.y + imageView.height)
            contentWidth: Math.max(width, imageView.x + imageView.width)
            interactive: imageView.status === KiriImageView.Ready && (contentWidth > width || contentHeight > height)

            Controls.ScrollBar.horizontal: Controls.ScrollBar {
                policy: imageFlickable.contentWidth > imageFlickable.width ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
            }

            Controls.ScrollBar.vertical: Controls.ScrollBar {
                policy: imageFlickable.contentHeight > imageFlickable.height ? Controls.ScrollBar.AsNeeded : Controls.ScrollBar.AlwaysOff
            }

            KiriImageView {
                id: imageView

                height: displaySize.height
                viewportSize: Qt.size(imageFlickable.width, imageFlickable.height)
                width: displaySize.width
                x: Math.max(0, (imageFlickable.width - width) / 2)
                y: Math.max(0, (imageFlickable.height - height) / 2)
            }
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
