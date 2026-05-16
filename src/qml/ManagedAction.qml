// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQml
import QtQuick
import io.github.hnjae.kiriview

Item {
    id: root

    required property KiriViewApplication application
    required property int actionId
    property bool bindEnabled: false
    property bool bindChecked: false
    property bool actionEnabled: sourceAction !== null && sourceAction !== undefined && sourceAction.enabled
    property bool actionChecked: sourceAction !== null && sourceAction !== undefined && sourceAction.checked
    property bool proxyEnabled: sourceAction !== null && sourceAction !== undefined && sourceAction.enabled
    property bool proxyCheckable: sourceAction !== null && sourceAction !== undefined && sourceAction.checkable
    property bool proxyChecked: sourceAction !== null && sourceAction !== undefined && sourceAction.checked
    property int displayHint: 0
    property string menuText: ""
    readonly property int shortcutRevision: application.shortcutRevision
    readonly property var menuShortcut: {
        root.shortcutRevision;
        return root.application.menuShortcutForId(root.actionId);
    }
    readonly property string menuShortcutText: {
        root.shortcutRevision;
        return root.application.menuShortcutTextForId(root.actionId);
    }
    readonly property string menuDisplayText: {
        return root.menuText.length > 0 ? root.menuText : root.sourceAction?.text ?? "";
    }
    readonly property var sourceAction: application.actionForId(actionId)
    readonly property ActionProxy proxy: ActionProxy {
        checkableOverride: root.proxyCheckable
        checkedOverride: root.proxyChecked
        displayHint: root.displayHint
        enabledOverride: root.proxyEnabled
        sourceAction: root.sourceAction
    }
    readonly property ActionProxy menuProxy: ActionProxy {
        checkableOverride: root.proxyCheckable
        checkedOverride: root.proxyChecked
        displayHint: root.displayHint
        enabledOverride: root.proxyEnabled
        menuShortcutText: root.menuShortcutText
        shortcutOverride: root.menuShortcut
        sourceAction: root.sourceAction
        textOverride: root.menuDisplayText
        tooltipOverride: ""
    }

    signal triggered

    Binding {
        property: "enabled"
        target: root.sourceAction
        value: root.actionEnabled
        when: root.bindEnabled && root.sourceAction !== null && root.sourceAction !== undefined
    }

    Binding {
        property: "checked"
        target: root.sourceAction
        value: root.actionChecked
        when: root.bindChecked && root.sourceAction !== null && root.sourceAction !== undefined
    }

    Connections {
        target: root.sourceAction

        function onTriggered() {
            root.triggered();
        }
    }
}
