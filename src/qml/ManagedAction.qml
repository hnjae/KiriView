// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQml
import QtQuick
import io.github.hnjae.kiriview

Item {
    id: root

    required property KiriViewApplication application
    required property int actionId
    property int displayHint: 0
    property string fixedShortcutText: ""
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
    readonly property string menuDisplayShortcutText: {
        return root.fixedShortcutText.length > 0 ? root.fixedShortcutText : root.menuShortcutText;
    }
    readonly property string toolbarDisplayText: {
        root.actionStateRevision;
        return root.application.actionToolbarTextForId(root.actionId);
    }
    readonly property var toolbarDisplayTooltip: {
        root.actionStateRevision;
        const tooltip = root.application.actionToolbarTooltipTextForId(root.actionId);
        if (tooltip.length > 0) {
            return tooltip;
        }
        return undefined;
    }
    readonly property string menuDisplayText: {
        root.actionStateRevision;
        const text = root.application.actionMenuTextForId(root.actionId);
        return text.length > 0 ? text : root.sourceAction?.text ?? "";
    }
    readonly property var sourceAction: application.actionForId(actionId)
    readonly property ActionProxy proxy: ActionProxy {
        displayHint: root.displayHint
        displayShortcutText: root.fixedShortcutText
        placementVisible: root.runtimePlacementEnabled
        sourceAction: root.sourceAction
        textOverride: root.toolbarDisplayText
        tooltipOverride: root.toolbarDisplayTooltip
    }
    readonly property ActionProxy menuProxy: ActionProxy {
        displayHint: root.displayHint
        menuShortcutText: root.menuDisplayShortcutText
        placementVisible: root.runtimePlacementEnabled
        sourceAction: root.sourceAction
        textOverride: root.menuDisplayText
        tooltipOverride: ""
    }
}
