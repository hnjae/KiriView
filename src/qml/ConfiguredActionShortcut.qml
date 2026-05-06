// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick

Item {
    id: root

    enum ShortcutFilter {
        AllShortcuts,
        WithCommandModifier,
        WithoutCommandModifier
    }

    required property var application
    required property string actionName
    property int shortcutFilter: ConfiguredActionShortcut.AllShortcuts
    property bool shortcutsEnabled: true
    readonly property int shortcutRevision: application.shortcutRevision
    readonly property var action: application.action(actionName)

    function actionShortcuts() {
        root.shortcutRevision;
        switch (root.shortcutFilter) {
        case ConfiguredActionShortcut.WithCommandModifier:
            return root.application.shortcutsWithCommandModifier(root.actionName);
        case ConfiguredActionShortcut.WithoutCommandModifier:
            return root.application.shortcutsWithoutCommandModifier(root.actionName);
        case ConfiguredActionShortcut.AllShortcuts:
        default:
            return root.application.shortcuts(root.actionName);
        }
    }

    function actionEnabled() {
        return root.action !== null && root.action !== undefined && root.action.enabled;
    }

    function triggerAction() {
        if (actionEnabled()) {
            root.action.trigger();
        }
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.shortcutsEnabled && root.actionEnabled()
        sequences: root.actionShortcuts()

        onActivated: root.triggerAction()
    }
}
