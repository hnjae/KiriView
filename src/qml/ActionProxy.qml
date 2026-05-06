// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigami.private as KirigamiPrivate

Kirigami.Action {
    id: root

    required property var sourceAction
    property bool enabledOverride: sourceAction && sourceAction.enabled

    checkable: sourceAction ? sourceAction.checkable : false
    checked: sourceAction ? sourceAction.checked : false
    enabled: sourceAction && enabledOverride
    icon.name: sourceAction ? KirigamiPrivate.ActionHelper.iconName(sourceAction.icon) : ""
    text: sourceAction ? sourceAction.text : ""

    onTriggered: {
        if (sourceAction) {
            sourceAction.trigger();
        }
    }
}
