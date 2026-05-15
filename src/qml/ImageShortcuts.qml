// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick
import io.github.hnjae.kiriview
import org.kde.ki18n

Item {
    id: root

    required property KiriViewApplication application
    required property KiriImageDocument imageDocument
    required property var imageViewport
    required property var imageToolBar
    required property bool helpDialogOpen

    readonly property bool imageReady: imageDocument.status === KiriImageDocument.Ready
    readonly property bool imagePannable: imageViewport.imagePannable
    readonly property bool imageHorizontallyPannable: imageViewport.imageHorizontallyPannable
    readonly property bool atFirstImage: imageReady && imageDocument.currentPageNumber === 1 && imageDocument.imageCount > 0
    readonly property bool atLastImage: imageReady && imageDocument.currentPageNumber > 0 && imageDocument.currentPageNumber === imageDocument.imageCount
    readonly property int keyboardPanDistance: 64
    readonly property bool commandShortcutsEnabled: !root.textInputFocused() && !root.helpDialogOpen
    readonly property bool helpShortcutsEnabled: !root.helpDialogOpen
    readonly property bool readyShortcutsEnabled: root.imageReady && !root.imageDocument.fileDeletionInProgress && root.helpShortcutsEnabled
    readonly property bool readyCommandShortcutsEnabled: root.imageReady && !root.imageDocument.fileDeletionInProgress && root.commandShortcutsEnabled
    readonly property bool pageCommandShortcutsEnabled: readyCommandShortcutsEnabled && root.imageDocument.imageCount > 0
    readonly property bool twoPageCommandShortcutsEnabled: readyCommandShortcutsEnabled && root.imageDocument.twoPageModeAvailable && root.imageDocument.twoPageModeEnabled
    readonly property bool rightToLeftReadingCommandShortcutsEnabled: readyCommandShortcutsEnabled && root.imageDocument.rightToLeftReadingAvailable
    readonly property bool pannableCommandShortcutsEnabled: root.imagePannable && !root.imageDocument.fileDeletionInProgress && root.commandShortcutsEnabled
    readonly property bool containerCommandShortcutsEnabled: root.imageDocument.containerNavigationAvailable && !root.imageDocument.fileDeletionInProgress && root.commandShortcutsEnabled

    readonly property var previousImageQAction: root.application.actionForId(KiriViewApplication.GoPreviousImageAction)
    readonly property var nextImageQAction: root.application.actionForId(KiriViewApplication.GoNextImageAction)
    readonly property var zoomInQAction: root.application.actionForId(KiriViewApplication.ViewZoomInAction)
    readonly property var zoomOutQAction: root.application.actionForId(KiriViewApplication.ViewZoomOutAction)
    readonly property var panLeftQAction: root.application.actionForId(KiriViewApplication.ViewPanLeftAction)
    readonly property var panRightQAction: root.application.actionForId(KiriViewApplication.ViewPanRightAction)
    readonly property var panUpQAction: root.application.actionForId(KiriViewApplication.ViewPanUpAction)
    readonly property var panDownQAction: root.application.actionForId(KiriViewApplication.ViewPanDownAction)
    readonly property var panTopLeftQAction: root.application.actionForId(KiriViewApplication.ViewPanTopLeftAction)
    readonly property var panBottomRightQAction: root.application.actionForId(KiriViewApplication.ViewPanBottomRightAction)
    readonly property var scanForwardQAction: root.application.actionForId(KiriViewApplication.ViewScanForwardAction)
    readonly property var scanBackwardQAction: root.application.actionForId(KiriViewApplication.ViewScanBackwardAction)

    signal imageBoundaryReached(string message)

    function panBy(deltaX, deltaY) {
        return imageViewport.panBy(deltaX, deltaY);
    }

    function panToBottomRight() {
        return imageViewport.panToBottomRight();
    }

    function panToTopLeft() {
        return imageViewport.panToTopLeft();
    }

    function openLeftFallbackImage() {
        if (root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable) {
            root.nextImageQAction.trigger();
            return;
        }

        root.previousImageQAction.trigger();
    }

    function openRightFallbackImage() {
        if (root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable) {
            root.previousImageQAction.trigger();
            return;
        }

        root.nextImageQAction.trigger();
    }

    function panLeftOrOpenPreviousImage() {
        if (root.imageHorizontallyPannable) {
            root.panBy(-root.keyboardPanDistance, 0);
            return;
        }

        root.openLeftFallbackImage();
    }

    function panRightOrOpenNextImage() {
        if (root.imageHorizontallyPannable) {
            root.panBy(root.keyboardPanDistance, 0);
            return;
        }

        root.openRightFallbackImage();
    }

    function scanForward() {
        if (!root.imagePannable) {
            root.nextImageQAction.trigger();
            return;
        }

        if (root.imageViewport.scanForward()) {
            return;
        }

        root.nextImageQAction.trigger();
    }

    function scanBackward() {
        if (!root.imagePannable) {
            root.previousImageQAction.trigger();
            return;
        }

        if (root.imageViewport.scanBackward()) {
            return;
        }

        if (root.atFirstImage) {
            root.imageBoundaryReached(KI18n.i18nc("@info:status", "First image"));
            return;
        }

        if (root.imageDocument.currentPageNumber > 1) {
            root.imageViewport.setNextDisplayedImageStartToFinalScanPosition();
            root.imageDocument.openImageAtPage(root.imageDocument.currentPageNumber - 1);
            return;
        }

        root.previousImageQAction.trigger();
    }

    function textInputFocused() {
        return imageToolBar.textInputFocused();
    }

    function toggleMenuPresentation() {
        if (root.application.menuPresentation === KiriViewApplication.MenuBar) {
            root.application.menuPresentation = KiriViewApplication.HamburgerMenu;
            return;
        }

        root.application.menuPresentation = KiriViewApplication.MenuBar;
    }

    function zoomByStep(stepCount, viewportX, viewportY) {
        return imageViewport.zoomByStep(stepCount, viewportX, viewportY);
    }

    function zoomByStepAtCenter(stepCount) {
        return root.zoomByStep(stepCount, root.imageViewport.viewportWidth / 2, root.imageViewport.viewportHeight / 2);
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileOpenAction]
        application: root.application
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileQuitAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.commandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileQuitAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileMoveToTrashAction, KiriViewApplication.FileDeleteAction]
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewZoomInAction, KiriViewApplication.ViewZoomOutAction, KiriViewApplication.ViewFitAction, KiriViewApplication.ViewFitHeightAction, KiriViewApplication.ViewFitWidthAction, KiriViewApplication.ViewActualSizeAction, KiriViewApplication.ViewToggleTwoPageModeAction]
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewToggleRightToLeftReadingAction]
        application: root.application
        shortcutsEnabled: root.rightToLeftReadingCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewPanLeftAction, KiriViewApplication.ViewPanRightAction]
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewPanUpAction, KiriViewApplication.ViewPanDownAction, KiriViewApplication.ViewPanTopLeftAction, KiriViewApplication.ViewPanBottomRightAction]
        application: root.application
        shortcutsEnabled: root.pannableCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewScanForwardAction, KiriViewApplication.ViewScanBackwardAction]
        application: root.application
        shortcutsEnabled: root.readyCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousImageAction, KiriViewApplication.GoNextImageAction]
        application: root.application
        shortcutsEnabled: root.readyShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoFirstImageAction, KiriViewApplication.GoLastImageAction]
        application: root.application
        shortcutsEnabled: root.pageCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousSinglePageAction, KiriViewApplication.GoNextSinglePageAction]
        application: root.application
        shortcutsEnabled: root.twoPageCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousArchiveAction, KiriViewApplication.GoNextArchiveAction]
        application: root.application
        shortcutsEnabled: root.containerCommandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.WindowFullscreenAction, KiriViewApplication.HelpShortcutsAction]
        application: root.application
        shortcutsEnabled: root.commandShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.OptionsConfigureAction, KiriViewApplication.OptionsConfigureKeybindingAction, KiriViewApplication.OptionsShowMenubarAction]
        application: root.application
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.helpShortcutsEnabled
        sequence: "Ctrl+M"

        onActivated: root.toggleMenuPresentation()
    }

    ActionTrigger {
        action: root.zoomInQAction
        handler: () => root.zoomByStepAtCenter(1)
    }

    ActionTrigger {
        action: root.zoomOutQAction
        handler: () => root.zoomByStepAtCenter(-1)
    }

    ActionTrigger {
        action: root.panLeftQAction
        handler: () => root.panLeftOrOpenPreviousImage()
    }

    ActionTrigger {
        action: root.panRightQAction
        handler: () => root.panRightOrOpenNextImage()
    }

    ActionTrigger {
        action: root.panUpQAction
        handler: () => root.panBy(0, -root.keyboardPanDistance)
    }

    ActionTrigger {
        action: root.panDownQAction
        handler: () => root.panBy(0, root.keyboardPanDistance)
    }

    ActionTrigger {
        action: root.panTopLeftQAction
        handler: () => root.panToTopLeft()
    }

    ActionTrigger {
        action: root.panBottomRightQAction
        handler: () => root.panToBottomRight()
    }

    ActionTrigger {
        action: root.scanForwardQAction
        handler: () => root.scanForward()
    }

    ActionTrigger {
        action: root.scanBackwardQAction
        handler: () => root.scanBackward()
    }
}
