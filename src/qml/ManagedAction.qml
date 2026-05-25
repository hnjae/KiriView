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
    property string fixedShortcutText: ""
    property string menuText: ""
    property string toolbarText: ""
    property var toolbarTooltip
    readonly property int shortcutRevision: application.shortcutRevision
    readonly property string menuShortcutText: {
        root.shortcutRevision;
        return root.application.menuShortcutTextForId(root.actionId);
    }
    readonly property string menuDisplayText: {
        return root.menuText.length > 0 ? root.menuText : root.sourceAction?.text ?? "";
    }
    readonly property string menuDisplayShortcutText: {
        return root.fixedShortcutText.length > 0 ? root.fixedShortcutText : root.menuShortcutText;
    }
    readonly property var toolbarDisplayTooltip: {
        if (root.toolbarTooltip !== undefined) {
            return root.toolbarTooltip;
        }
        if (root.toolbarText.length > 0) {
            return root.sourceAction?.text ?? "";
        }
        return undefined;
    }
    readonly property var sourceAction: application.actionForId(actionId)
    readonly property ActionProxy proxy: ActionProxy {
        checkableOverride: root.proxyCheckable
        checkedOverride: root.proxyChecked
        displayHint: root.displayHint
        displayShortcutText: root.fixedShortcutText
        enabledOverride: root.proxyEnabled
        sourceAction: root.sourceAction
        textOverride: root.toolbarText
        tooltipOverride: root.toolbarDisplayTooltip
    }
    readonly property ActionProxy menuProxy: ActionProxy {
        checkableOverride: root.proxyCheckable
        checkedOverride: root.proxyChecked
        displayHint: root.displayHint
        enabledOverride: root.proxyEnabled
        menuShortcutText: root.menuDisplayShortcutText
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
