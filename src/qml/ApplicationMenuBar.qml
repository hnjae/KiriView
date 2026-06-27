// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick.Controls as Controls
import org.hnjae.kiriview
import org.kde.ki18n

Controls.MenuBar {
    id: root

    required property var actions
    required property var navigationPresentationProvider
    property bool imageMode: true
    property bool mediaMode: imageMode

    readonly property var leadingArchiveMenuAction: navigationPresentationOrder.leadingArchiveMenuAction
    readonly property var leadingImageMenuAction: navigationPresentationOrder.leadingImageMenuAction
    readonly property var trailingArchiveMenuAction: navigationPresentationOrder.trailingArchiveMenuAction
    readonly property var trailingImageMenuAction: navigationPresentationOrder.trailingImageMenuAction
    readonly property string leadingArchiveMenuIconName: navigationPresentationOrder.leadingArchiveMenuIconName
    readonly property string leadingImageMenuIconName: navigationPresentationOrder.leadingImageMenuIconName
    readonly property string firstImageMenuIconName: navigationPresentationOrder.firstImageMenuIconName
    readonly property string lastImageMenuIconName: navigationPresentationOrder.lastImageMenuIconName
    readonly property string trailingArchiveMenuIconName: navigationPresentationOrder.trailingArchiveMenuIconName
    readonly property string trailingImageMenuIconName: navigationPresentationOrder.trailingImageMenuIconName

    NavigationPresentationOrder {
        id: navigationPresentationOrder

        actions: root.actions
        projectionProvider: root.navigationPresentationProvider
    }

    Controls.Menu {
        id: fileMenu

        title: KI18n.i18nc("@title:menu", "&File")

        MenuAccessKeyRouter {
            menu: fileMenu
        }

        MenuActionItem {
            action: root.actions.openMenuAction
        }

        MenuActionItem {
            action: root.actions.openWithMenuAction
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.moveToTrashMenuAction
        }

        MenuActionItem {
            action: root.actions.deleteFileMenuAction
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.quitMenuAction
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
        }

        MenuActionItem {
            action: root.trailingImageMenuAction
            icon.name: root.trailingImageMenuIconName
        }

        MenuActionItem {
            action: navigationPresentationOrder.firstImageMenuAction
            icon.name: root.firstImageMenuIconName
            visible: root.mediaMode
        }

        MenuActionItem {
            action: navigationPresentationOrder.lastImageMenuAction
            icon.name: root.lastImageMenuIconName
            visible: root.mediaMode
        }

        Controls.MenuSeparator {
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.leadingArchiveMenuAction
            icon.name: root.leadingArchiveMenuIconName
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.trailingArchiveMenuAction
            icon.name: root.trailingArchiveMenuIconName
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
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.zoomOutMenuAction
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.zoom50PercentMenuAction
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.zoom100PercentMenuAction
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.zoom200PercentMenuAction
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
                checked: root.actions.fitMenuAction?.checked ?? false
            }

            MenuActionItem {
                action: root.actions.fitHeightMenuAction
                checkable: true
                checked: root.actions.fitHeightMenuAction?.checked ?? false
            }

            MenuActionItem {
                action: root.actions.fitWidthMenuAction
                checkable: true
                checked: root.actions.fitWidthMenuAction?.checked ?? false
            }
        }

        Controls.MenuSeparator {
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.rotateClockwiseMenuAction
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.rotateCounterclockwiseMenuAction
            visible: root.imageMode
        }

        Controls.MenuSeparator {
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.twoPageModeMenuAction
            checkable: true
            checked: root.actions.twoPageModeMenuAction?.checked ?? false
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.rightToLeftReadingMenuAction
            checkable: true
            checked: root.actions.rightToLeftReadingMenuAction?.checked ?? false
            visible: root.imageMode
        }

        Controls.MenuSeparator {
            visible: root.imageMode
        }

        MenuActionItem {
            action: root.actions.infoPanelMenuAction ?? null
            checkable: true
            checked: root.actions.infoPanelMenuAction?.checked ?? false
            visible: root.actions.infoPanelMenuAction !== undefined
        }

        MenuActionItem {
            action: root.actions.thumbnailPanelMenuAction ?? null
            checkable: true
            checked: root.actions.thumbnailPanelMenuAction?.checked ?? false
            visible: root.actions.thumbnailPanelMenuAction !== undefined
        }

        Controls.MenuSeparator {
            visible: root.actions.infoPanelMenuAction !== undefined || root.actions.thumbnailPanelMenuAction !== undefined
        }

        MenuActionItem {
            action: root.actions.fullscreenMenuAction
            checkable: true
            checked: root.actions.fullscreenMenuAction?.checked ?? false
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
        }

        Controls.MenuSeparator {}

        MenuActionItem {
            action: root.actions.configureShortcutsMenuAction
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
        }
    }
}
