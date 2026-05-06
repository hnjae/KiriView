// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import org.kde.kirigami.private as KirigamiPrivate

Item {
    id: root

    required property var action
    required property int shortcutRevision
    property bool shortcutsEnabled: true

    function triggerAction() {
        if (root.action && root.action.enabled) {
            root.action.trigger();
        }
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.shortcutsEnabled && root.action && root.action.enabled
        sequence: {
            root.shortcutRevision;
            return root.action ? root.action.shortcut : "";
        }

        onActivated: root.triggerAction()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.shortcutsEnabled && root.action && root.action.enabled
        sequences: {
            root.shortcutRevision;
            return root.action ? KirigamiPrivate.ActionHelper.alternateShortcuts(root.action) : [];
        }

        onActivated: root.triggerAction()
    }
}
