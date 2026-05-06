// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick

Item {
    id: root

    required property var sequences
    property bool shortcutsEnabled: true

    signal activated

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.shortcutsEnabled
        sequences: root.sequences

        onActivated: root.activated()
    }
}
