// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick

Item {
    id: root

    enum ShortcutFilter {
        AllShortcuts,
        WithCommandModifier,
        WithoutCommandModifier,
        ShortcutAliases
    }

    required property var application
    required property int actionId
    property bool interceptShortcut: false
    property int shortcutFilter: ConfiguredActionShortcut.AllShortcuts
    property bool shortcutsEnabled: true
    readonly property int shortcutRevision: application.shortcutRevision
    readonly property var action: application.actionForId(actionId)

    signal shortcutIntercepted(int actionId)

    function actionShortcuts() {
        root.shortcutRevision;
        switch (root.shortcutFilter) {
        case ConfiguredActionShortcut.WithCommandModifier:
            return root.application.shortcutsWithCommandModifierForId(root.actionId);
        case ConfiguredActionShortcut.WithoutCommandModifier:
            return root.application.shortcutsWithoutCommandModifierForId(root.actionId);
        case ConfiguredActionShortcut.ShortcutAliases:
            return root.application.shortcutAliasesForId(root.actionId);
        case ConfiguredActionShortcut.AllShortcuts:
        default:
            return root.application.shortcutsForId(root.actionId);
        }
    }

    function actionEnabled() {
        return root.action !== null && root.action !== undefined && root.action.enabled;
    }

    function triggerAction() {
        if (root.interceptShortcut) {
            root.shortcutIntercepted(root.actionId);
            return;
        }

        if (actionEnabled()) {
            root.action.trigger();
        }
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.shortcutsEnabled && (root.actionEnabled() || root.interceptShortcut)
        sequences: root.actionShortcuts()

        onActivated: root.triggerAction()
    }
}
