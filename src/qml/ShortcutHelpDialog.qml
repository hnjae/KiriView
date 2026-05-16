// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

Kirigami.Dialog {
    id: root

    required property KiriViewApplication application

    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnReleaseOutside
    focus: true
    preferredWidth: Kirigami.Units.gridUnit * 28
    showCloseButton: true
    standardButtons: Kirigami.Dialog.Ok
    title: KI18n.i18nc("@title:window", "Keyboard Shortcuts")

    onOpened: {
        const okButton = root.standardButton(Kirigami.Dialog.Ok);
        if (okButton) {
            okButton.forceActiveFocus();
        }
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.opened
        sequences: ["Return", "Enter"]

        onActivated: root.accept()
    }

    ColumnLayout {
        implicitWidth: Kirigami.Units.gridUnit * 28
        spacing: Kirigami.Units.smallSpacing

        Repeater {
            model: root.application.shortcutHelpModel

            delegate: RowLayout {
                id: shortcutRow

                required property string actionText
                required property string shortcutText

                Layout.fillWidth: true
                spacing: Kirigami.Units.largeSpacing

                Controls.Label {
                    objectName: "shortcutTextLabel"

                    Layout.preferredWidth: Kirigami.Units.gridUnit * 9
                    font.family: "monospace"
                    text: shortcutRow.shortcutText
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }

                Controls.Label {
                    objectName: "shortcutActionLabel"

                    Layout.fillWidth: true
                    text: shortcutRow.actionText
                    textFormat: Text.PlainText
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
