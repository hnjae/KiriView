// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Controls.Pane {
    id: root

    required property KiriDocumentSession documentSession

    leftPadding: Kirigami.Units.largeSpacing
    rightPadding: Kirigami.Units.largeSpacing
    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.smallSpacing

    contentItem: ListView {
        id: thumbnailStrip

        objectName: "thumbnailStrip"

        boundsBehavior: Flickable.StopAtBounds
        clip: true
        model: root.documentSession.activeNavigationThumbnailModel
        orientation: ListView.Horizontal
        spacing: Kirigami.Units.smallSpacing

        Controls.ScrollBar.horizontal: Controls.ScrollBar {
            policy: Controls.ScrollBar.AsNeeded
        }

        delegate: Controls.ItemDelegate {
            id: thumbnailDelegate

            required property bool current
            required property string iconName
            required property string label
            required property int number

            objectName: "thumbnailStripItem"

            Accessible.name: label
            Accessible.role: Accessible.Button
            enabled: root.documentSession.activeNavigationEditable
            height: thumbnailStrip.height
            hoverEnabled: true
            width: Kirigami.Units.gridUnit * 8

            Controls.ToolTip.text: label
            Controls.ToolTip.visible: hovered && label.length > 0 && !Kirigami.Settings.hasTransientTouchInput

            background: Rectangle {
                color: thumbnailDelegate.current ? Kirigami.Theme.highlightColor : "transparent"
                opacity: thumbnailDelegate.current ? 0.22 : 0
                radius: 4
            }

            contentItem: ColumnLayout {
                spacing: Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2))

                Kirigami.Icon {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredHeight: Kirigami.Units.iconSizes.enormous
                    Layout.preferredWidth: Kirigami.Units.iconSizes.enormous
                    source: thumbnailDelegate.iconName
                }

                Controls.Label {
                    Layout.fillWidth: true
                    color: Kirigami.Theme.textColor
                    elide: Text.ElideRight
                    font: Kirigami.Theme.fixedWidthFont
                    horizontalAlignment: Text.AlignHCenter
                    maximumLineCount: 1
                    text: thumbnailDelegate.label
                    textFormat: Text.PlainText
                    wrapMode: Text.NoWrap
                }
            }

            onClicked: root.documentSession.openActiveNavigationAtNumber(number)
        }
    }
}
