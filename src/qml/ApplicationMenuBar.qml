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

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "&File")

        MenuActionItem {
            action: root.actions.openAction
            text: KI18n.i18nc("@action:inmenu", "&Open")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.moveToTrashAction
            text: KI18n.i18nc("@action:inmenu", "Move to &Trash")
        }

        MenuActionItem {
            action: root.actions.deleteFileAction
            text: KI18n.i18nc("@action:inmenu", "Delete &Permanently")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.quitAction
            text: KI18n.i18nc("@action:inmenu", "&Quit")
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "&Go")

        MenuActionItem {
            action: root.actions.previousImageAction
            text: KI18n.i18nc("@action:inmenu", "&Previous")
        }

        MenuActionItem {
            action: root.actions.nextImageAction
            text: KI18n.i18nc("@action:inmenu", "&Next")
        }

        MenuActionItem {
            action: root.actions.previousSinglePageAction
            text: KI18n.i18nc("@action:inmenu", "Previous P&age")
        }

        MenuActionItem {
            action: root.actions.nextSinglePageAction
            text: KI18n.i18nc("@action:inmenu", "Ne&xt Page")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.firstImageAction
            text: KI18n.i18nc("@action:inmenu", "&First Image")
        }

        MenuActionItem {
            action: root.actions.lastImageAction
            text: KI18n.i18nc("@action:inmenu", "&Last Image")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.previousContainerAction
            text: KI18n.i18nc("@action:inmenu", "Previous A&rchive")
        }

        MenuActionItem {
            action: root.actions.nextContainerAction
            text: KI18n.i18nc("@action:inmenu", "Nex&t Archive")
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "&View")

        MenuActionItem {
            action: root.actions.zoomInAction
            text: KI18n.i18nc("@action:inmenu", "&Zoom In")
        }

        MenuActionItem {
            action: root.actions.zoomOutAction
            text: KI18n.i18nc("@action:inmenu", "Zoom &Out")
        }

        Controls.MenuSeparator {}

        Controls.Menu {
            title: KI18n.i18nc("@title:menu", "&Fit")

            MenuActionItem {
                action: root.actions.fitAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.Fit
                text: KI18n.i18nc("@action:inmenu", "&Fit")
            }

            MenuActionItem {
                action: root.actions.fitHeightAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.FitHeight
                text: KI18n.i18nc("@action:inmenu", "Fit &Height")
            }

            MenuActionItem {
                action: root.actions.fitWidthAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.FitWidth
                text: KI18n.i18nc("@action:inmenu", "Fit &Width")
            }
        }

        MenuActionItem {
            action: root.actions.actualSizeAction
            text: KI18n.i18nc("@action:inmenu", "&Actual Size")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.rotateClockwiseAction
            text: KI18n.i18nc("@action:inmenu", "Rotate &Clockwise")
        }

        MenuActionItem {
            action: root.actions.rotateCounterclockwiseAction
            text: KI18n.i18nc("@action:inmenu", "Rotate C&ounterclockwise")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.twoPageModeAction
            checkable: true
            checked: root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable
            text: KI18n.i18nc("@action:inmenu", "Two-Page &Spread")
        }

        MenuActionItem {
            action: root.actions.rightToLeftReadingAction
            checkable: true
            checked: root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable
            text: KI18n.i18nc("@action:inmenu", "&Right-to-Left Reading")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.fullscreenAction
            checkable: true
            checked: root.fullscreen
            text: KI18n.i18nc("@action:inmenu", "Full&screen")
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "&Settings")

        MenuActionItem {
            action: root.actions.showMenubarAction
            text: KI18n.i18nc("@action:inmenu", "Show &Menubar")
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.configureShortcutsAction
            text: KI18n.i18nc("@action:inmenu", "Configure &Shortcuts...")
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "&Help")

        MenuActionItem {
            action: root.actions.shortcutHelpAction
            text: KI18n.i18nc("@action:inmenu", "&Keyboard Shortcuts")
        }
    }
}
