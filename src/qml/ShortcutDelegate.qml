// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.ki18n
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.AbstractFormDelegate {
    id: root

    required property string actionText
    required property string scopeText
    required property string shortcutText
    required property var shortcutKeyTexts
    readonly property bool narrow: width < Kirigami.Units.gridUnit * 22

    objectName: "shortcutDelegate"
    Accessible.name: root.shortcutText.length > 0 ? KI18n.i18nc("@info:accessible", "%1, %2 shortcut %3", root.actionText, root.scopeText, root.shortcutText) : KI18n.i18nc("@info:accessible", "%1, %2", root.actionText, root.scopeText)
    text: root.actionText

    contentItem: GridLayout {
        columnSpacing: Kirigami.Units.largeSpacing
        columns: root.narrow ? 1 : 2
        rowSpacing: Kirigami.Units.smallSpacing

        ColumnLayout {
            id: actionLabel

            property string text: root.actionText

            objectName: "shortcutActionLabel"

            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
            spacing: 0

            Controls.Label {
                Accessible.ignored: true
                Layout.fillWidth: true
                color: Kirigami.Theme.textColor
                elide: Text.ElideRight
                maximumLineCount: root.narrow ? 2 : 1
                text: root.actionText
                textFormat: Text.PlainText
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.Wrap
            }

            Controls.Label {
                objectName: "shortcutScopeLabel"

                Accessible.ignored: true
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                elide: Text.ElideRight
                font: Kirigami.Theme.smallFont
                text: root.scopeText
                textFormat: Text.PlainText
            }
        }

        Flow {
            id: keycapFlow

            Layout.alignment: root.narrow ? Qt.AlignLeft : Qt.AlignRight | Qt.AlignVCenter
            Layout.fillWidth: root.narrow
            Layout.maximumWidth: root.narrow ? root.width : root.width / 2
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: root.shortcutKeyTexts

                delegate: Controls.Label {
                    id: keycap

                    required property string modelData

                    objectName: "shortcutKeycap"

                    Accessible.ignored: true
                    bottomPadding: Kirigami.Units.smallSpacing / 2
                    color: Kirigami.Theme.textColor
                    font.family: Kirigami.Theme.fixedWidthFont.family
                    font.pointSize: Kirigami.Theme.smallFont.pointSize
                    leftPadding: Kirigami.Units.smallSpacing
                    rightPadding: Kirigami.Units.smallSpacing
                    text: keycap.modelData
                    textFormat: Text.PlainText
                    topPadding: Kirigami.Units.smallSpacing / 2

                    background: Rectangle {
                        border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
                        color: Kirigami.Theme.alternateBackgroundColor
                        radius: Kirigami.Units.cornerRadius
                    }
                }
            }
        }
    }
}
