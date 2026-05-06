// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    required property var actionNames
    required property var application
    property int shortcutFilter: ConfiguredActionShortcut.AllShortcuts
    property bool shortcutsEnabled: true

    Repeater {
        model: root.actionNames

        ConfiguredActionShortcut {
            required property string modelData

            actionName: modelData
            application: root.application
            shortcutFilter: root.shortcutFilter
            shortcutsEnabled: root.shortcutsEnabled
        }
    }
}
