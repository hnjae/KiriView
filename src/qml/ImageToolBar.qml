// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models
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
    property bool rightToLeftReadingActive: false
    property bool pageNavigationInputFocused: false
    property bool zoomInputFocused: false
    property Item applicationMenuButtonAnchor: null
    // Popup.Window may close before the same button click reaches the toolbar.
    property double applicationMenuClosedTimestamp: 0
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property int edgeMargin: controlSpacing
    readonly property bool interactionActive: toolbarHoverHandler.hovered || textInputFocused()
    readonly property int toolbarVerticalPadding: controlSpacing
    readonly property var toolbarControls: [root.actions.rightToLeftReadingAction, root.actions.twoPageModeAction, zoomLevelAction, fitMenuAction]
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

    function applicationMenuOpen() {
        return applicationMenuPopup.visible || applicationMenuPopup.opened;
    }

    function applicationMenuButtonUsable(button) {
        if (!button || !button.visible || button.width <= 0 || button.height <= 0 || !actionToolBar.visible) {
            return false;
        }

        try {
            const center = button.mapToItem(actionToolBar, button.width / 2, button.height / 2);
            return center.x >= 0 && center.x <= actionToolBar.width && center.y >= 0 && center.y <= actionToolBar.height;
        } catch (error) {
            return false;
        }
    }

    function updateApplicationMenuButtonAnchor(button) {
        if (applicationMenuButtonUsable(button)) {
            if (applicationMenuButtonAnchor === button || !applicationMenuButtonUsable(applicationMenuButtonAnchor)) {
                applicationMenuButtonAnchor = button;
            }
        } else if (applicationMenuButtonAnchor === button) {
            applicationMenuButtonAnchor = null;
        }
    }

    function popupApplicationMenu() {
        if (!showApplicationMenuActions || applicationMenuActions.length <= 0) {
            return false;
        }

        const menuButton = applicationMenuButtonAnchor;
        if (applicationMenuButtonUsable(menuButton)) {
            const popupPosition = menuButton.mapToItem(root, 0, menuButton.height);
            applicationMenuPopupAnchor.x = popupPosition.x;
            applicationMenuPopupAnchor.y = popupPosition.y;
            applicationMenuPopup.popup(applicationMenuPopupAnchor, 0, 0);
            return true;
        }

        applicationMenuPopup.popup(actionToolBar, Math.max(0, actionToolBar.width - applicationMenuPopup.implicitWidth), actionToolBar.height);
        return true;
    }

    function openApplicationMenu() {
        if (applicationMenuOpen()) {
            return true;
        }

        return popupApplicationMenu();
    }

    function toggleApplicationMenu() {
        if (applicationMenuOpen()) {
            applicationMenuPopup.dismiss();
            return true;
        }

        return popupApplicationMenu();
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
        displayComponent: Controls.ToolButton {
            id: applicationMenuButton

            objectName: "toolbarApplicationMenuButton"

            Accessible.name: root.applicationMenuAction.text
            Accessible.role: Accessible.ButtonMenu
            Accessible.ignored: !visible

            property bool skipNextClick: false

            display: Controls.AbstractButton.IconOnly
            enabled: root.applicationMenuAction.enabled
            icon.name: root.applicationMenuAction.icon.name
            text: root.applicationMenuAction.text

            Controls.ToolTip.text: root.applicationMenuAction.tooltip
            Controls.ToolTip.visible: hovered && Controls.ToolTip.text.length > 0 && !applicationMenuPopup.visible && !pressed && !Kirigami.Settings.hasTransientTouchInput

            Component.onCompleted: root.updateApplicationMenuButtonAnchor(applicationMenuButton)
            Component.onDestruction: {
                if (root.applicationMenuButtonAnchor === applicationMenuButton) {
                    root.applicationMenuButtonAnchor = null;
                }
            }
            onHeightChanged: root.updateApplicationMenuButtonAnchor(applicationMenuButton)
            onParentChanged: root.updateApplicationMenuButtonAnchor(applicationMenuButton)
            onVisibleChanged: root.updateApplicationMenuButtonAnchor(applicationMenuButton)
            onWidthChanged: root.updateApplicationMenuButtonAnchor(applicationMenuButton)
            onXChanged: root.updateApplicationMenuButtonAnchor(applicationMenuButton)
            onYChanged: root.updateApplicationMenuButtonAnchor(applicationMenuButton)

            onClicked: {
                if (skipNextClick) {
                    skipNextClick = false;
                    return;
                }
                if (Date.now() - root.applicationMenuClosedTimestamp < 250) {
                    return;
                }

                root.toggleApplicationMenu();
            }
            onPressed: {
                if (root.applicationMenuOpen()) {
                    skipNextClick = true;
                    applicationMenuPopup.dismiss();
                }
            }
        }
        displayHint: Kirigami.DisplayHint.KeepVisible
        enabled: root.applicationMenuActions.length > 0
        icon.name: "open-menu-symbolic"
        text: KI18n.i18nc("@action", "Application Menu")
        tooltip: KI18n.i18nc("@info:tooltip", "Open menu") + " (F10)"

        onTriggered: root.toggleApplicationMenu()
    }

    Item {
        id: applicationMenuPopupAnchor

        enabled: false
        height: 1
        width: 1
    }

    Controls.Menu {
        id: applicationMenuPopup

        closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutsideParent
        popupType: Controls.Popup.Window

        onAboutToHide: root.applicationMenuClosedTimestamp = Date.now()

        MenuAccessKeyRouter {
            enabled: root.showApplicationMenuActions
            menu: applicationMenuPopup
        }

        Instantiator {
            model: root.applicationMenuActions

            delegate: DelegateChooser {
                role: "separator"

                DelegateChoice {
                    roleValue: true

                    Controls.MenuSeparator {}
                }

                DelegateChoice {
                    roleValue: false

                    MenuActionItem {
                        required property var modelData

                        action: modelData
                    }
                }
            }

            onObjectAdded: (index, object) => applicationMenuPopup.insertItem(index, object)
            onObjectRemoved: (index, object) => applicationMenuPopup.removeItem(object)
        }
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
            rightToLeftReadingActive: root.rightToLeftReadingActive

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
