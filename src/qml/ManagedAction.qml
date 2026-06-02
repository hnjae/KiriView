// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQml
import QtQuick
import io.github.hnjae.kiriview

Item {
    id: root

    required property KiriViewApplication application
    required property int actionId
    property bool proxyEnabled: runtimePlacementEnabled
    property bool proxyCheckable: sourceAction !== null && sourceAction !== undefined && sourceAction.checkable
    property bool proxyChecked: sourceAction !== null && sourceAction !== undefined && sourceAction.checked
    property int displayHint: 0
    property string fixedShortcutText: ""
    property string menuText: ""
    property string toolbarText: ""
    property var toolbarTooltip
    readonly property int shortcutRevision: application.shortcutRevision
    readonly property int actionStateRevision: application.actionStateRevision
    readonly property bool runtimePlacementEnabled: {
        root.actionStateRevision;
        return root.application.actionPlacementEnabled(root.actionId);
    }
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

    Connections {
        target: root.sourceAction

        function onTriggered() {
            root.triggered();
        }
    }
}
