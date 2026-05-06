// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQml
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

    readonly property var openQAction: root.application.action("file_open")
    readonly property var previousContainerQAction: root.application.action("go_previous_archive")
    readonly property var nextContainerQAction: root.application.action("go_next_archive")
    readonly property var previousImageQAction: root.application.action("go_previous_image")
    readonly property var nextImageQAction: root.application.action("go_next_image")
    readonly property var firstImageQAction: root.application.action("go_first_image")
    readonly property var lastImageQAction: root.application.action("go_last_image")
    readonly property var fitQAction: root.application.action("view_fit")
    readonly property var fitHeightQAction: root.application.action("view_fit_height")
    readonly property var fitWidthQAction: root.application.action("view_fit_width")
    readonly property var actualSizeQAction: root.application.action("view_actual_size")
    readonly property var zoomInQAction: root.application.action("view_zoom_in")
    readonly property var zoomOutQAction: root.application.action("view_zoom_out")
    readonly property var scanForwardQAction: root.application.action("view_scan_forward")
    readonly property var scanBackwardQAction: root.application.action("view_scan_backward")
    readonly property var fullscreenQAction: root.application.action("window_fullscreen")
    readonly property var shortcutHelpQAction: root.application.action("help_shortcuts")
    readonly property var configureQAction: root.application.action("options_configure")
    readonly property var configureShortcutsQAction: root.application.action("options_configure_keybinding")
    readonly property var showMenubarQAction: root.application.action("options_show_menubar")
    readonly property var quitQAction: root.application.action("file_quit")

    readonly property ActionProxy openAction: ActionProxy {
        sourceAction: root.openQAction
    }
    readonly property ActionProxy previousContainerAction: ActionProxy {
        sourceAction: root.previousContainerQAction
    }
    readonly property ActionProxy nextContainerAction: ActionProxy {
        sourceAction: root.nextContainerQAction
    }
    readonly property ActionProxy previousImageAction: ActionProxy {
        enabledOverride: root.canOpenPreviousImage && !root.helpDialogOpen
        sourceAction: root.previousImageQAction
    }
    readonly property ActionProxy nextImageAction: ActionProxy {
        enabledOverride: root.canOpenNextImage && !root.helpDialogOpen
        sourceAction: root.nextImageQAction
    }
    readonly property ActionProxy firstImageAction: ActionProxy {
        enabledOverride: root.canUsePageActions
        sourceAction: root.firstImageQAction
    }
    readonly property ActionProxy lastImageAction: ActionProxy {
        enabledOverride: root.canUsePageActions
        sourceAction: root.lastImageQAction
    }
    readonly property ActionProxy fitAction: ActionProxy {
        sourceAction: root.fitQAction
    }
    readonly property ActionProxy fitHeightAction: ActionProxy {
        sourceAction: root.fitHeightQAction
    }
    readonly property ActionProxy fitWidthAction: ActionProxy {
        sourceAction: root.fitWidthQAction
    }
    readonly property ActionProxy actualSizeAction: ActionProxy {
        sourceAction: root.actualSizeQAction
    }
    readonly property ActionProxy zoomInAction: ActionProxy {
        sourceAction: root.zoomInQAction
    }
    readonly property ActionProxy zoomOutAction: ActionProxy {
        sourceAction: root.zoomOutQAction
    }
    readonly property ActionProxy scanForwardAction: ActionProxy {
        sourceAction: root.scanForwardQAction
    }
    readonly property ActionProxy scanBackwardAction: ActionProxy {
        sourceAction: root.scanBackwardQAction
    }
    readonly property ActionProxy fullscreenAction: ActionProxy {
        sourceAction: root.fullscreenQAction
    }
    readonly property ActionProxy shortcutHelpAction: ActionProxy {
        sourceAction: root.shortcutHelpQAction
    }
    readonly property ActionProxy configureAction: ActionProxy {
        sourceAction: root.configureQAction
    }
    readonly property ActionProxy configureShortcutsAction: ActionProxy {
        sourceAction: root.configureShortcutsQAction
    }
    readonly property ActionProxy showMenubarAction: ActionProxy {
        sourceAction: root.showMenubarQAction
    }
    readonly property ActionProxy quitAction: ActionProxy {
        sourceAction: root.quitQAction
    }

    readonly property list<Kirigami.Action> hamburgerActions: [openAction, previousContainerAction, nextContainerAction, previousImageAction, nextImageAction, firstImageAction, lastImageAction, fitAction, fitHeightAction, fitWidthAction, actualSizeAction, zoomInAction, zoomOutAction, fullscreenAction, showMenubarSeparator, showMenubarAction, configureAction, configureShortcutsAction, shortcutHelpAction]

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

    property Kirigami.Action showMenubarSeparator: Kirigami.Action {
        separator: true
    }

    Binding {
        property: "enabled"
        target: root.openQAction
        value: !root.helpDialogOpen
        when: root.openQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.previousContainerQAction
        value: root.imageDocument.containerNavigationAvailable && !root.helpDialogOpen
        when: root.previousContainerQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.nextContainerQAction
        value: root.imageDocument.containerNavigationAvailable && !root.helpDialogOpen
        when: root.nextContainerQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.previousImageQAction
        value: root.canUseReadyActions
        when: root.previousImageQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.nextImageQAction
        value: root.canUseReadyActions
        when: root.nextImageQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.firstImageQAction
        value: root.canUsePageActions
        when: root.firstImageQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.lastImageQAction
        value: root.canUsePageActions
        when: root.lastImageQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.fitQAction
        value: root.canUseReadyActions
        when: root.fitQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.fitHeightQAction
        value: root.canUseReadyActions
        when: root.fitHeightQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.fitWidthQAction
        value: root.canUseReadyActions
        when: root.fitWidthQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.actualSizeQAction
        value: root.canUseReadyActions
        when: root.actualSizeQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.zoomInQAction
        value: root.canUseReadyActions
        when: root.zoomInQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.zoomOutQAction
        value: root.canUseReadyActions
        when: root.zoomOutQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.scanForwardQAction
        value: root.canUseReadyActions
        when: root.scanForwardQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.scanBackwardQAction
        value: root.canUseReadyActions
        when: root.scanBackwardQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.fullscreenQAction
        value: !root.helpDialogOpen
        when: root.fullscreenQAction !== null
    }

    Binding {
        property: "checked"
        target: root.fullscreenQAction
        value: root.fullscreen
        when: root.fullscreenQAction !== null
    }

    Binding {
        property: "enabled"
        target: root.shortcutHelpQAction
        value: !root.helpDialogOpen
        when: root.shortcutHelpQAction !== null
    }

    Connections {
        target: root.openQAction

        function onTriggered() {
            root.openDialogRequested();
        }
    }

    Connections {
        target: root.previousContainerQAction

        function onTriggered() {
            root.imageDocument.openPreviousContainer();
        }
    }

    Connections {
        target: root.nextContainerQAction

        function onTriggered() {
            root.imageDocument.openNextContainer();
        }
    }

    Connections {
        target: root.previousImageQAction

        function onTriggered() {
            root.openPreviousImage();
        }
    }

    Connections {
        target: root.nextImageQAction

        function onTriggered() {
            root.openNextImage();
        }
    }

    Connections {
        target: root.firstImageQAction

        function onTriggered() {
            root.openFirstImage();
        }
    }

    Connections {
        target: root.lastImageQAction

        function onTriggered() {
            root.openLastImage();
        }
    }

    Connections {
        target: root.fitQAction

        function onTriggered() {
            root.imageDocument.resetZoom();
        }
    }

    Connections {
        target: root.fitHeightQAction

        function onTriggered() {
            root.imageDocument.setFitMode(KiriImageDocument.FitHeight);
        }
    }

    Connections {
        target: root.fitWidthQAction

        function onTriggered() {
            root.imageDocument.setFitMode(KiriImageDocument.FitWidth);
        }
    }

    Connections {
        target: root.actualSizeQAction

        function onTriggered() {
            root.imageDocument.zoomPercent = 100;
        }
    }

    Connections {
        target: root.fullscreenQAction

        function onTriggered() {
            root.toggleFullScreenRequested();
        }
    }

    Connections {
        target: root.shortcutHelpQAction

        function onTriggered() {
            root.shortcutHelpRequested();
        }
    }
}
