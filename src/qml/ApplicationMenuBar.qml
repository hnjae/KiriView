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
    property bool rightToLeftReadingActive: false

    readonly property var leadingArchiveMenuAction: root.rightToLeftReadingActive ? root.actions.nextContainerMenuAction : root.actions.previousContainerMenuAction
    readonly property string leadingArchiveMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "Nex&t Archive") : KI18n.i18nc("@action:inmenu", "Previous A&rchive")
    readonly property var leadingImageMenuAction: root.rightToLeftReadingActive ? root.actions.nextImageMenuAction : root.actions.previousImageMenuAction
    readonly property string leadingImageMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "&Next") : KI18n.i18nc("@action:inmenu", "&Previous")
    readonly property var leadingSinglePageMenuAction: root.rightToLeftReadingActive ? root.actions.nextSinglePageMenuAction : root.actions.previousSinglePageMenuAction
    readonly property string leadingSinglePageMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "Ne&xt Page") : KI18n.i18nc("@action:inmenu", "Previous P&age")
    readonly property var trailingArchiveMenuAction: root.rightToLeftReadingActive ? root.actions.previousContainerMenuAction : root.actions.nextContainerMenuAction
    readonly property string trailingArchiveMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "Previous A&rchive") : KI18n.i18nc("@action:inmenu", "Nex&t Archive")
    readonly property var trailingImageMenuAction: root.rightToLeftReadingActive ? root.actions.previousImageMenuAction : root.actions.nextImageMenuAction
    readonly property string trailingImageMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "&Previous") : KI18n.i18nc("@action:inmenu", "&Next")
    readonly property var trailingSinglePageMenuAction: root.rightToLeftReadingActive ? root.actions.previousSinglePageMenuAction : root.actions.nextSinglePageMenuAction
    readonly property string trailingSinglePageMenuText: root.rightToLeftReadingActive ? KI18n.i18nc("@action:inmenu", "Previous P&age") : KI18n.i18nc("@action:inmenu", "Ne&xt Page")

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
            text: root.leadingImageMenuText
        }

        MenuActionItem {
            action: root.trailingImageMenuAction
            text: root.trailingImageMenuText
        }

        MenuActionItem {
            action: root.leadingSinglePageMenuAction
            text: root.leadingSinglePageMenuText
        }

        MenuActionItem {
            action: root.trailingSinglePageMenuAction
            text: root.trailingSinglePageMenuText
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.firstImageMenuAction
            text: KI18n.i18nc("@action:inmenu", "&First Image")
        }

        MenuActionItem {
            action: root.actions.lastImageMenuAction
            text: KI18n.i18nc("@action:inmenu", "&Last Image")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.leadingArchiveMenuAction
            text: root.leadingArchiveMenuText
        }

        MenuActionItem {
            action: root.trailingArchiveMenuAction
            text: root.trailingArchiveMenuText
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
        }

        MenuActionItem {
            action: root.actions.zoomOutMenuAction
            text: KI18n.i18nc("@action:inmenu", "Zoom &Out")
        }

        Controls.MenuSeparator {}

        Controls.Menu {
            id: fitMenu

            title: KI18n.i18nc("@title:menu", "&Fit")

            MenuActionItem {
                action: root.actions.fitMenuAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.Fit
                text: KI18n.i18nc("@action:inmenu", "&Fit")
            }

            MenuActionItem {
                action: root.actions.fitHeightMenuAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.FitHeight
                text: KI18n.i18nc("@action:inmenu", "Fit &Height")
            }

            MenuActionItem {
                action: root.actions.fitWidthMenuAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.FitWidth
                text: KI18n.i18nc("@action:inmenu", "Fit &Width")
            }
        }

        MenuActionItem {
            action: root.actions.actualSizeMenuAction
            text: KI18n.i18nc("@action:inmenu", "&Actual Size")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.rotateClockwiseMenuAction
            text: KI18n.i18nc("@action:inmenu", "Rotate &Clockwise")
        }

        MenuActionItem {
            action: root.actions.rotateCounterclockwiseMenuAction
            text: KI18n.i18nc("@action:inmenu", "Rotate C&ounterclockwise")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.twoPageModeMenuAction
            checkable: true
            checked: root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable
            text: KI18n.i18nc("@action:inmenu", "Two-Page &Spread")
        }

        MenuActionItem {
            action: root.actions.rightToLeftReadingMenuAction
            checkable: true
            checked: root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable
            text: KI18n.i18nc("@action:inmenu", "&Right-to-Left Reading")
        }

        Controls.MenuSeparator {}

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
