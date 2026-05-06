// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick

Item {
    id: root

    required property var action
    required property var handler

    Connections {
        target: root.action

        function onTriggered() {
            root.handler();
        }
    }
}
