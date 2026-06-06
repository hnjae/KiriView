// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQml

QtObject {
    id: root

    required property var actions
    property bool rightToLeftReadingActive: false

    readonly property var leadingImageAction: root.rightToLeftReadingActive ? root.actions.nextImageAction : root.actions.previousImageAction
    readonly property var trailingImageAction: root.rightToLeftReadingActive ? root.actions.previousImageAction : root.actions.nextImageAction
    readonly property string leadingImageIconName: root.actions.previousImageAction?.icon.name ?? ""
    readonly property string trailingImageIconName: root.actions.nextImageAction?.icon.name ?? ""

    readonly property var leadingImageMenuAction: root.rightToLeftReadingActive ? root.actions.nextImageMenuAction : root.actions.previousImageMenuAction
    readonly property var trailingImageMenuAction: root.rightToLeftReadingActive ? root.actions.previousImageMenuAction : root.actions.nextImageMenuAction
    readonly property string leadingImageMenuIconName: root.actions.previousImageMenuAction?.icon.name ?? ""
    readonly property string trailingImageMenuIconName: root.actions.nextImageMenuAction?.icon.name ?? ""

    readonly property var firstImageMenuAction: root.actions.firstImageMenuAction
    readonly property var lastImageMenuAction: root.actions.lastImageMenuAction
    readonly property string firstImageMenuIconName: root.rightToLeftReadingActive ? (root.actions.lastImageMenuAction?.icon.name ?? "") : (root.actions.firstImageMenuAction?.icon.name ?? "")
    readonly property string lastImageMenuIconName: root.rightToLeftReadingActive ? (root.actions.firstImageMenuAction?.icon.name ?? "") : (root.actions.lastImageMenuAction?.icon.name ?? "")

    readonly property var leadingArchiveMenuAction: root.rightToLeftReadingActive ? root.actions.nextContainerMenuAction : root.actions.previousContainerMenuAction
    readonly property var trailingArchiveMenuAction: root.rightToLeftReadingActive ? root.actions.previousContainerMenuAction : root.actions.nextContainerMenuAction
    readonly property string leadingArchiveMenuIconName: root.actions.previousContainerMenuAction?.icon.name ?? ""
    readonly property string trailingArchiveMenuIconName: root.actions.nextContainerMenuAction?.icon.name ?? ""
    readonly property var applicationMenuNavigationActions: [leadingArchiveMenuAction, trailingArchiveMenuAction]
}
