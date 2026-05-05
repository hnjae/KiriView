// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.Dialog {
    id: root

    required property real windowWidth
    required property real windowHeight

    closePolicy: Controls.Popup.CloseOnEscape
    modal: true
    standardButtons: Controls.Dialog.Close
    title: "Keyboard Shortcuts"
    width: Math.min(windowWidth - Kirigami.Units.largeSpacing * 2, Kirigami.Units.gridUnit * 28)
    x: Math.round((windowWidth - width) / 2)
    y: Math.round((windowHeight - height) / 2)

    ShortcutDefinitions {
        id: shortcutDefinitions
    }

    contentItem: ColumnLayout {
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: shortcutDefinitions.helpEntries

            delegate: RowLayout {
                id: shortcutRow

                required property var modelData

                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                Controls.Label {
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                    font.family: "monospace"
                    text: shortcutRow.modelData.keyText
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }

                Controls.Label {
                    Layout.fillWidth: true
                    text: shortcutRow.modelData.description
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
