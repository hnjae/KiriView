// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQml.Models
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

Controls.ToolBar {
    id: root

    property var actions: []
    property bool showApplicationMenuActions: false
    property double applicationMenuClosedTimestamp: 0

    function applicationMenuOpen() {
        return applicationMenuPopup.visible || applicationMenuPopup.opened;
    }

    function popupApplicationMenu() {
        if (!showApplicationMenuActions || actions.length <= 0) {
            return false;
        }

        applicationMenuPopup.popup(applicationMenuButton, Math.max(0, applicationMenuButton.width - applicationMenuPopup.implicitWidth), applicationMenuButton.height);
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

    function textInputFocused() {
        return false;
    }

    function cancelTextInputEditing(returnViewerFocus) {
        return false;
    }

    function commitTextInputEditing(returnViewerFocus) {
        return false;
    }

    leftPadding: Kirigami.Units.smallSpacing
    rightPadding: Kirigami.Units.smallSpacing
    topPadding: Kirigami.Units.smallSpacing
    bottomPadding: Kirigami.Units.smallSpacing

    contentItem: RowLayout {
        Item {
            Layout.fillWidth: true
        }

        Controls.ToolButton {
            id: applicationMenuButton

            objectName: "applicationMenuHostButton"

            Accessible.name: text
            Accessible.role: Accessible.ButtonMenu
            display: Controls.AbstractButton.IconOnly
            enabled: root.showApplicationMenuActions && root.actions.length > 0
            icon.name: "open-menu-symbolic"
            text: KI18n.i18nc("@action", "Application Menu")

            Controls.ToolTip.text: KI18n.i18nc("@info:tooltip", "Open menu") + " (F10)"
            Controls.ToolTip.visible: hovered && Controls.ToolTip.text.length > 0 && !applicationMenuPopup.visible && !pressed && !Kirigami.Settings.hasTransientTouchInput

            onClicked: {
                if (Date.now() - root.applicationMenuClosedTimestamp < 250) {
                    return;
                }

                root.toggleApplicationMenu();
            }
            onPressed: {
                if (root.applicationMenuOpen()) {
                    applicationMenuPopup.dismiss();
                }
            }
        }
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
            model: root.actions

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
}
