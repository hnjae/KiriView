// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.FormCardDialog {
    id: root

    required property KiriViewApplication application
    readonly property real dialogMargin: Kirigami.Units.gridUnit
    readonly property real maximumDialogWidth: Kirigami.Units.gridUnit * 34
    readonly property real maximumDialogHeight: Kirigami.Units.gridUnit * 32
    readonly property real minimumDialogWidth: Kirigami.Units.gridUnit * 14
    readonly property real availableDialogWidth: parent ? Math.max(minimumDialogWidth, parent.width - dialogMargin * 2) : maximumDialogWidth
    readonly property real availableDialogHeight: parent ? Math.max(Kirigami.Units.gridUnit * 12, parent.height - dialogMargin * 2) : maximumDialogHeight
    readonly property real formMaximumWidth: Kirigami.Units.gridUnit * 32

    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnReleaseOutside
    focus: true
    height: Math.min(implicitHeight, availableDialogHeight, maximumDialogHeight)
    parent: Controls.Overlay.overlay
    standardButtons: Controls.Dialog.Ok
    title: KI18n.i18nc("@title:window", "Keyboard Shortcuts")
    width: Math.min(maximumDialogWidth, availableDialogWidth)

    onOpened: {
        const okButton = root.standardButton(Controls.Dialog.Ok);
        if (okButton) {
            okButton.forceActiveFocus();
        }
    }

    header: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            bottomPadding: 0
            elide: Text.ElideRight
            leftPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
            rightPadding: Kirigami.Units.smallSpacing
            text: root.title
            topPadding: Kirigami.Units.largeSpacing + Kirigami.Units.smallSpacing
        }

        Controls.ToolButton {
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
            Layout.rightMargin: Kirigami.Units.smallSpacing
            Layout.topMargin: Kirigami.Units.smallSpacing
            Accessible.name: text
            display: Controls.AbstractButton.IconOnly
            icon.name: "dialog-close-symbolic"
            text: KI18n.i18nc("@action:button", "Close")

            Controls.ToolTip.text: text
            Controls.ToolTip.visible: hovered && !Kirigami.Settings.hasTransientTouchInput

            onClicked: root.close()
        }
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        enabled: root.opened
        sequences: ["Return", "Enter"]

        onActivated: root.accept()
    }

    Shortcut {
        context: Qt.ApplicationShortcut
        enabled: root.opened
        sequence: "Esc"

        onActivated: root.close()
    }

    Controls.ScrollView {
        id: scrollView

        objectName: "shortcutHelpScrollView"

        Layout.fillWidth: true
        Layout.maximumHeight: Math.max(Kirigami.Units.gridUnit * 8, root.availableDialogHeight - Kirigami.Units.gridUnit * 6)
        Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff
        clip: true
        contentWidth: availableWidth

        ColumnLayout {
            id: shortcutList

            objectName: "shortcutHelpList"

            spacing: 0
            width: scrollView.availableWidth

            Repeater {
                model: root.application.shortcutHelpModel

                delegate: ColumnLayout {
                    id: shortcutRow

                    required property string actionText
                    required property bool categoryFirst
                    required property bool categoryLast
                    required property string categoryText
                    required property var shortcutKeyTexts
                    required property string shortcutText

                    Layout.fillWidth: true
                    spacing: 0

                    FormCard.FormHeader {
                        objectName: "shortcutCategoryHeader"

                        Layout.fillWidth: true
                        maximumWidth: root.formMaximumWidth
                        title: shortcutRow.categoryText
                        visible: shortcutRow.categoryFirst
                    }

                    ShortcutDelegate {
                        Layout.fillWidth: true
                        actionText: shortcutRow.actionText
                        shortcutKeyTexts: shortcutRow.shortcutKeyTexts
                        shortcutText: shortcutRow.shortcutText
                    }

                    FormCard.FormDelegateSeparator {
                        Layout.fillWidth: true
                        visible: !shortcutRow.categoryLast
                    }
                }
            }
        }
    }
}
