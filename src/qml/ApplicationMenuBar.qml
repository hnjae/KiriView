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

        Controls.MenuItem {
            action: root.actions.openAction
            text: KI18n.i18nc("@action:inmenu", "&Open")
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.moveToTrashAction
            text: KI18n.i18nc("@action:inmenu", "Move to &Trash")
        }

        Controls.MenuItem {
            action: root.actions.deleteFileAction
            text: KI18n.i18nc("@action:inmenu", "Delete &Permanently")
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.quitAction
            text: KI18n.i18nc("@action:inmenu", "&Quit")
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "&Go")

        Controls.MenuItem {
            action: root.actions.previousImageAction
            text: KI18n.i18nc("@action:inmenu", "&Previous")
        }

        Controls.MenuItem {
            action: root.actions.nextImageAction
            text: KI18n.i18nc("@action:inmenu", "&Next")
        }

        Controls.MenuItem {
            action: root.actions.previousSinglePageAction
            text: KI18n.i18nc("@action:inmenu", "Previous P&age")
        }

        Controls.MenuItem {
            action: root.actions.nextSinglePageAction
            text: KI18n.i18nc("@action:inmenu", "Ne&xt Page")
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.firstImageAction
            text: KI18n.i18nc("@action:inmenu", "&First Image")
        }

        Controls.MenuItem {
            action: root.actions.lastImageAction
            text: KI18n.i18nc("@action:inmenu", "&Last Image")
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.previousContainerAction
            text: KI18n.i18nc("@action:inmenu", "Previous A&rchive")
        }

        Controls.MenuItem {
            action: root.actions.nextContainerAction
            text: KI18n.i18nc("@action:inmenu", "Nex&t Archive")
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "&View")

        Controls.MenuItem {
            action: root.actions.zoomInAction
            text: KI18n.i18nc("@action:inmenu", "&Zoom In")
        }

        Controls.MenuItem {
            action: root.actions.zoomOutAction
            text: KI18n.i18nc("@action:inmenu", "Zoom &Out")
        }

        Controls.MenuSeparator {}

        Controls.Menu {
            title: KI18n.i18nc("@title:menu", "&Fit")

            Controls.MenuItem {
                action: root.actions.fitAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.Fit
                text: KI18n.i18nc("@action:inmenu", "&Fit")
            }

            Controls.MenuItem {
                action: root.actions.fitHeightAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.FitHeight
                text: KI18n.i18nc("@action:inmenu", "Fit &Height")
            }

            Controls.MenuItem {
                action: root.actions.fitWidthAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.FitWidth
                text: KI18n.i18nc("@action:inmenu", "Fit &Width")
            }
        }

        Controls.MenuItem {
            action: root.actions.actualSizeAction
            text: KI18n.i18nc("@action:inmenu", "&Actual Size")
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.twoPageModeAction
            checkable: true
            checked: root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable
            text: KI18n.i18nc("@action:inmenu", "Two-Page &Spread")
        }

        Controls.MenuItem {
            action: root.actions.rightToLeftReadingAction
            checkable: true
            checked: root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable
            text: KI18n.i18nc("@action:inmenu", "&Right-to-Left Reading")
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.fullscreenAction
            checkable: true
            checked: root.fullscreen
            text: KI18n.i18nc("@action:inmenu", "Full&screen")
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "&Settings")

        Controls.MenuItem {
            action: root.actions.showMenubarAction
            text: KI18n.i18nc("@action:inmenu", "Show &Menubar")
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.configureShortcutsAction
            text: KI18n.i18nc("@action:inmenu", "Configure &Shortcuts...")
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "&Help")

        Controls.MenuItem {
            action: root.actions.shortcutHelpAction
            text: KI18n.i18nc("@action:inmenu", "&Keyboard Shortcuts")
        }
    }
}
