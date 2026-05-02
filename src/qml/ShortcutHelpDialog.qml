// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.Dialog {
    required property real windowWidth
    required property real windowHeight

    closePolicy: Controls.Popup.CloseOnEscape
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
                    description: "First or last image in container"
                    keys: "Home / End / Ctrl+Home / Ctrl+End"
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
                    description: "Fit image"
                    keys: "1"
                }
                ListElement {
                    description: "Fit image height"
                    keys: "2"
                }
                ListElement {
                    description: "Fit image width"
                    keys: "3"
                }
                ListElement {
                    description: "Set 100% zoom"
                    keys: "0"
                }
                ListElement {
                    description: "Zoom around the cursor"
                    keys: "Ctrl+drag up/down"
                }
                ListElement {
                    description: "Pan the zoomed image"
                    keys: "Drag / Arrow keys"
                }
                ListElement {
                    description: "Pan the zoomed image horizontally"
                    keys: "Shift+wheel"
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
                id: shortcutRow

                required property string description
                required property string keys

                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                Controls.Label {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                    font.family: "monospace"
                    text: shortcutRow.keys
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }

                Controls.Label {
                    Layout.fillWidth: true
                    text: shortcutRow.description
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
