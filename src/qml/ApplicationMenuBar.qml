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
        title: KI18n.i18nc("@title:menu", "File")

        Controls.MenuItem {
            action: root.actions.openAction
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.moveToTrashAction
        }

        Controls.MenuItem {
            action: root.actions.deleteFileAction
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.quitAction
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "Go")

        Controls.MenuItem {
            action: root.actions.previousImageAction
        }

        Controls.MenuItem {
            action: root.actions.nextImageAction
        }

        Controls.MenuItem {
            action: root.actions.previousSinglePageAction
        }

        Controls.MenuItem {
            action: root.actions.nextSinglePageAction
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.firstImageAction
        }

        Controls.MenuItem {
            action: root.actions.lastImageAction
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.previousContainerAction
        }

        Controls.MenuItem {
            action: root.actions.nextContainerAction
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "View")

        Controls.MenuItem {
            action: root.actions.zoomInAction
        }

        Controls.MenuItem {
            action: root.actions.zoomOutAction
        }

        Controls.MenuSeparator {}

        Controls.Menu {
            title: KI18n.i18nc("@title:menu", "Fit")

            Controls.MenuItem {
                action: root.actions.fitAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.Fit
            }

            Controls.MenuItem {
                action: root.actions.fitHeightAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.FitHeight
            }

            Controls.MenuItem {
                action: root.actions.fitWidthAction
                checkable: true
                checked: root.imageDocument.zoomMode === KiriImageDocument.FitWidth
            }
        }

        Controls.MenuItem {
            action: root.actions.actualSizeAction
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.twoPageModeAction
            checkable: true
            checked: root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.fullscreenAction
            checkable: true
            checked: root.fullscreen
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "Settings")

        Controls.MenuItem {
            action: root.actions.showMenubarAction
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.configureAction
        }

        Controls.MenuItem {
            action: root.actions.configureShortcutsAction
        }
    }

    Controls.Menu {
        title: KI18n.i18nc("@title:menu", "Help")

        Controls.MenuItem {
            action: root.actions.shortcutHelpAction
        }
    }
}
