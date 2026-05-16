// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

Controls.ToolBar {
    id: root

    required property KiriImageDocument imageDocument
    required property bool imageReady
    required property int minimumManualZoomPercent
    required property int maximumManualZoomPercent
    required property real zoomStepFactor
    required property var actions
    property bool compact: false
    property bool floating: false
    property bool transientOverlay: false
    property var applicationMenuActions: []
    property bool showApplicationMenuActions: false
    property bool pageNavigationInputFocused: false
    property bool zoomInputFocused: false
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property int edgeMargin: controlSpacing
    readonly property bool interactionActive: toolbarHoverHandler.hovered || textInputFocused()
    readonly property int toolbarVerticalPadding: controlSpacing
    readonly property var toolbarControls: [root.actions.twoPageModeAction, zoomLevelAction, fitMenuAction]
    readonly property var toolbarActions: showApplicationMenuActions ? toolbarControls.concat([applicationMenuAction]) : toolbarControls

    signal pageNumberResetRequested
    signal textInputCancelRequested(bool returnViewerFocus)
    signal textInputCommitRequested(bool returnViewerFocus)
    signal textInputFocusReturnRequested

    function cancelTextInputEditing(returnViewerFocus) {
        if (!textInputFocused()) {
            return false;
        }

        textInputCancelRequested(returnViewerFocus === undefined ? true : returnViewerFocus);
        return true;
    }

    function commitTextInputEditing(returnViewerFocus) {
        if (!textInputFocused()) {
            return false;
        }

        textInputCommitRequested(returnViewerFocus === undefined ? true : returnViewerFocus);
        return true;
    }

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

    readonly property Kirigami.Action zoomLevelAction: Kirigami.Action {
        displayComponent: ImageZoomControls {
            id: zoomControls

            compact: root.compact
            imageDocument: root.imageDocument
            imageReady: root.imageReady
            maximumManualZoomPercent: root.maximumManualZoomPercent
            minimumManualZoomPercent: root.minimumManualZoomPercent
            zoomStepFactor: root.zoomStepFactor

            Component.onDestruction: {
                if (textInputActive) {
                    root.zoomInputFocused = false;
                }
            }
            onTextInputActiveChanged: root.zoomInputFocused = textInputActive

            onEditingCompleted: function (returnViewerFocus) {
                if (returnViewerFocus) {
                    root.textInputFocusReturnRequested();
                }
            }

            Connections {
                target: root

                function onTextInputCancelRequested(returnViewerFocus) {
                    zoomControls.cancelEditing(returnViewerFocus);
                }

                function onTextInputCommitRequested(returnViewerFocus) {
                    zoomControls.commitEditing(returnViewerFocus);
                }
            }
        }
        displayHint: Kirigami.DisplayHint.KeepVisible
        icon.name: "zoom-original-symbolic"
        text: KI18n.i18nc("@action", "Zoom")
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

    readonly property Kirigami.Action applicationMenuAction: Kirigami.Action {
        children: root.applicationMenuActions
        displayHint: Kirigami.DisplayHint.KeepVisible
        enabled: root.applicationMenuActions.length > 0
        icon.name: "open-menu-symbolic"
        text: KI18n.i18nc("@action", "Application Menu")
        tooltip: KI18n.i18nc("@info:tooltip", "Open menu")
    }

    contentItem: RowLayout {
        spacing: root.controlSpacing

        ImagePageNavigation {
            id: pageNavigation

            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

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

            onEditingCompleted: function (returnViewerFocus) {
                if (returnViewerFocus) {
                    root.textInputFocusReturnRequested();
                }
            }

            Connections {
                target: root

                function onPageNumberResetRequested() {
                    pageNavigation.resetPageNumberText();
                }

                function onTextInputCancelRequested(returnViewerFocus) {
                    pageNavigation.cancelEditing(returnViewerFocus);
                }

                function onTextInputCommitRequested(returnViewerFocus) {
                    pageNavigation.commitEditing(returnViewerFocus);
                }
            }
        }

        Kirigami.ActionToolBar {
            id: actionToolBar

            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            Layout.fillWidth: true
            Layout.minimumWidth: Kirigami.Units.gridUnit * 2

            actions: root.toolbarActions
            alignment: Qt.AlignRight
            display: Controls.AbstractButton.IconOnly
        }
    }
}
