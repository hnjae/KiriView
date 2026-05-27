// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick.Controls as Controls
import io.github.hnjae.kiriview
import org.kde.ki18n

Controls.MenuBar {
    id: root

    required property var actions
    required property KiriImageDocument imageDocument
    required property bool fullscreen
    property bool imageMode: true
    property bool mediaMode: imageMode
    property bool rightToLeftReadingActive: false

    readonly property var leadingArchiveMenuAction: root.rightToLeftReadingActive ? root.actions.nextContainerMenuAction : root.actions.previousContainerMenuAction
    readonly property string leadingArchiveMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "Nex&t Archive") : KI18n.i18nc("@action:inmenu", "Previous A&rchive")
    readonly property var leadingImageMenuAction: root.rightToLeftReadingActive ? root.actions.nextImageMenuAction : root.actions.previousImageMenuAction
    readonly property string leadingImageMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "&Next") : KI18n.i18nc("@action:inmenu", "&Previous")
    readonly property var trailingArchiveMenuAction: root.rightToLeftReadingActive ? root.actions.previousContainerMenuAction : root.actions.nextContainerMenuAction
    readonly property string trailingArchiveMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "Previous A&rchive") : KI18n.i18nc("@action:inmenu", "Nex&t Archive")
    readonly property var trailingImageMenuAction: root.rightToLeftReadingActive ? root.actions.previousImageMenuAction : root.actions.nextImageMenuAction
    readonly property string trailingImageMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "&Previous") : KI18n.i18nc("@action:inmenu", "&Next")
    readonly property string leadingArchiveMenuIconName: root.actions.previousContainerMenuAction?.icon.name ?? ""
    readonly property string leadingImageMenuIconName: root.actions.previousImageMenuAction?.icon.name ?? ""
    readonly property string firstImageMenuIconName: root.rightToLeftReadingActive ? (root.actions.lastImageMenuAction?.icon.name ?? "") : (root.actions.firstImageMenuAction?.icon.name ?? "")
    readonly property string lastImageMenuIconName: root.rightToLeftReadingActive ? (root.actions.firstImageMenuAction?.icon.name ?? "") : (root.actions.lastImageMenuAction?.icon.name ?? "")
    readonly property string trailingArchiveMenuIconName: root.actions.nextContainerMenuAction?.icon.name ?? ""
    readonly property string trailingImageMenuIconName: root.actions.nextImageMenuAction?.icon.name ?? ""

    Controls.Menu {
        id: fileMenu

        title: KI18n.i18nc("@title:menu", "&File")

        MenuAccessKeyRouter {
            menu: fileMenu
        }

        MenuActionItem {
            action: root.actions.openMenuAction
            text: KI18n.i18nc("@action:inmenu", "&Open")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.moveToTrashMenuAction
            text: KI18n.i18nc("@action:inmenu", "Move to &Trash")
        }

        MenuActionItem {
            action: root.actions.deleteFileMenuAction
            text: KI18n.i18nc("@action:inmenu", "Delete &Permanently")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.quitMenuAction
            text: KI18n.i18nc("@action:inmenu", "&Quit")
        }
    }

    Controls.Menu {
        id: goMenu

        title: KI18n.i18nc("@title:menu", "&Go")

        MenuAccessKeyRouter {
            menu: goMenu
        }

        MenuActionItem {
            action: root.leadingImageMenuAction
            icon.name: root.leadingImageMenuIconName
            text: root.leadingImageMenuText
        }

        MenuActionItem {
            action: root.trailingImageMenuAction
            icon.name: root.trailingImageMenuIconName
            text: root.trailingImageMenuText
        }

        MenuActionItem {
            action: root.actions.firstImageMenuAction
            icon.name: root.firstImageMenuIconName
            text: root.imageMode ? KI18n.i18nc("@action:inmenu", "&First Image") : KI18n.i18nc("@action:inmenu", "&First Media Item")
            visible: root.mediaMode
        }

        MenuActionItem {
            action: root.actions.lastImageMenuAction
            icon.name: root.lastImageMenuIconName
            text: root.imageMode ? KI18n.i18nc("@action:inmenu", "&Last Image") : KI18n.i18nc("@action:inmenu", "&Last Media Item")
            visible: root.mediaMode
        }

        Controls.MenuSeparator {
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.leadingArchiveMenuAction
            icon.name: root.leadingArchiveMenuIconName
            text: root.leadingArchiveMenuText
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.trailingArchiveMenuAction
            icon.name: root.trailingArchiveMenuIconName
            text: root.trailingArchiveMenuText
            visible: root.imageMode
        }
    }

    Controls.Menu {
        id: viewMenu

        title: KI18n.i18nc("@title:menu", "&View")

        MenuAccessKeyRouter {
            menu: viewMenu
        }

        MenuActionItem {
            action: root.actions.zoomInMenuAction
            text: KI18n.i18nc("@action:inmenu", "&Zoom In")
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.zoomOutMenuAction
            text: KI18n.i18nc("@action:inmenu", "Zoom &Out")
            visible: root.imageMode
        }

        Controls.MenuSeparator {
            visible: root.imageMode
        }

        Controls.Menu {
            id: fitMenu

            enabled: root.imageMode
            title: KI18n.i18nc("@title:menu", "&Fit")

            MenuActionItem {
                action: root.actions.fitMenuAction
                checkable: true
                checked: root.imageMode && root.imageDocument.zoomMode === KiriImageDocument.Fit
                text: KI18n.i18nc("@action:inmenu", "&Fit")
            }

            MenuActionItem {
                action: root.actions.fitHeightMenuAction
                checkable: true
                checked: root.imageMode && root.imageDocument.zoomMode === KiriImageDocument.FitHeight
                text: KI18n.i18nc("@action:inmenu", "Fit &Height")
            }

            MenuActionItem {
                action: root.actions.fitWidthMenuAction
                checkable: true
                checked: root.imageMode && root.imageDocument.zoomMode === KiriImageDocument.FitWidth
                text: KI18n.i18nc("@action:inmenu", "Fit &Width")
            }
        }

        MenuActionItem {
            action: root.actions.actualSizeMenuAction
            text: KI18n.i18nc("@action:inmenu", "&Actual Size")
            visible: root.imageMode
        }

        Controls.MenuSeparator {
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.rotateClockwiseMenuAction
            text: KI18n.i18nc("@action:inmenu", "Rotate &Clockwise")
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.rotateCounterclockwiseMenuAction
            text: KI18n.i18nc("@action:inmenu", "Rotate C&ounterclockwise")
            visible: root.imageMode
        }

        Controls.MenuSeparator {
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.twoPageModeMenuAction
            checkable: true
            checked: root.imageMode && root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable
            text: KI18n.i18nc("@action:inmenu", "Two-Page &Spread")
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.rightToLeftReadingMenuAction
            checkable: true
            checked: root.imageMode && root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable
            text: KI18n.i18nc("@action:inmenu", "&Right-to-Left Reading")
            visible: root.imageMode
        }

        Controls.MenuSeparator {
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.infoPanelMenuAction ?? null
            checkable: true
            checked: root.actions.infoPanelMenuAction?.checked ?? false
            text: KI18n.i18nc("@action:inmenu", "Show &Info Panel")
            visible: root.actions.infoPanelMenuAction !== undefined
        }

        MenuActionItem {
            action: root.actions.thumbnailPanelMenuAction ?? null
            checkable: true
            checked: root.actions.thumbnailPanelMenuAction?.checked ?? false
            text: KI18n.i18nc("@action:inmenu", "Show &Thumbnail Panel")
            visible: root.actions.thumbnailPanelMenuAction !== undefined
        }

        Controls.MenuSeparator {
            visible: root.actions.infoPanelMenuAction !== undefined || root.actions.thumbnailPanelMenuAction !== undefined
        }

        MenuActionItem {
            action: root.actions.fullscreenMenuAction
            checkable: true
            checked: root.fullscreen
            text: KI18n.i18nc("@action:inmenu", "Full&screen")
        }
    }

    Controls.Menu {
        id: settingsMenu

        title: KI18n.i18nc("@title:menu", "&Settings")

        MenuAccessKeyRouter {
            menu: settingsMenu
        }

        MenuActionItem {
            action: root.actions.showMenubarMenuAction
            text: KI18n.i18nc("@action:inmenu", "Show &Menubar")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.configureShortcutsMenuAction
            text: KI18n.i18nc("@action:inmenu", "Configure &Shortcuts...")
        }
    }

    Controls.Menu {
        id: helpMenu

        title: KI18n.i18nc("@title:menu", "&Help")

        MenuAccessKeyRouter {
            menu: helpMenu
        }

        MenuActionItem {
            action: root.actions.shortcutHelpMenuAction
            text: KI18n.i18nc("@action:inmenu", "&Keyboard Shortcuts")
        }
    }
}
