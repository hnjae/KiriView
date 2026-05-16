// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

Item {
    id: root

    required property KiriViewApplication application
    required property KiriImageDocument imageDocument
    required property bool imageReady
    required property bool helpDialogOpen
    required property bool fullscreen

    readonly property bool canOpenNextImage: root.imageDocument.currentPageNumber > 0 && root.imageDocument.currentLastPageNumber < root.imageDocument.imageCount
    readonly property bool canOpenPreviousImage: root.imageDocument.currentPageNumber > 1
    readonly property bool atKnownFirstImage: root.imageDocument.imageCount > 0 && root.imageDocument.currentPageNumber === 1
    readonly property bool atKnownLastImage: root.imageDocument.imageCount > 0 && root.imageDocument.currentPageNumber > 0 && root.imageDocument.currentLastPageNumber > 0 && root.imageDocument.currentLastPageNumber >= root.imageDocument.imageCount
    readonly property bool canUsePageActions: root.imageDocument.imageCount > 0 && root.imageDocument.currentPageNumber > 0 && !root.imageDocument.fileDeletionInProgress && !root.helpDialogOpen
    readonly property bool canUseReadyActions: root.imageReady && !root.imageDocument.fileDeletionInProgress && !root.helpDialogOpen
    readonly property bool canUseRotateActions: root.canUseReadyActions && !(root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable)
    readonly property bool canUseTwoPageActions: root.canUsePageActions && root.imageDocument.twoPageModeAvailable && root.imageDocument.twoPageModeEnabled
    readonly property bool canUseRightToLeftReadingActions: root.canUseReadyActions && root.imageDocument.rightToLeftReadingAvailable

    readonly property var openAction: openManagedAction.proxy
    readonly property var moveToTrashAction: moveToTrashManagedAction.proxy
    readonly property var deleteFileAction: deleteFileManagedAction.proxy
    readonly property var previousContainerAction: previousContainerManagedAction.proxy
    readonly property var nextContainerAction: nextContainerManagedAction.proxy
    readonly property var previousImageAction: previousImageManagedAction.proxy
    readonly property var nextImageAction: nextImageManagedAction.proxy
    readonly property var previousSinglePageAction: previousSinglePageManagedAction.proxy
    readonly property var nextSinglePageAction: nextSinglePageManagedAction.proxy
    readonly property var firstImageAction: firstImageManagedAction.proxy
    readonly property var lastImageAction: lastImageManagedAction.proxy
    readonly property var fitAction: fitManagedAction.proxy
    readonly property var fitHeightAction: fitHeightManagedAction.proxy
    readonly property var fitWidthAction: fitWidthManagedAction.proxy
    readonly property var actualSizeAction: actualSizeManagedAction.proxy
    readonly property var rotateClockwiseAction: rotateClockwiseManagedAction.proxy
    readonly property var rotateCounterclockwiseAction: rotateCounterclockwiseManagedAction.proxy
    readonly property var twoPageModeAction: twoPageModeManagedAction.proxy
    readonly property var rightToLeftReadingAction: rightToLeftReadingManagedAction.proxy
    readonly property var zoomInAction: zoomInManagedAction.proxy
    readonly property var zoomOutAction: zoomOutManagedAction.proxy
    readonly property var scanForwardAction: scanForwardManagedAction.proxy
    readonly property var scanBackwardAction: scanBackwardManagedAction.proxy
    readonly property var fullscreenAction: fullscreenManagedAction.proxy
    readonly property var shortcutHelpAction: shortcutHelpManagedAction.proxy
    readonly property var configureShortcutsAction: configureShortcutsManagedAction.proxy
    readonly property var showMenubarAction: showMenubarManagedAction.proxy
    readonly property var quitAction: quitManagedAction.proxy
    readonly property var applicationMenuActions: [openManagedAction.menuProxy, applicationMenuFileSeparator, moveToTrashManagedAction.menuProxy, deleteFileManagedAction.menuProxy, applicationMenuNavigationSeparator, previousContainerManagedAction.menuProxy, nextContainerManagedAction.menuProxy, applicationMenuViewSeparator, rotateClockwiseManagedAction.menuProxy, rotateCounterclockwiseManagedAction.menuProxy, twoPageModeManagedAction.menuProxy, rightToLeftReadingManagedAction.menuProxy, fullscreenManagedAction.menuProxy, applicationMenuSettingsSeparator, showMenubarManagedAction.menuProxy, configureShortcutsManagedAction.menuProxy, applicationMenuHelpSeparator, shortcutHelpManagedAction.menuProxy, applicationMenuQuitSeparator, quitManagedAction.menuProxy]

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
        if (root.atKnownLastImage) {
            root.imageBoundaryReached(KI18n.i18nc("@info:status", "Last image"));
            return;
        }

        root.imageDocument.openNextImage();
    }

    function openPreviousImage() {
        if (root.atKnownFirstImage) {
            root.imageBoundaryReached(KI18n.i18nc("@info:status", "First image"));
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

    property Kirigami.Action applicationMenuNavigationSeparator: Kirigami.Action {
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
        actionId: KiriViewApplication.FileOpenAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Open")

        onTriggered: root.openDialogRequested()
    }

    ManagedAction {
        id: moveToTrashManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.FileMoveToTrashAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Move to &Trash")

        onTriggered: root.imageDocument.deleteDisplayedFile(KiriImageDocument.MoveToTrash)
    }

    ManagedAction {
        id: deleteFileManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.FileDeleteAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Delete Permanently")

        onTriggered: root.imageDocument.deleteDisplayedFile(KiriImageDocument.DeletePermanently)
    }

    ManagedAction {
        id: previousContainerManagedAction

        actionEnabled: root.imageDocument.containerNavigationAvailable && !root.imageDocument.fileDeletionInProgress && !root.helpDialogOpen
        actionId: KiriViewApplication.GoPreviousArchiveAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Previous Archive")

        onTriggered: root.imageDocument.openPreviousContainer()
    }

    ManagedAction {
        id: nextContainerManagedAction

        actionEnabled: root.imageDocument.containerNavigationAvailable && !root.imageDocument.fileDeletionInProgress && !root.helpDialogOpen
        actionId: KiriViewApplication.GoNextArchiveAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Next Archive")

        onTriggered: root.imageDocument.openNextContainer()
    }

    ManagedAction {
        id: previousImageManagedAction

        actionEnabled: root.canUsePageActions
        actionId: KiriViewApplication.GoPreviousImageAction
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canUsePageActions && root.canOpenPreviousImage

        onTriggered: root.openPreviousImage()
    }

    ManagedAction {
        id: nextImageManagedAction

        actionEnabled: root.canUsePageActions
        actionId: KiriViewApplication.GoNextImageAction
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canUsePageActions && root.canOpenNextImage

        onTriggered: root.openNextImage()
    }

    ManagedAction {
        id: previousSinglePageManagedAction

        actionEnabled: root.canUseTwoPageActions
        actionId: KiriViewApplication.GoPreviousSinglePageAction
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canUseTwoPageActions && root.canOpenPreviousImage

        onTriggered: root.imageDocument.openPreviousSinglePage()
    }

    ManagedAction {
        id: nextSinglePageManagedAction

        actionEnabled: root.canUseTwoPageActions
        actionId: KiriViewApplication.GoNextSinglePageAction
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canUseTwoPageActions && root.imageDocument.currentPageNumber < root.imageDocument.imageCount

        onTriggered: root.imageDocument.openNextSinglePage()
    }

    ManagedAction {
        id: firstImageManagedAction

        actionEnabled: root.canUsePageActions
        actionId: KiriViewApplication.GoFirstImageAction
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canUsePageActions

        onTriggered: root.openFirstImage()
    }

    ManagedAction {
        id: lastImageManagedAction

        actionEnabled: root.canUsePageActions
        actionId: KiriViewApplication.GoLastImageAction
        application: root.application
        bindEnabled: true
        proxyEnabled: root.canUsePageActions

        onTriggered: root.openLastImage()
    }

    ManagedAction {
        id: fitManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.ViewFitAction
        application: root.application
        bindEnabled: true
        proxyCheckable: true
        proxyChecked: root.imageDocument.zoomMode === KiriImageDocument.Fit

        onTriggered: root.imageDocument.resetZoom()
    }

    ManagedAction {
        id: fitHeightManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.ViewFitHeightAction
        application: root.application
        bindEnabled: true
        proxyCheckable: true
        proxyChecked: root.imageDocument.zoomMode === KiriImageDocument.FitHeight

        onTriggered: root.imageDocument.setFitMode(KiriImageDocument.FitHeight)
    }

    ManagedAction {
        id: fitWidthManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.ViewFitWidthAction
        application: root.application
        bindEnabled: true
        proxyCheckable: true
        proxyChecked: root.imageDocument.zoomMode === KiriImageDocument.FitWidth

        onTriggered: root.imageDocument.setFitMode(KiriImageDocument.FitWidth)
    }

    ManagedAction {
        id: actualSizeManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.ViewActualSizeAction
        application: root.application
        bindEnabled: true

        onTriggered: root.imageDocument.zoomPercent = 100
    }

    ManagedAction {
        id: rotateClockwiseManagedAction

        actionEnabled: root.canUseRotateActions
        actionId: KiriViewApplication.ViewRotateClockwiseAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Rotate &Clockwise")

        onTriggered: root.imageDocument.rotateClockwise()
    }

    ManagedAction {
        id: rotateCounterclockwiseManagedAction

        actionEnabled: root.canUseRotateActions
        actionId: KiriViewApplication.ViewRotateCounterclockwiseAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Rotate C&ounterclockwise")

        onTriggered: root.imageDocument.rotateCounterclockwise()
    }

    ManagedAction {
        id: twoPageModeManagedAction

        actionChecked: root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable
        actionEnabled: root.canUseReadyActions && root.imageDocument.twoPageModeAvailable
        actionId: KiriViewApplication.ViewToggleTwoPageModeAction
        application: root.application
        bindChecked: true
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.KeepVisible
        menuText: KI18n.i18nc("@action:inmenu", "Two-Page &Spread")
        proxyCheckable: true
        proxyChecked: root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable

        onTriggered: root.imageDocument.twoPageModeEnabled = !root.imageDocument.twoPageModeEnabled
    }

    ManagedAction {
        id: rightToLeftReadingManagedAction

        actionChecked: root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable
        actionEnabled: root.canUseRightToLeftReadingActions
        actionId: KiriViewApplication.ViewToggleRightToLeftReadingAction
        application: root.application
        bindChecked: true
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.KeepVisible
        menuText: KI18n.i18nc("@action:inmenu", "&Right-to-Left Reading")
        proxyCheckable: true
        proxyChecked: root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable

        onTriggered: root.imageDocument.rightToLeftReadingEnabled = !root.imageDocument.rightToLeftReadingEnabled
    }

    ManagedAction {
        id: zoomInManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.ViewZoomInAction
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: zoomOutManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.ViewZoomOutAction
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: scanForwardManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.ViewScanForwardAction
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: scanBackwardManagedAction

        actionEnabled: root.canUseReadyActions
        actionId: KiriViewApplication.ViewScanBackwardAction
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: fullscreenManagedAction

        actionChecked: root.fullscreen
        actionEnabled: !root.helpDialogOpen
        actionId: KiriViewApplication.WindowFullscreenAction
        application: root.application
        bindChecked: true
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Fullscreen")
        proxyCheckable: true
        proxyChecked: root.fullscreen

        onTriggered: root.toggleFullScreenRequested()
    }

    ManagedAction {
        id: shortcutHelpManagedAction

        actionEnabled: !root.helpDialogOpen
        actionId: KiriViewApplication.HelpShortcutsAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Keyboard Shortcuts")

        onTriggered: root.shortcutHelpRequested()
    }

    ManagedAction {
        id: configureShortcutsManagedAction

        actionId: KiriViewApplication.OptionsConfigureKeybindingAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Configure Shortcuts...")
    }

    ManagedAction {
        id: showMenubarManagedAction

        actionId: KiriViewApplication.OptionsShowMenubarAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Show Menubar")
    }

    ManagedAction {
        id: quitManagedAction

        actionId: KiriViewApplication.FileQuitAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Quit")
    }
}
