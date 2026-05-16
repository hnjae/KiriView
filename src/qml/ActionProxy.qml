// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

Kirigami.Action {
    id: root

    required property var sourceAction
    property bool checkableOverride: sourceAction ? sourceAction.checkable : false
    property bool checkedOverride: sourceAction ? sourceAction.checked : false
    property bool enabledOverride: sourceAction && sourceAction.enabled
    property string menuShortcutText: ""
    property var shortcutOverride: ""
    property string textOverride: ""
    property var tooltipOverride

    checkable: checkableOverride
    checked: checkable && checkedOverride
    enabled: sourceAction && enabledOverride
    fromQAction: sourceAction
    shortcut: shortcutOverride
    text: textOverride.length > 0 ? textOverride : sourceAction?.text ?? ""
    tooltip: tooltipOverride === undefined ? text : tooltipOverride

    alternateShortcut.enabled: false
}
