// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.kirigami as Kirigami

Controls.ToolBar {
    id: root

    required property KiriImageDocument imageDocument
    required property bool imageReady
    required property int minimumManualZoomPercent
    required property int maximumManualZoomPercent
    required property int zoomStepPercent
    required property var actions

    function resetPageNumberText() {
        pageNavigation.resetPageNumberText();
    }

    function textInputFocused() {
        return pageNavigation.textInputFocused() || zoomControls.textInputFocused();
    }

    contentItem: Item {
        implicitHeight: Math.max(leftActions.implicitHeight, pageNavigation.implicitHeight, zoomControls.implicitHeight) + Kirigami.Units.smallSpacing * 2

        RowLayout {
            id: leftActions

            anchors.left: parent.left
            anchors.leftMargin: Kirigami.Units.smallSpacing
            anchors.verticalCenter: parent.verticalCenter
            spacing: Kirigami.Units.smallSpacing

            Controls.ToolButton {
                action: root.actions.openAction
                display: Controls.AbstractButton.IconOnly

                Controls.ToolTip.text: action.text
                Controls.ToolTip.visible: hovered
            }

            Controls.ToolButton {
                action: root.actions.previousContainerAction
                display: Controls.AbstractButton.IconOnly

                Controls.ToolTip.text: action.text
                Controls.ToolTip.visible: hovered
            }

            Controls.ToolButton {
                action: root.actions.nextContainerAction
                display: Controls.AbstractButton.IconOnly

                Controls.ToolTip.text: action.text
                Controls.ToolTip.visible: hovered
            }
        }

        ImagePageNavigation {
            id: pageNavigation

            anchors.centerIn: parent

            actions: root.actions
            imageDocument: root.imageDocument
            imageReady: root.imageReady
        }

        ImageZoomControls {
            id: zoomControls

            anchors.right: parent.right
            anchors.rightMargin: Kirigami.Units.smallSpacing
            anchors.verticalCenter: parent.verticalCenter

            actions: root.actions
            imageDocument: root.imageDocument
            imageReady: root.imageReady
            maximumManualZoomPercent: root.maximumManualZoomPercent
            minimumManualZoomPercent: root.minimumManualZoomPercent
            zoomStepPercent: root.zoomStepPercent
        }
    }
}
