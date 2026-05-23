// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import io.github.hnjae.kiriview
import org.kde.ki18n
import org.kde.kirigami as Kirigami

Item {
    id: root

    required property KiriViewApplication application
    required property ImageActionAvailability actionAvailability
    required property KiriDocumentSession documentSession
    required property KiriImageDocument imageDocument
    required property bool fullscreen

    readonly property var openAction: openManagedAction.proxy
    readonly property var moveToTrashAction: moveToTrashManagedAction.proxy
    readonly property var deleteFileAction: deleteFileManagedAction.proxy
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
    readonly property var openMenuAction: openManagedAction.menuProxy
    readonly property var moveToTrashMenuAction: moveToTrashManagedAction.menuProxy
    readonly property var deleteFileMenuAction: deleteFileManagedAction.menuProxy
    readonly property var previousContainerMenuAction: previousContainerManagedAction.menuProxy
    readonly property var nextContainerMenuAction: nextContainerManagedAction.menuProxy
    readonly property var previousImageMenuAction: previousImageManagedAction.menuProxy
    readonly property var nextImageMenuAction: nextImageManagedAction.menuProxy
    readonly property var firstImageMenuAction: firstImageManagedAction.menuProxy
    readonly property var lastImageMenuAction: lastImageManagedAction.menuProxy
    readonly property var fitMenuAction: fitManagedAction.menuProxy
    readonly property var fitHeightMenuAction: fitHeightManagedAction.menuProxy
    readonly property var fitWidthMenuAction: fitWidthManagedAction.menuProxy
    readonly property var actualSizeMenuAction: actualSizeManagedAction.menuProxy
    readonly property var rotateClockwiseMenuAction: rotateClockwiseManagedAction.menuProxy
    readonly property var rotateCounterclockwiseMenuAction: rotateCounterclockwiseManagedAction.menuProxy
    readonly property var twoPageModeMenuAction: twoPageModeManagedAction.menuProxy
    readonly property var rightToLeftReadingMenuAction: rightToLeftReadingManagedAction.menuProxy
    readonly property var zoomInMenuAction: zoomInManagedAction.menuProxy
    readonly property var zoomOutMenuAction: zoomOutManagedAction.menuProxy
    readonly property var fullscreenMenuAction: fullscreenManagedAction.menuProxy
    readonly property var shortcutHelpMenuAction: shortcutHelpManagedAction.menuProxy
    readonly property var configureShortcutsMenuAction: configureShortcutsManagedAction.menuProxy
    readonly property var showMenubarMenuAction: showMenubarManagedAction.menuProxy
    readonly property var quitMenuAction: quitManagedAction.menuProxy
    readonly property bool imageMode: root.documentSession.documentKind === KiriDocumentSession.Image
    readonly property bool videoMode: root.documentSession.documentKind === KiriDocumentSession.Video
    readonly property bool documentDeletionAvailable: root.documentSession.displayedFileDeletionAvailable && root.actionAvailability.helpShortcutsEnabled
    readonly property bool activeNavigationActionsAvailable: root.documentSession.activeNavigationAvailable && root.documentSession.activeNavigationKnown && !root.documentSession.fileDeletionInProgress && root.actionAvailability.helpShortcutsEnabled
    readonly property bool previousPageActionAvailable: root.activeNavigationActionsAvailable
    readonly property bool nextPageActionAvailable: root.activeNavigationActionsAvailable
    readonly property bool previousPageProxyAvailable: root.activeNavigationActionsAvailable && root.documentSession.canOpenPreviousActiveNavigation
    readonly property bool nextPageProxyAvailable: root.activeNavigationActionsAvailable && root.documentSession.canOpenNextActiveNavigation
    readonly property bool firstLastPageActionAvailable: root.activeNavigationActionsAvailable && root.documentSession.activeNavigationCount > 0
    readonly property bool rightToLeftReadingActive: root.actionAvailability.rightToLeftReadingActive
    readonly property var applicationMenuNavigationActions: root.rightToLeftReadingActive ? [nextContainerManagedAction.menuProxy, previousContainerManagedAction.menuProxy] : [previousContainerManagedAction.menuProxy, nextContainerManagedAction.menuProxy]
    readonly property var applicationMenuDocumentActions: root.imageMode || root.videoMode ? [applicationMenuNavigationSeparator, previousImageManagedAction.menuProxy, nextImageManagedAction.menuProxy, firstImageManagedAction.menuProxy, lastImageManagedAction.menuProxy] : []
    readonly property var applicationMenuImageActions: root.imageMode ? root.applicationMenuNavigationActions.concat([rotateClockwiseManagedAction.menuProxy, rotateCounterclockwiseManagedAction.menuProxy, twoPageModeManagedAction.menuProxy, rightToLeftReadingManagedAction.menuProxy]) : []
    readonly property var applicationMenuActions: [openManagedAction.menuProxy, applicationMenuFileSeparator, moveToTrashManagedAction.menuProxy, deleteFileManagedAction.menuProxy].concat(root.applicationMenuDocumentActions, root.applicationMenuImageActions, [applicationMenuViewSeparator, fullscreenManagedAction.menuProxy, applicationMenuSettingsSeparator, showMenubarManagedAction.menuProxy, configureShortcutsManagedAction.menuProxy, applicationMenuHelpSeparator, shortcutHelpManagedAction.menuProxy, applicationMenuQuitSeparator, quitManagedAction.menuProxy])

    signal openDialogRequested
    signal imageBoundaryReached(string message)
    signal shortcutHelpRequested
    signal toggleFullScreenRequested

    function firstBoundaryText() {
        switch (root.documentSession.activeNavigationBoundaryScope) {
        case KiriDocumentSession.MediaNavigationBoundary:
            return KI18n.i18nc("@info:status", "First media item");
        case KiriDocumentSession.ImageNavigationBoundary:
            return KI18n.i18nc("@info:status", "First image");
        case KiriDocumentSession.NoNavigationBoundary:
        default:
            return "";
        }
    }

    function lastBoundaryText() {
        switch (root.documentSession.activeNavigationBoundaryScope) {
        case KiriDocumentSession.MediaNavigationBoundary:
            return KI18n.i18nc("@info:status", "Last media item");
        case KiriDocumentSession.ImageNavigationBoundary:
            return KI18n.i18nc("@info:status", "Last image");
        case KiriDocumentSession.NoNavigationBoundary:
        default:
            return "";
        }
    }

    function activeFirstMenuText() {
        return root.documentSession.activeNavigationBoundaryScope === KiriDocumentSession.MediaNavigationBoundary ? KI18n.i18nc("@action:inmenu", "&First Media Item") : "";
    }

    function activeLastMenuText() {
        return root.documentSession.activeNavigationBoundaryScope === KiriDocumentSession.MediaNavigationBoundary ? KI18n.i18nc("@action:inmenu", "&Last Media Item") : "";
    }

    function openFirstImage() {
        root.documentSession.openFirstActiveNavigation();
    }

    function openLastImage() {
        root.documentSession.openLastActiveNavigation();
    }

    function openNextImage() {
        if (root.documentSession.atKnownLastActiveNavigation) {
            root.imageBoundaryReached(lastBoundaryText());
            return;
        }

        root.documentSession.openNextActiveNavigation();
    }

    function openPreviousImage() {
        if (root.documentSession.atKnownFirstActiveNavigation) {
            root.imageBoundaryReached(firstBoundaryText());
            return;
        }

        root.documentSession.openPreviousActiveNavigation();
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

        actionEnabled: root.actionAvailability.helpShortcutsEnabled
        actionId: KiriViewApplication.FileOpenAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Open")

        onTriggered: root.openDialogRequested()
    }

    ManagedAction {
        id: moveToTrashManagedAction

        actionEnabled: root.documentDeletionAvailable
        actionId: KiriViewApplication.FileMoveToTrashAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Move to &Trash")

        onTriggered: root.documentSession.deleteDisplayedFile(KiriDocumentSession.MoveToTrash)
    }

    ManagedAction {
        id: deleteFileManagedAction

        actionEnabled: root.documentDeletionAvailable
        actionId: KiriViewApplication.FileDeleteAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Delete Permanently")

        onTriggered: root.documentSession.deleteDisplayedFile(KiriDocumentSession.DeletePermanently)
    }

    ManagedAction {
        id: previousContainerManagedAction

        actionEnabled: root.actionAvailability.containerShortcutsEnabled
        actionId: KiriViewApplication.GoPreviousArchiveAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Previous Archive")

        onTriggered: root.imageDocument.openPreviousContainer()
    }

    ManagedAction {
        id: nextContainerManagedAction

        actionEnabled: root.actionAvailability.containerShortcutsEnabled
        actionId: KiriViewApplication.GoNextArchiveAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Next Archive")

        onTriggered: root.imageDocument.openNextContainer()
    }

    ManagedAction {
        id: previousImageManagedAction

        actionEnabled: root.previousPageActionAvailable
        actionId: KiriViewApplication.GoPreviousImageAction
        application: root.application
        bindEnabled: true
        proxyEnabled: root.previousPageProxyAvailable

        onTriggered: root.openPreviousImage()
    }

    ManagedAction {
        id: nextImageManagedAction

        actionEnabled: root.nextPageActionAvailable
        actionId: KiriViewApplication.GoNextImageAction
        application: root.application
        bindEnabled: true
        proxyEnabled: root.nextPageProxyAvailable

        onTriggered: root.openNextImage()
    }

    ManagedAction {
        id: firstImageManagedAction

        actionEnabled: root.firstLastPageActionAvailable
        actionId: KiriViewApplication.GoFirstImageAction
        application: root.application
        bindEnabled: true
        menuText: root.activeFirstMenuText()
        proxyEnabled: root.firstLastPageActionAvailable

        onTriggered: root.openFirstImage()
    }

    ManagedAction {
        id: lastImageManagedAction

        actionEnabled: root.firstLastPageActionAvailable
        actionId: KiriViewApplication.GoLastImageAction
        application: root.application
        bindEnabled: true
        menuText: root.activeLastMenuText()
        proxyEnabled: root.firstLastPageActionAvailable

        onTriggered: root.openLastImage()
    }

    ManagedAction {
        id: fitManagedAction

        actionEnabled: root.actionAvailability.canUseReadyActions
        actionId: KiriViewApplication.ViewFitAction
        application: root.application
        bindEnabled: true
        proxyCheckable: true
        proxyChecked: root.imageDocument !== null && root.imageDocument.zoomMode === KiriImageDocument.Fit

        onTriggered: root.imageDocument.resetZoom()
    }

    ManagedAction {
        id: fitHeightManagedAction

        actionEnabled: root.actionAvailability.canUseReadyActions
        actionId: KiriViewApplication.ViewFitHeightAction
        application: root.application
        bindEnabled: true
        proxyCheckable: true
        proxyChecked: root.imageDocument !== null && root.imageDocument.zoomMode === KiriImageDocument.FitHeight

        onTriggered: root.imageDocument.setFitMode(KiriImageDocument.FitHeight)
    }

    ManagedAction {
        id: fitWidthManagedAction

        actionEnabled: root.actionAvailability.canUseReadyActions
        actionId: KiriViewApplication.ViewFitWidthAction
        application: root.application
        bindEnabled: true
        proxyCheckable: true
        proxyChecked: root.imageDocument !== null && root.imageDocument.zoomMode === KiriImageDocument.FitWidth

        onTriggered: root.imageDocument.setFitMode(KiriImageDocument.FitWidth)
    }

    ManagedAction {
        id: actualSizeManagedAction

        actionEnabled: root.actionAvailability.canUseReadyActions
        actionId: KiriViewApplication.ViewActualSizeAction
        application: root.application
        bindEnabled: true

        onTriggered: root.imageDocument.zoomPercent = 100
    }

    ManagedAction {
        id: rotateClockwiseManagedAction

        actionEnabled: root.actionAvailability.canUseRotateActions
        actionId: KiriViewApplication.ViewRotateClockwiseAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Rotate &Clockwise")

        onTriggered: root.imageDocument.rotateClockwise()
    }

    ManagedAction {
        id: rotateCounterclockwiseManagedAction

        actionEnabled: root.actionAvailability.canUseRotateActions
        actionId: KiriViewApplication.ViewRotateCounterclockwiseAction
        application: root.application
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Rotate C&ounterclockwise")

        onTriggered: root.imageDocument.rotateCounterclockwise()
    }

    ManagedAction {
        id: twoPageModeManagedAction

        actionChecked: root.actionAvailability.twoPageModeActive
        actionEnabled: root.actionAvailability.canUseTwoPageModeActions
        actionId: KiriViewApplication.ViewToggleTwoPageModeAction
        application: root.application
        bindChecked: true
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.KeepVisible
        menuText: KI18n.i18nc("@action:inmenu", "Two-Page &Spread")
        proxyCheckable: true
        proxyChecked: root.actionAvailability.twoPageModeActive

        onTriggered: root.imageDocument.twoPageModeEnabled = !root.imageDocument.twoPageModeEnabled
    }

    ManagedAction {
        id: rightToLeftReadingManagedAction

        actionChecked: root.actionAvailability.rightToLeftReadingActive
        actionEnabled: root.actionAvailability.canUseRightToLeftReadingActions
        actionId: KiriViewApplication.ViewToggleRightToLeftReadingAction
        application: root.application
        bindChecked: true
        bindEnabled: true
        displayHint: Kirigami.DisplayHint.KeepVisible
        menuText: KI18n.i18nc("@action:inmenu", "&Right-to-Left Reading")
        proxyCheckable: true
        proxyChecked: root.actionAvailability.rightToLeftReadingActive

        onTriggered: root.imageDocument.rightToLeftReadingEnabled = !root.imageDocument.rightToLeftReadingEnabled
    }

    ManagedAction {
        id: zoomInManagedAction

        actionEnabled: root.actionAvailability.canUseReadyActions
        actionId: KiriViewApplication.ViewZoomInAction
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: zoomOutManagedAction

        actionEnabled: root.actionAvailability.canUseReadyActions
        actionId: KiriViewApplication.ViewZoomOutAction
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: scanForwardManagedAction

        actionEnabled: root.actionAvailability.canUseReadyActions
        actionId: KiriViewApplication.ViewScanForwardAction
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: scanBackwardManagedAction

        actionEnabled: root.actionAvailability.canUseReadyActions
        actionId: KiriViewApplication.ViewScanBackwardAction
        application: root.application
        bindEnabled: true
    }

    ManagedAction {
        id: fullscreenManagedAction

        actionChecked: root.fullscreen
        actionEnabled: root.actionAvailability.helpShortcutsEnabled
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

        actionEnabled: root.actionAvailability.helpShortcutsEnabled
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
        fixedShortcutText: "Ctrl+M"
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
