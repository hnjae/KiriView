// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.ki18n
import org.kde.kirigami as Kirigami

Controls.Control {
    id: root

    objectName: "fitSplitButton"

    required property var action
    required property var fitAction
    required property var fitHeightAction
    required property var fitWidthAction
    required property int fitMode
    required property int fitHeightMode
    required property int fitWidthMode
    required property int selectedFitMode
    property bool textVisible: true
    property int buttonSpacing: Kirigami.Units.smallSpacing
    readonly property real buttonHeight: Math.max(primaryButton.implicitHeight, menuButton.implicitHeight)

    property alias display: primaryButton.display
    property alias text: primaryButton.text
    property string iconName: primaryButton.icon.name
    property string tooltip: primaryButton.Controls.ToolTip.text

    signal fitModeTriggered(int zoomMode)

    function triggerFitMode(zoomMode) {
        root.fitModeTriggered(zoomMode);
        fitMenu.dismiss();
    }

    enabled: action?.enabled ?? false
    implicitHeight: buttonRow.implicitHeight
    implicitWidth: buttonRow.implicitWidth
    Layout.alignment: Qt.AlignVCenter
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    topPadding: 0

    contentItem: RowLayout {
        id: buttonRow

        spacing: root.buttonSpacing

        Controls.ToolButton {
            id: primaryButton

            objectName: "fitPrimaryButton"

            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: root.buttonHeight

            Accessible.name: root.action?.text ?? text
            Accessible.role: Accessible.Button
            Accessible.ignored: !visible

            action: root.action
            display: root.textVisible ? Controls.AbstractButton.TextBesideIcon : Controls.AbstractButton.IconOnly
            enabled: root.enabled
            flat: true
            icon.name: root.action?.icon.name ?? ""
            text: root.action?.text ?? ""

            Controls.ToolTip.text: root.action?.tooltip ?? text
            Controls.ToolTip.visible: hovered && Controls.ToolTip.text.length > 0 && !pressed && !Kirigami.Settings.hasTransientTouchInput
        }

        Controls.ToolButton {
            id: menuButton

            objectName: "fitMenuButton"

            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: root.buttonHeight

            readonly property string menuTooltip: KI18n.i18nc("@info:tooltip", "Choose fit mode")

            Accessible.name: menuTooltip
            Accessible.role: Accessible.ButtonMenu
            Accessible.ignored: !visible

            display: Controls.AbstractButton.IconOnly
            enabled: root.fitAction?.enabled || root.fitWidthAction?.enabled || root.fitHeightAction?.enabled
            flat: true
            icon.name: "go-down"
            text: KI18n.i18nc("@action", "Fit Mode")

            Controls.ToolTip.text: menuTooltip
            Controls.ToolTip.visible: hovered && Controls.ToolTip.text.length > 0 && !fitMenu.visible && !pressed && !Kirigami.Settings.hasTransientTouchInput

            onClicked: fitMenu.open()
        }
    }

    Controls.Menu {
        id: fitMenu

        closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutsideParent
        popupType: Controls.Popup.Item
        y: root.height

        Controls.MenuItem {
            objectName: "fitToWindowMenuItem"

            checkable: true
            checked: root.selectedFitMode === root.fitMode
            enabled: root.fitAction?.enabled ?? false
            icon.name: root.fitAction?.icon.name ?? "zoom-fit-best-symbolic"
            text: KI18n.i18nc("@action:inmenu", "Fit to Window")

            onTriggered: root.triggerFitMode(root.fitMode)
        }

        Controls.MenuItem {
            objectName: "fitWidthMenuItem"

            checkable: true
            checked: root.selectedFitMode === root.fitWidthMode
            enabled: root.fitWidthAction?.enabled ?? false
            icon.name: "zoom-fit-width"
            text: KI18n.i18nc("@action:inmenu", "Fit Width")

            onTriggered: root.triggerFitMode(root.fitWidthMode)
        }

        Controls.MenuItem {
            objectName: "fitHeightMenuItem"

            checkable: true
            checked: root.selectedFitMode === root.fitHeightMode
            enabled: root.fitHeightAction?.enabled ?? false
            icon.name: "zoom-fit-height"
            text: KI18n.i18nc("@action:inmenu", "Fit Height")

            onTriggered: root.triggerFitMode(root.fitHeightMode)
        }
    }
}
