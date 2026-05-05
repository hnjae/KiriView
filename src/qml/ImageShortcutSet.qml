// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQml.Models
import QtQuick

Item {
    id: root

    required property var sequences

    signal activated

    Instantiator {
        model: root.sequences

        delegate: ImageShortcut {
            required property var modelData

            enabled: root.enabled
            sequence: modelData

            onActivated: root.activated()
        }
    }
}
