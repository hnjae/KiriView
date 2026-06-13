// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

Controls.Pane {
    id: root

    required property KiriDocumentSession documentSession
    readonly property var mediaInformation: documentSession.mediaInformation

    signal closeRequested

    implicitWidth: Kirigami.Units.gridUnit * 18
    leftPadding: Kirigami.Units.largeSpacing
    rightPadding: Kirigami.Units.largeSpacing
    topPadding: Kirigami.Units.largeSpacing
    bottomPadding: Kirigami.Units.largeSpacing

    component MetadataSection: ColumnLayout {
        id: sectionRoot

        required property var rowModel
        required property string title
        property bool collapsible: false
        property bool initiallyExpanded: true
        property bool expanded: initiallyExpanded
        property bool sectionVisible: true

        visible: sectionVisible
        spacing: Kirigami.Units.smallSpacing

        RowLayout {
            Layout.fillWidth: true

            Controls.Label {
                Layout.fillWidth: true
                color: Kirigami.Theme.textColor
                elide: Text.ElideRight
                font.bold: true
                maximumLineCount: 1
                text: sectionRoot.title
                textFormat: Text.PlainText
            }

            Controls.ToolButton {
                Accessible.name: sectionRoot.title
                display: Controls.AbstractButton.IconOnly
                icon.name: sectionRoot.expanded ? "go-up-symbolic" : "go-down-symbolic"
                visible: sectionRoot.collapsible

                onClicked: sectionRoot.expanded = !sectionRoot.expanded
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Kirigami.Theme.disabledTextColor
            opacity: 0.25
        }

        Repeater {
            model: sectionRoot.rowModel
            visible: sectionRoot.expanded

            delegate: RowLayout {
                id: rowDelegate

                required property string label
                required property string value

                spacing: Kirigami.Units.smallSpacing

                Controls.Label {
                    Layout.fillWidth: true
                    Layout.minimumWidth: Kirigami.Units.gridUnit * 5
                    Layout.preferredWidth: Kirigami.Units.gridUnit * 7
                    color: Kirigami.Theme.disabledTextColor
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    text: rowDelegate.label
                    textFormat: Text.PlainText
                    wrapMode: Text.NoWrap
                }

                Controls.Label {
                    Layout.fillWidth: true
                    color: Kirigami.Theme.textColor
                    elide: Text.ElideMiddle
                    horizontalAlignment: Text.AlignRight
                    maximumLineCount: 1
                    text: rowDelegate.value
                    textFormat: Text.PlainText
                    wrapMode: Text.NoWrap
                }
            }
        }
    }

    background: Rectangle {
        color: Kirigami.Theme.backgroundColor
    }

    contentItem: Controls.ScrollView {
        id: infoPanelScrollView

        objectName: "infoPanelScrollView"

        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            id: contentLayout

            spacing: Kirigami.Units.largeSpacing
            width: infoPanelScrollView.availableWidth

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Icon {
                    Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
                    Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
                    source: "documentinfo-symbolic"
                }

                Controls.Label {
                    objectName: "infoPanelHeaderTitle"

                    Layout.fillWidth: true
                    color: Kirigami.Theme.textColor
                    elide: Text.ElideRight
                    font.bold: true
                    maximumLineCount: 1
                    text: KI18n.i18nc("@title:window", "Information")
                    textFormat: Text.PlainText
                }

                Controls.ToolButton {
                    objectName: "infoPanelCloseButton"

                    Accessible.name: KI18n.i18nc("@action:button", "Close Information Panel")
                    display: Controls.AbstractButton.IconOnly
                    icon.name: "window-close-symbolic"

                    Controls.ToolTip.text: Accessible.name
                    Controls.ToolTip.visible: hovered && !Kirigami.Settings.hasTransientTouchInput

                    onClicked: root.closeRequested()
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                visible: root.mediaInformation.available

                Controls.Label {
                    objectName: "infoPanelTitle"

                    Layout.fillWidth: true
                    color: Kirigami.Theme.textColor
                    elide: Text.ElideRight
                    font.bold: true
                    maximumLineCount: 1
                    text: root.mediaInformation.title
                    textFormat: Text.PlainText
                }

                Controls.Label {
                    objectName: "infoPanelSummary"

                    Layout.fillWidth: true
                    color: Kirigami.Theme.disabledTextColor
                    elide: Text.ElideRight
                    maximumLineCount: 1
                    text: root.mediaInformation.summary
                    textFormat: Text.PlainText
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing
                visible: root.mediaInformation.available

                Controls.ToolButton {
                    objectName: "infoPanelCopyPathButton"

                    Accessible.name: KI18n.i18nc("@action:button", "Copy File Path")
                    display: Controls.AbstractButton.IconOnly
                    enabled: root.mediaInformation.canCopyFilePath
                    icon.name: "edit-copy-symbolic"

                    Controls.ToolTip.text: Accessible.name
                    Controls.ToolTip.visible: hovered && !Kirigami.Settings.hasTransientTouchInput

                    onClicked: root.mediaInformation.copyFilePath()
                }

                Controls.ToolButton {
                    objectName: "infoPanelOpenFolderButton"

                    Accessible.name: KI18n.i18nc("@action:button", "Open Containing Folder")
                    display: Controls.AbstractButton.IconOnly
                    enabled: root.mediaInformation.canOpenContainingFolder
                    icon.name: "document-open-folder-symbolic"

                    Controls.ToolTip.text: Accessible.name
                    Controls.ToolTip.visible: hovered && !Kirigami.Settings.hasTransientTouchInput

                    onClicked: root.mediaInformation.openContainingFolder()
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            Controls.Label {
                Layout.fillWidth: true
                color: Kirigami.Theme.disabledTextColor
                elide: Text.ElideRight
                maximumLineCount: 1
                text: root.mediaInformation.summary
                textFormat: Text.PlainText
                visible: !root.mediaInformation.available
            }

            MetadataSection {
                Layout.fillWidth: true
                rowModel: root.mediaInformation.generalRows
                sectionVisible: root.mediaInformation.available
                title: KI18n.i18nc("@title:group", "General")
            }

            MetadataSection {
                Layout.fillWidth: true
                rowModel: root.mediaInformation.mediaRows
                sectionVisible: root.mediaInformation.available
                title: root.mediaInformation.mediaSectionTitle
            }

            MetadataSection {
                Layout.fillWidth: true
                rowModel: root.mediaInformation.cameraRows
                sectionVisible: root.mediaInformation.hasCameraSection
                title: KI18n.i18nc("@title:group", "Camera")
            }

            MetadataSection {
                Layout.fillWidth: true
                collapsible: true
                initiallyExpanded: false
                rowModel: root.mediaInformation.advancedRows
                sectionVisible: root.mediaInformation.hasAdvancedSection
                title: KI18n.i18nc("@title:group", "Advanced Metadata")
            }

            Item {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }
}
