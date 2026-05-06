// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick.Controls as Controls
import io.github.hnjae.kiriview

Controls.MenuBar {
    id: root

    required property var actions
    required property KiriImageDocument imageDocument
    required property bool fullscreen

    Controls.Menu {
        title: "File"

        Controls.MenuItem {
            action: root.actions.openAction
        }

        Controls.MenuSeparator {}

        Controls.MenuItem {
            action: root.actions.quitAction
        }
    }

    Controls.Menu {
        title: "Go"

        Controls.MenuItem {
            action: root.actions.previousImageAction
        }

        Controls.MenuItem {
            action: root.actions.nextImageAction
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
        title: "View"

        Controls.MenuItem {
            action: root.actions.zoomInAction
        }

        Controls.MenuItem {
            action: root.actions.zoomOutAction
        }

        Controls.MenuSeparator {}

        Controls.Menu {
            title: "Fit"

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
            action: root.actions.fullscreenAction
            checkable: true
            checked: root.fullscreen
        }
    }

    Controls.Menu {
        title: "Settings"

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
        title: "Help"

        Controls.MenuItem {
            action: root.actions.shortcutHelpAction
        }
    }
}
