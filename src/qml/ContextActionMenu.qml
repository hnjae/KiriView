// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick.Controls as Controls
import QtQml.Models
import org.hnjae.kiriview

Controls.Menu {
    id: root

    property var actions: []
    readonly property int actionCount: actions.length

    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    popupType: Controls.Popup.Window

    MenuAccessKeyRouter {
        enabled: root.visible || root.opened
        menu: root
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

        onObjectAdded: (index, object) => root.insertItem(index, object)
        onObjectRemoved: (index, object) => root.removeItem(object)
    }
}
