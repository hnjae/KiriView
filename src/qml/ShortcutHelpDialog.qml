// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.Dialog {
    required property real windowWidth
    required property real windowHeight

    closePolicy: Controls.Popup.NoAutoClose
    modal: true
    standardButtons: Controls.Dialog.Close
    title: "Keyboard Shortcuts"
    width: Math.min(windowWidth - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 28)
    x: Math.round((windowWidth - width) / 2)
    y: Math.round((windowHeight - height) / 2)

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: ListModel {
                ListElement {
                    description: "Open an image or comic book"
                    keys: "Ctrl+O"
                }
                ListElement {
                    description: "Close dialog or KiriView"
                    keys: "Esc"
                }
                ListElement {
                    description: "Previous or next image"
                    keys: "Page Up / Page Down"
                }
                ListElement {
                    description: "Zoom in"
                    keys: "Ctrl+= / Ctrl++"
                }
                ListElement {
                    description: "Zoom out"
                    keys: "Ctrl+-"
                }
                ListElement {
                    description: "Zoom around the cursor"
                    keys: "Ctrl+drag up/down"
                }
                ListElement {
                    description: "Pan the zoomed image"
                    keys: "Arrow keys"
                }
                ListElement {
                    description: "Previous or next container"
                    keys: "[ / ]"
                }
                ListElement {
                    description: "Toggle fullscreen"
                    keys: "F / F11"
                }
                ListElement {
                    description: "Show this shortcut help"
                    keys: "? / F1"
                }
            }

            delegate: RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                Controls.Label {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                    font.family: "monospace"
                    text: keys
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }

                Controls.Label {
                    Layout.fillWidth: true
                    text: description
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
