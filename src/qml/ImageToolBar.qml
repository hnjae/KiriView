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
    property bool compact: false
    property bool floating: false
    property bool showApplicationMenuButton: false
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property int edgeMargin: controlSpacing
    readonly property bool interactionActive: toolbarHoverHandler.hovered || textInputFocused() || zoomControls.menuOpen
    readonly property int toolbarVerticalPadding: controlSpacing

    signal applicationMenuRequested

    function resetPageNumberText() {
        pageNavigation.resetPageNumberText();
    }

    function textInputFocused() {
        return pageNavigation.textInputFocused() || zoomControls.textInputFocused();
    }

    Component.onCompleted: {
        if (floating) {
            background = floatingBackgroundComponent.createObject(root);
        }
    }

    HoverHandler {
        id: toolbarHoverHandler

        enabled: root.floating
    }

    Component {
        id: floatingBackgroundComponent

        Kirigami.ShadowedRectangle {
            color: Kirigami.Theme.backgroundColor
            opacity: 0.84
            radius: Kirigami.Units.cornerRadius

            border.color: Kirigami.ColorUtils.linearInterpolation(Kirigami.Theme.backgroundColor, Kirigami.Theme.textColor, Kirigami.Theme.frameContrast)
            border.width: 1

            shadow.color: Qt.rgba(0, 0, 0, 0.35)
            shadow.size: Kirigami.Units.smallSpacing
            shadow.xOffset: 0
            shadow.yOffset: Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2))
        }
    }

    contentItem: Item {
        implicitHeight: Math.max(leftActions.implicitHeight, pageNavigation.implicitHeight, zoomControls.implicitHeight) + root.toolbarVerticalPadding * 2

        RowLayout {
            id: leftActions

            anchors.left: parent.left
            anchors.leftMargin: root.edgeMargin
            anchors.verticalCenter: parent.verticalCenter
            spacing: root.controlSpacing

            Controls.ToolButton {
                display: Controls.AbstractButton.IconOnly
                icon.name: "application-menu-symbolic"
                text: "Menu"
                visible: root.showApplicationMenuButton

                onClicked: root.applicationMenuRequested()

                Controls.ToolTip.text: text
                Controls.ToolTip.visible: hovered
            }

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
            compact: root.compact
            imageDocument: root.imageDocument
            imageReady: root.imageReady
        }

        ImageZoomControls {
            id: zoomControls

            anchors.right: parent.right
            anchors.rightMargin: root.edgeMargin
            anchors.verticalCenter: parent.verticalCenter

            actions: root.actions
            compact: root.compact
            imageDocument: root.imageDocument
            imageReady: root.imageReady
            maximumManualZoomPercent: root.maximumManualZoomPercent
            minimumManualZoomPercent: root.minimumManualZoomPercent
            zoomStepPercent: root.zoomStepPercent
        }
    }
}
