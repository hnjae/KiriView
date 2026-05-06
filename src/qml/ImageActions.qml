// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import io.github.hnjae.kiriview

Item {
    id: root

    required property KiriViewApplication application
    required property KiriImageDocument imageDocument
    required property bool imageReady
    required property bool helpDialogOpen
    required property bool fullscreen

    readonly property bool canOpenNextImage: root.imageReady && root.imageDocument.currentPageNumber > 0 && root.imageDocument.currentPageNumber < root.imageDocument.imageCount
    readonly property bool canOpenPreviousImage: root.imageReady && root.imageDocument.currentPageNumber > 1
    readonly property bool canUsePageActions: root.imageReady && root.imageDocument.imageCount > 0 && !root.helpDialogOpen
    readonly property bool canUseReadyActions: root.imageReady && !root.helpDialogOpen

    readonly property var openAction: openManagedAction.proxy
    readonly property var previousContainerAction: previousContainerManagedAction.proxy
    readonly property var nextContainerAction: nextContainerManagedAction.proxy
    readonly property var previousImageAction: previousImageManagedAction.proxy
    readonly property var nextImageAction: nextImageManagedAction.proxy
    readonly property var firstImageAction: firstImageManagedAction.proxy
    readonly property var lastImageAction: lastImageManagedAction.proxy
    readonly property var fitAction: fitManagedAction.proxy
    readonly property var fitHeightAction: fitHeightManagedAction.proxy
    readonly property var fitWidthAction: fitWidthManagedAction.proxy
    readonly property var actualSizeAction: actualSizeManagedAction.proxy
    readonly property var zoomInAction: zoomInManagedAction.proxy
    readonly property var zoomOutAction: zoomOutManagedAction.proxy
    readonly property var scanForwardAction: scanForwardManagedAction.proxy
    readonly property var scanBackwardAction: scanBackwardManagedAction.proxy
    readonly property var fullscreenAction: fullscreenManagedAction.proxy
    readonly property var shortcutHelpAction: shortcutHelpManagedAction.proxy
    readonly property var configureAction: configureManagedAction.proxy
    readonly property var configureShortcutsAction: configureShortcutsManagedAction.proxy
    readonly property var showMenubarAction: showMenubarManagedAction.proxy
    readonly property var quitAction: quitManagedAction.proxy
    readonly property var applicationMenuActions: [openAction, applicationMenuFileSeparator, previousContainerAction, nextContainerAction, applicationMenuViewSeparator, fullscreenAction, applicationMenuSettingsSeparator, showMenubarAction, configureAction, configureShortcutsAction, applicationMenuHelpSeparator, shortcutHelpAction, applicationMenuQuitSeparator, quitAction]

    signal openDialogRequested
    signal imageBoundaryReached(string message)
    signal shortcutHelpRequested
    signal toggleFullScreenRequested

    function openFirstImage() {
        if (root.imageDocument.imageCount > 0) {
            root.imageDocument.openImageAtPage(1);
        }
    }

    function openLastImage() {
        if (root.imageDocument.imageCount > 0) {
            root.imageDocument.openImageAtPage(root.imageDocument.imageCount);
        }
    }

    function openNextImage() {
        if (root.imageReady && root.imageDocument.currentPageNumber === root.imageDocument.imageCount) {
            root.imageBoundaryReached("Last image");
            return;
        }

        root.imageDocument.openNextImage();
    }

    function openPreviousImage() {
        if (root.imageReady && root.imageDocument.currentPageNumber === 1 && root.imageDocument.imageCount > 0) {
            root.imageBoundaryReached("First image");
            return;
        }

        root.imageDocument.openPreviousImage();
    }

    property Kirigami.Action applicationMenuFileSeparator: Kirigami.Action {
        displayHint: Kirigami.DisplayHint.AlwaysHide
        separator: true
    }

    property Kirigami.Action applicationMenuViewSeparator: Kirigami.Action {
        displayHint: Kirigami.DisplayHint.AlwaysHide
        separator: true
    }

    property Kirigami.Action applicationMenuSettingsSeparator: Kirigami.Action {
        displayHint: Kirigami.DisplayHint.AlwaysHide
        separator: true
    }

    property Kirigami.Action applicationMenuHelpSeparator: Kirigami.Action {
        displayHint: Kirigami.DisplayHint.AlwaysHide
        separator: true
    }

    property Kirigami.Action applicationMenuQuitSeparator: Kirigami.Action {
        displayHint: Kirigami.DisplayHint.AlwaysHide
        separator: true
    }

    ManagedAction {
        id: openManagedAction

        actionEnabled: !root.helpDialogOpen
        actionName: "file_open"
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide

        onTriggered: root.openDialogRequested()
    }

    ManagedAction {
        id: previousContainerManagedAction

        actionEnabled: root.imageDocument.containerNavigationAvailable && !root.helpDialogOpen
        actionName: "go_previous_archive"
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide

        onTriggered: root.imageDocument.openPreviousContainer()
    }

    ManagedAction {
        id: nextContainerManagedAction

        actionEnabled: root.imageDocument.containerNavigationAvailable && !root.helpDialogOpen
        actionName: "go_next_archive"
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide

        onTriggered: root.imageDocument.openNextContainer()
    }

    ManagedAction {
        id: previousImageManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "go_previous_image"
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canOpenPreviousImage && !root.helpDialogOpen

        onTriggered: root.openPreviousImage()
    }

    ManagedAction {
        id: nextImageManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "go_next_image"
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canOpenNextImage && !root.helpDialogOpen

        onTriggered: root.openNextImage()
    }

    ManagedAction {
        id: firstImageManagedAction

        actionEnabled: root.canUsePageActions
        actionName: "go_first_image"
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canUsePageActions

        onTriggered: root.openFirstImage()
    }

    ManagedAction {
        id: lastImageManagedAction

        actionEnabled: root.canUsePageActions
        actionName: "go_last_image"
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canUsePageActions

        onTriggered: root.openLastImage()
    }

    ManagedAction {
        id: fitManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "view_fit"
        application: root.application
        bindEnabled: true
        proxyCheckable: true
        proxyChecked: root.imageDocument.zoomMode === KiriImageDocument.Fit

        onTriggered: root.imageDocument.resetZoom()
    }

    ManagedAction {
        id: fitHeightManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "view_fit_height"
        application: root.application
        bindEnabled: true
        proxyCheckable: true
        proxyChecked: root.imageDocument.zoomMode === KiriImageDocument.FitHeight

        onTriggered: root.imageDocument.setFitMode(KiriImageDocument.FitHeight)
    }

    ManagedAction {
        id: fitWidthManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "view_fit_width"
        application: root.application
        bindEnabled: true
        proxyCheckable: true
        proxyChecked: root.imageDocument.zoomMode === KiriImageDocument.FitWidth

        onTriggered: root.imageDocument.setFitMode(KiriImageDocument.FitWidth)
    }

    ManagedAction {
        id: actualSizeManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "view_actual_size"
        application: root.application
        bindEnabled: true

        onTriggered: root.imageDocument.zoomPercent = 100
    }

    ManagedAction {
        id: zoomInManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "view_zoom_in"
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: zoomOutManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "view_zoom_out"
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: scanForwardManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "view_scan_forward"
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: scanBackwardManagedAction

        actionEnabled: root.canUseReadyActions
        actionName: "view_scan_backward"
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: fullscreenManagedAction

        actionChecked: root.fullscreen
        actionEnabled: !root.helpDialogOpen
        actionName: "window_fullscreen"
        application: root.application
        bindChecked: true
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        proxyCheckable: true
        proxyChecked: root.fullscreen

        onTriggered: root.toggleFullScreenRequested()
    }

    ManagedAction {
        id: shortcutHelpManagedAction

        actionEnabled: !root.helpDialogOpen
        actionName: "help_shortcuts"
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide

        onTriggered: root.shortcutHelpRequested()
    }

    ManagedAction {
        id: configureManagedAction

        actionName: "options_configure"
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
    }

    ManagedAction {
        id: configureShortcutsManagedAction

        actionName: "options_configure_keybinding"
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
    }

    ManagedAction {
        id: showMenubarManagedAction

        actionName: "options_show_menubar"
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
    }

    ManagedAction {
        id: quitManagedAction

        actionName: "file_quit"
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
    }
}
