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
    property bool applicationMenuShortcutEnabled: false
    property bool infoPanelVisible: false
    property bool showMenubarActionEnabled: true
    property bool thumbnailPanelVisible: false

    readonly property var openAction: openManagedAction.proxy
    readonly property var openWithAction: openWithManagedAction.proxy
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
    readonly property var infoPanelAction: infoPanelManagedAction.proxy
    readonly property var thumbnailPanelAction: thumbnailPanelManagedAction.proxy
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
    readonly property var openWithMenuAction: openWithManagedAction.menuProxy
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
    readonly property var infoPanelMenuAction: infoPanelManagedAction.menuProxy
    readonly property var thumbnailPanelMenuAction: thumbnailPanelManagedAction.menuProxy
    readonly property var zoomInMenuAction: zoomInManagedAction.menuProxy
    readonly property var zoomOutMenuAction: zoomOutManagedAction.menuProxy
    readonly property var fullscreenMenuAction: fullscreenManagedAction.menuProxy
    readonly property var shortcutHelpMenuAction: shortcutHelpManagedAction.menuProxy
    readonly property var configureShortcutsMenuAction: configureShortcutsManagedAction.menuProxy
    readonly property var showMenubarMenuAction: showMenubarManagedAction.menuProxy
    readonly property var quitMenuAction: quitManagedAction.menuProxy
    readonly property bool imageMode: root.documentSession.documentKind === KiriDocumentSession.Image
    readonly property bool videoMode: root.documentSession.documentKind === KiriDocumentSession.Video
    readonly property bool rightToLeftReadingActive: root.actionAvailability.rightToLeftReadingActive
    readonly property var applicationMenuNavigationActions: root.rightToLeftReadingActive ? [nextContainerManagedAction.menuProxy, previousContainerManagedAction.menuProxy] : [previousContainerManagedAction.menuProxy, nextContainerManagedAction.menuProxy]
    readonly property var applicationMenuDocumentActions: root.imageMode || root.videoMode ? [applicationMenuNavigationSeparator, previousImageManagedAction.menuProxy, nextImageManagedAction.menuProxy, firstImageManagedAction.menuProxy, lastImageManagedAction.menuProxy] : []
    readonly property var applicationMenuImageActions: root.imageMode ? root.applicationMenuNavigationActions.concat([rotateClockwiseManagedAction.menuProxy, rotateCounterclockwiseManagedAction.menuProxy, twoPageModeManagedAction.menuProxy, rightToLeftReadingManagedAction.menuProxy]) : []
    readonly property var applicationMenuActions: [openManagedAction.menuProxy, openWithManagedAction.menuProxy, applicationMenuFileSeparator, moveToTrashManagedAction.menuProxy, deleteFileManagedAction.menuProxy].concat(root.applicationMenuDocumentActions, root.applicationMenuImageActions, [applicationMenuViewSeparator, infoPanelManagedAction.menuProxy, thumbnailPanelManagedAction.menuProxy, fullscreenManagedAction.menuProxy, applicationMenuSettingsSeparator, showMenubarManagedAction.menuProxy, configureShortcutsManagedAction.menuProxy, applicationMenuHelpSeparator, shortcutHelpManagedAction.menuProxy, applicationMenuQuitSeparator, quitManagedAction.menuProxy])
    readonly property var contextMenuActions: [openManagedAction.menuProxy, openWithManagedAction.menuProxy, contextMenuNavigationSeparator, previousImageManagedAction.menuProxy, nextImageManagedAction.menuProxy, firstImageManagedAction.menuProxy, lastImageManagedAction.menuProxy, contextMenuImageSeparator, rotateClockwiseManagedAction.menuProxy, rotateCounterclockwiseManagedAction.menuProxy, zoomInManagedAction.menuProxy, zoomOutManagedAction.menuProxy, fitManagedAction.menuProxy, fitHeightManagedAction.menuProxy, fitWidthManagedAction.menuProxy, actualSizeManagedAction.menuProxy, contextMenuViewSeparator, infoPanelManagedAction.menuProxy, thumbnailPanelManagedAction.menuProxy, fullscreenManagedAction.menuProxy]

    signal openDialogRequested
    signal imageBoundaryReached(string message)
    signal shortcutHelpRequested
    signal toggleFullScreenRequested
    signal toggleInfoPanelRequested
    signal toggleThumbnailPanelRequested

    function publishActionState() {
        const zoomMode = root.imageDocument !== null && root.imageDocument !== undefined ? root.imageDocument.zoomMode : -1;
        root.application.updateActionState(root.actionAvailability.helpShortcutsEnabled, root.actionAvailability.canUseReadyActions, root.actionAvailability.canUseRotateActions, root.actionAvailability.canUseTwoPageModeActions, root.actionAvailability.canUseRightToLeftReadingActions, root.actionAvailability.containerShortcutsEnabled, root.documentSession.displayedMediaOpenWithAvailable, root.documentSession.displayedFileDeletionAvailable, root.documentSession.fileDeletionInProgress, root.documentSession.activeNavigationAvailable, root.documentSession.activeNavigationKnown, root.documentSession.activeNavigationCount > 0, root.documentSession.canOpenPreviousActiveNavigation, root.documentSession.canOpenNextActiveNavigation, root.imageMode && zoomMode === KiriImageDocument.Fit, root.imageMode && zoomMode === KiriImageDocument.FitHeight, root.imageMode && zoomMode === KiriImageDocument.FitWidth, root.actionAvailability.twoPageModeActive, root.actionAvailability.rightToLeftReadingActive, root.infoPanelVisible, root.thumbnailPanelVisible, root.fullscreen, root.applicationMenuShortcutEnabled, root.showMenubarActionEnabled);
    }

    function activeFirstMenuText() {
        return root.documentSession.activeNavigationBoundaryScope === KiriDocumentSession.DirectMediaNavigationBoundary ? KI18n.i18nc("@action:inmenu", "&First Media Item") : "";
    }

    function activeLastMenuText() {
        return root.documentSession.activeNavigationBoundaryScope === KiriDocumentSession.DirectMediaNavigationBoundary ? KI18n.i18nc("@action:inmenu", "&Last Media Item") : "";
    }

    function openFirstImage() {
        root.documentSession.openFirstActiveNavigation();
    }

    function openLastImage() {
        root.documentSession.openLastActiveNavigation();
    }

    function openNextPage() {
        const boundaryText = root.documentSession.requestNextActiveNavigationBoundaryText();
        if (boundaryText.length > 0) {
            root.imageBoundaryReached(boundaryText);
        }
    }

    function openPreviousPage() {
        const boundaryText = root.documentSession.requestPreviousActiveNavigationBoundaryText();
        if (boundaryText.length > 0) {
            root.imageBoundaryReached(boundaryText);
        }
    }

    Component.onCompleted: root.publishActionState()

    onApplicationMenuShortcutEnabledChanged: root.publishActionState()
    onFullscreenChanged: root.publishActionState()
    onInfoPanelVisibleChanged: root.publishActionState()
    onShowMenubarActionEnabledChanged: root.publishActionState()
    onThumbnailPanelVisibleChanged: root.publishActionState()

    Connections {
        target: root.actionAvailability

        function onAvailabilityChanged() {
            root.publishActionState();
        }
    }

    Connections {
        target: root.documentSession

        function onActiveNavigationChanged() {
            root.publishActionState();
        }

        function onDisplayedFileDeletionAvailabilityChanged() {
            root.publishActionState();
        }

        function onDisplayedMediaOpenWithAvailabilityChanged() {
            root.publishActionState();
        }

        function onDocumentKindChanged() {
            root.publishActionState();
        }

        function onFileDeletionInProgressChanged() {
            root.publishActionState();
        }
    }

    Connections {
        target: root.imageDocument

        function onZoomModeChanged() {
            root.publishActionState();
        }
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

    property Kirigami.Action contextMenuNavigationSeparator: Kirigami.Action {
        displayHint: Kirigami.DisplayHint.AlwaysHide
        separator: true
    }

    property Kirigami.Action contextMenuImageSeparator: Kirigami.Action {
        displayHint: Kirigami.DisplayHint.AlwaysHide
        separator: true
    }

    property Kirigami.Action contextMenuViewSeparator: Kirigami.Action {
        displayHint: Kirigami.DisplayHint.AlwaysHide
        separator: true
    }

    ManagedAction {
        id: openManagedAction

        actionId: KiriViewApplication.FileOpenAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Open")

        onTriggered: root.openDialogRequested()
    }

    ManagedAction {
        id: openWithManagedAction

        actionId: KiriViewApplication.FileOpenWithAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Open &With...")

        onTriggered: root.documentSession.openCurrentMediaWith()
    }

    ManagedAction {
        id: moveToTrashManagedAction

        actionId: KiriViewApplication.FileMoveToTrashAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Move to &Trash")

        onTriggered: root.documentSession.deleteDisplayedFile(KiriDocumentSession.MoveToTrash)
    }

    ManagedAction {
        id: deleteFileManagedAction

        actionId: KiriViewApplication.FileDeleteAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Delete Permanently")

        onTriggered: root.documentSession.deleteDisplayedFile(KiriDocumentSession.DeletePermanently)
    }

    ManagedAction {
        id: previousContainerManagedAction

        actionId: KiriViewApplication.GoPreviousArchiveAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Previous Archive")

        onTriggered: root.imageDocument.openPreviousContainer()
    }

    ManagedAction {
        id: nextContainerManagedAction

        actionId: KiriViewApplication.GoNextArchiveAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Next Archive")

        onTriggered: root.imageDocument.openNextContainer()
    }

    ManagedAction {
        id: previousImageManagedAction

        actionId: KiriViewApplication.GoPreviousImageAction
        application: root.application

        onTriggered: root.openPreviousPage()
    }

    ManagedAction {
        id: nextImageManagedAction

        actionId: KiriViewApplication.GoNextImageAction
        application: root.application

        onTriggered: root.openNextPage()
    }

    ManagedAction {
        id: firstImageManagedAction

        actionId: KiriViewApplication.GoFirstImageAction
        application: root.application
        menuText: root.activeFirstMenuText()

        onTriggered: root.openFirstImage()
    }

    ManagedAction {
        id: lastImageManagedAction

        actionId: KiriViewApplication.GoLastImageAction
        application: root.application
        menuText: root.activeLastMenuText()

        onTriggered: root.openLastImage()
    }

    ManagedAction {
        id: fitManagedAction

        actionId: KiriViewApplication.ViewFitAction
        application: root.application

        onTriggered: root.imageDocument.resetZoom()
    }

    ManagedAction {
        id: fitHeightManagedAction

        actionId: KiriViewApplication.ViewFitHeightAction
        application: root.application

        onTriggered: root.imageDocument.setFitMode(KiriImageDocument.FitHeight)
    }

    ManagedAction {
        id: fitWidthManagedAction

        actionId: KiriViewApplication.ViewFitWidthAction
        application: root.application

        onTriggered: root.imageDocument.setFitMode(KiriImageDocument.FitWidth)
    }

    ManagedAction {
        id: actualSizeManagedAction

        actionId: KiriViewApplication.ViewActualSizeAction
        application: root.application

        onTriggered: root.imageDocument.zoomPercent = 100
    }

    ManagedAction {
        id: rotateClockwiseManagedAction

        actionId: KiriViewApplication.ViewRotateClockwiseAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Rotate &Clockwise")

        onTriggered: root.imageDocument.rotateClockwise()
    }

    ManagedAction {
        id: rotateCounterclockwiseManagedAction

        actionId: KiriViewApplication.ViewRotateCounterclockwiseAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "Rotate C&ounterclockwise")

        onTriggered: root.imageDocument.rotateCounterclockwise()
    }

    ManagedAction {
        id: twoPageModeManagedAction

        actionId: KiriViewApplication.ViewToggleTwoPageModeAction
        application: root.application
        displayHint: Kirigami.DisplayHint.KeepVisible
        menuText: KI18n.i18nc("@action:inmenu", "Two-Page &Spread")
        toolbarText: KI18n.i18nc("@action:button", "Two-Page &Spread")

        onTriggered: root.imageDocument.twoPageModeEnabled = !root.imageDocument.twoPageModeEnabled
    }

    ManagedAction {
        id: rightToLeftReadingManagedAction

        actionId: KiriViewApplication.ViewToggleRightToLeftReadingAction
        application: root.application
        displayHint: Kirigami.DisplayHint.KeepVisible
        menuText: KI18n.i18nc("@action:inmenu", "&Right-to-Left Reading")
        toolbarText: KI18n.i18nc("@action:button", "&Right-to-Left")

        onTriggered: root.imageDocument.rightToLeftReadingEnabled = !root.imageDocument.rightToLeftReadingEnabled
    }

    ManagedAction {
        id: infoPanelManagedAction

        actionId: KiriViewApplication.ViewToggleInfoPanelAction
        application: root.application
        menuText: KI18n.i18nc("@action:inmenu", "Show &Info Panel")

        onTriggered: root.toggleInfoPanelRequested()
    }

    ManagedAction {
        id: thumbnailPanelManagedAction

        actionId: KiriViewApplication.ViewToggleThumbnailPanelAction
        application: root.application
        menuText: KI18n.i18nc("@action:inmenu", "Show &Thumbnail Panel")

        onTriggered: root.toggleThumbnailPanelRequested()
    }

    ManagedAction {
        id: zoomInManagedAction

        actionId: KiriViewApplication.ViewZoomInAction
        application: root.application
    }

    ManagedAction {
        id: zoomOutManagedAction

        actionId: KiriViewApplication.ViewZoomOutAction
        application: root.application
    }

    ManagedAction {
        id: scanForwardManagedAction

        actionId: KiriViewApplication.ViewScanForwardAction
        application: root.application
    }

    ManagedAction {
        id: scanBackwardManagedAction

        actionId: KiriViewApplication.ViewScanBackwardAction
        application: root.application
    }

    ManagedAction {
        id: fullscreenManagedAction

        actionId: KiriViewApplication.WindowFullscreenAction
        application: root.application
        displayHint: Kirigami.DisplayHint.AlwaysHide
        menuText: KI18n.i18nc("@action:inmenu", "&Fullscreen")

        onTriggered: root.toggleFullScreenRequested()
    }

    ManagedAction {
        id: shortcutHelpManagedAction

        actionId: KiriViewApplication.HelpShortcutsAction
        application: root.application
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
