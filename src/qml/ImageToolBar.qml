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
    property bool videoMode: false
    property bool zoomPercentAvailable: imageReady
    property bool zoomPercentKnown: imageReady
    property real zoomPercent: imageDocument.zoomPercent
    property bool zoomEditable: !videoMode && imageReady
    property bool activeNavigationAvailable: false
    property int activeNavigationCount: 0
    property int activeNavigationCurrentNumber: 0
    property bool activeNavigationEditable: false
    property bool activeNavigationKnown: false
    property var openActiveNavigationAtNumber: function (number) {}
    property bool rightToLeftReadingActive: false
    property bool rightToLeftReadingControlVisible: false
    property bool twoPageModeControlVisible: false
    property bool pageNavigationInputFocused: false
    property bool zoomInputFocused: false
    property Item applicationMenuButtonAnchor: null
    // Popup.Window may close before the same button click reaches the toolbar.
    property double applicationMenuClosedTimestamp: 0
    readonly property int controlSpacing: compact ? Math.max(1, Math.round(Kirigami.Units.smallSpacing / 2)) : Kirigami.Units.smallSpacing
    readonly property int edgeMargin: controlSpacing
    property int selectedFitMode: KiriImageDocument.Fit
    readonly property bool fitSplitButtonTextVisible: width >= Kirigami.Units.gridUnit * 40
    readonly property bool interactionActive: toolbarHoverHandler.hovered || textInputFocused()
    readonly property int toolbarVerticalPadding: controlSpacing
    readonly property var imageToolbarControls: (root.rightToLeftReadingControlVisible ? [root.actions.rightToLeftReadingAction] : []).concat(root.twoPageModeControlVisible ? [root.actions.twoPageModeAction] : [], [fitMenuAction, zoomLevelAction])
    readonly property var toolbarControls: imageToolbarControls
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

    function fitModeIsSelectable(zoomMode) {
        return zoomMode === KiriImageDocument.Fit || zoomMode === KiriImageDocument.FitHeight || zoomMode === KiriImageDocument.FitWidth;
    }

    function fitModeAction(zoomMode) {
        if (zoomMode === KiriImageDocument.FitHeight) {
            return root.actions.fitHeightAction;
        }
        if (zoomMode === KiriImageDocument.FitWidth) {
            return root.actions.fitWidthAction;
        }
        return root.actions.fitAction;
    }

    function fitModeIconName(zoomMode) {
        if (zoomMode === KiriImageDocument.FitHeight) {
            return "zoom-fit-height";
        }
        if (zoomMode === KiriImageDocument.FitWidth) {
            return "zoom-fit-width";
        }
        return "zoom-fit-best-symbolic";
    }

    function fitModeText(zoomMode) {
        if (zoomMode === KiriImageDocument.FitHeight) {
            return KI18n.i18nc("@action:button", "Fit Height");
        }
        if (zoomMode === KiriImageDocument.FitWidth) {
            return KI18n.i18nc("@action:button", "Fit Width");
        }
        return KI18n.i18nc("@action:button", "Fit to Window");
    }

    function syncSelectedFitModeFromZoomMode() {
        if (imageDocument !== null && fitModeIsSelectable(imageDocument.zoomMode)) {
            selectedFitMode = imageDocument.zoomMode;
        }
    }

    function triggerFitMode(zoomMode) {
        if (!fitModeIsSelectable(zoomMode)) {
            return;
        }

        selectedFitMode = zoomMode;
        const action = fitModeAction(zoomMode);
        if (action !== null && action !== undefined && action.enabled) {
            action.trigger();
        }
    }

    leftPadding: edgeMargin
    rightPadding: edgeMargin
    topPadding: toolbarVerticalPadding
    bottomPadding: toolbarVerticalPadding

    Component.onCompleted: {
        syncSelectedFitModeFromZoomMode();
        if (floating) {
            background = floatingBackgroundComponent.createObject(root);
        }
    }

    Connections {
        target: root.imageDocument

        function onZoomModeChanged() {
            root.syncSelectedFitModeFromZoomMode();
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
            readOnlyDisplayMode: root.videoMode
            readOnlyPercent: Math.round(root.zoomPercent)
            readOnlyPercentKnown: root.zoomPercentKnown
            zoomEditable: root.zoomEditable
            zoomPercent: root.zoomPercent
            zoomPercentAvailable: root.zoomPercentAvailable
            zoomPercentKnown: root.zoomPercentKnown
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
        enabled: !root.videoMode && root.imageReady
        icon.name: "zoom-original-symbolic"
        text: KI18n.i18nc("@action", "Zoom")
        tooltip: root.videoMode ? (root.zoomPercentKnown ? KI18n.i18nc("@info:tooltip", "Fitted video zoom") : KI18n.i18nc("@info:tooltip", "Video zoom unavailable")) : text
    }

    readonly property Kirigami.Action fitMenuAction: Kirigami.Action {
        displayComponent: FitSplitButton {
            action: root.fitMenuAction
            fitAction: root.actions.fitAction
            fitHeightAction: root.actions.fitHeightAction
            fitHeightMode: KiriImageDocument.FitHeight
            fitMode: KiriImageDocument.Fit
            fitWidthAction: root.actions.fitWidthAction
            fitWidthMode: KiriImageDocument.FitWidth
            selectedFitMode: root.selectedFitMode
            textVisible: root.fitSplitButtonTextVisible

            onFitModeTriggered: function (zoomMode) {
                root.triggerFitMode(zoomMode);
            }
        }
        displayHint: Kirigami.DisplayHint.KeepVisible
        enabled: root.fitModeAction(root.selectedFitMode).enabled
        icon.name: root.fitModeIconName(root.selectedFitMode)
        text: root.fitModeText(root.selectedFitMode)
        tooltip: text

        onTriggered: root.triggerFitMode(root.selectedFitMode)
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

        ImageDocumentPageNavigation {
            id: pageNavigation

            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter

            actions: root.actions
            activeNavigationAvailable: root.activeNavigationAvailable
            activeNavigationCount: root.activeNavigationCount
            activeNavigationCurrentNumber: root.activeNavigationCurrentNumber
            activeNavigationEditable: root.activeNavigationEditable
            activeNavigationKnown: root.activeNavigationKnown
            compact: root.compact
            openActiveNavigationAtNumber: root.openActiveNavigationAtNumber
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
            display: Controls.AbstractButton.TextBesideIcon
        }
    }
}
