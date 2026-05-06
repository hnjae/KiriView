// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
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
    property bool transientOverlay: false
    property bool showApplicationMenuActions: false
    property bool pageNavigationInputFocused: false
    property bool zoomInputFocused: false
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property int edgeMargin: controlSpacing
    readonly property bool interactionActive: toolbarHoverHandler.hovered || textInputFocused()
    readonly property int toolbarVerticalPadding: controlSpacing
    readonly property var toolbarControls: [pageNavigationAction, zoomLevelAction, fitMenuAction]
    readonly property var toolbarActions: showApplicationMenuActions ? toolbarControls.concat(actions.applicationMenuActions) : toolbarControls

    signal pageNumberResetRequested

    function resetPageNumberText() {
        pageNumberResetRequested();
    }

    function textInputFocused() {
        return pageNavigationInputFocused || zoomInputFocused;
    }

    leftPadding: edgeMargin
    rightPadding: edgeMargin
    topPadding: toolbarVerticalPadding
    bottomPadding: toolbarVerticalPadding

    Component.onCompleted: {
        if (floating) {
            background = floatingBackgroundComponent.createObject(root);
        }
    }

    HoverHandler {
        id: toolbarHoverHandler

        enabled: root.floating || root.transientOverlay
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

    readonly property Kirigami.Action pageNavigationAction: Kirigami.Action {
        displayComponent: ImagePageNavigation {
            id: pageNavigation

            actions: root.actions
            compact: root.compact
            imageDocument: root.imageDocument
            imageReady: root.imageReady

            Component.onDestruction: {
                if (textInputActive) {
                    root.pageNavigationInputFocused = false;
                }
            }
            onTextInputActiveChanged: root.pageNavigationInputFocused = textInputActive

            Connections {
                target: root

                function onPageNumberResetRequested() {
                    pageNavigation.resetPageNumberText();
                }
            }
        }
        icon.name: "view-list-symbolic"
        text: "Page"
        tooltip: text
    }

    readonly property Kirigami.Action zoomLevelAction: Kirigami.Action {
        displayComponent: ImageZoomControls {
            id: zoomControls

            compact: root.compact
            imageDocument: root.imageDocument
            imageReady: root.imageReady
            maximumManualZoomPercent: root.maximumManualZoomPercent
            minimumManualZoomPercent: root.minimumManualZoomPercent
            zoomStepPercent: root.zoomStepPercent

            Component.onDestruction: {
                if (textInputActive) {
                    root.zoomInputFocused = false;
                }
            }
            onTextInputActiveChanged: root.zoomInputFocused = textInputActive
        }
        displayHint: Kirigami.DisplayHint.KeepVisible
        icon.name: "zoom-original-symbolic"
        text: "Zoom"
        tooltip: text
    }

    readonly property Kirigami.Action fitMenuAction: Kirigami.Action {
        children: [root.actions.fitAction, root.actions.fitHeightAction, root.actions.fitWidthAction]
        displayHint: Kirigami.DisplayHint.KeepVisible
        enabled: root.actions.fitAction.enabled
        icon.name: "zoom-fit-best-symbolic"
        text: root.actions.fitAction.text
        tooltip: text
    }

    contentItem: Kirigami.ActionToolBar {
        id: actionToolBar

        actions: root.toolbarActions
        alignment: Qt.AlignRight
        display: Controls.AbstractButton.IconOnly
        overflowIconName: "application-menu-symbolic"
    }
}
