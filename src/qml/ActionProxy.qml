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

    checkable: checkableOverride
    checked: checkable && checkedOverride
    enabled: sourceAction && enabledOverride
    fromQAction: sourceAction
    shortcut: ""
    tooltip: text

    alternateShortcut.enabled: false
}
