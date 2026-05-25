// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    required property var actionIds
    required property int shortcutFilter
    required property var application
    property var interceptShortcut: function (actionId) {
        return false;
    }
    property bool shortcutsEnabled: true

    signal shortcutIntercepted(int actionId)

    Repeater {
        model: root.actionIds

        ConfiguredActionShortcut {
            required property int modelData

            actionId: modelData
            application: root.application
            interceptShortcut: root.interceptShortcut(modelData)
            shortcutFilter: root.shortcutFilter
            shortcutsEnabled: root.shortcutsEnabled

            onShortcutIntercepted: function (actionId) {
                root.shortcutIntercepted(actionId);
            }
        }
    }
}
