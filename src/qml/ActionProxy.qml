// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigami.private as KirigamiPrivate

Kirigami.Action {
    id: root

    required property var sourceAction
    property bool checkableOverride: sourceAction ? sourceAction.checkable : false
    property bool checkedOverride: sourceAction ? sourceAction.checked : false
    property bool enabledOverride: sourceAction && sourceAction.enabled

    checkable: checkableOverride
    checked: checkable && checkedOverride
    enabled: sourceAction && enabledOverride
    icon.name: sourceAction ? KirigamiPrivate.ActionHelper.iconName(sourceAction.icon) : ""
    text: sourceAction ? sourceAction.text : ""
    tooltip: text

    onTriggered: {
        if (sourceAction) {
            sourceAction.trigger();
        }
    }
}
