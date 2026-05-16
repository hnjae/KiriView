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
    readonly property bool atFirstImage: imageDocument.currentPageNumber === 1 && imageDocument.imageCount > 0
    readonly property bool atLastImage: imageDocument.currentPageNumber > 0 && imageDocument.currentPageNumber === imageDocument.imageCount
    readonly property int keyboardPanDistance: 64
    readonly property bool helpShortcutsEnabled: !root.helpDialogOpen
    readonly property bool viewerShortcutsEnabled: !root.textInputFocused() && root.helpShortcutsEnabled
    readonly property bool readyShortcutsEnabled: root.imageReady && !root.imageDocument.fileDeletionInProgress && root.helpShortcutsEnabled
    readonly property bool readyViewerShortcutsEnabled: root.imageReady && !root.imageDocument.fileDeletionInProgress && root.viewerShortcutsEnabled
    readonly property bool imageSelectionShortcutsEnabled: root.imageDocument.imageCount > 0 && root.imageDocument.currentPageNumber > 0 && !root.imageDocument.fileDeletionInProgress && root.helpShortcutsEnabled
    readonly property bool imageSelectionViewerShortcutsEnabled: root.imageDocument.imageCount > 0 && root.imageDocument.currentPageNumber > 0 && !root.imageDocument.fileDeletionInProgress && root.viewerShortcutsEnabled
    readonly property bool pageShortcutsEnabled: root.imageSelectionShortcutsEnabled
    readonly property bool pageViewerShortcutsEnabled: root.imageSelectionViewerShortcutsEnabled
    readonly property bool twoPageViewerShortcutsEnabled: root.imageSelectionViewerShortcutsEnabled && root.imageDocument.twoPageModeAvailable && root.imageDocument.twoPageModeEnabled
    readonly property bool rightToLeftReadingShortcutsEnabled: root.readyShortcutsEnabled && root.imageDocument.rightToLeftReadingAvailable
    readonly property bool rightToLeftReadingViewerShortcutsEnabled: root.readyViewerShortcutsEnabled && root.imageDocument.rightToLeftReadingAvailable
    readonly property bool rotateShortcutsEnabled: root.readyShortcutsEnabled && !(root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable)
    readonly property bool rotateViewerShortcutsEnabled: root.readyViewerShortcutsEnabled && !(root.imageDocument.twoPageModeEnabled && root.imageDocument.twoPageModeAvailable)
    readonly property bool pannableShortcutsEnabled: root.imagePannable && !root.imageDocument.fileDeletionInProgress && root.helpShortcutsEnabled
    readonly property bool pannableViewerShortcutsEnabled: root.imagePannable && !root.imageDocument.fileDeletionInProgress && root.viewerShortcutsEnabled
    readonly property bool containerShortcutsEnabled: root.imageDocument.containerNavigationAvailable && !root.imageDocument.fileDeletionInProgress && root.helpShortcutsEnabled
    readonly property bool containerViewerShortcutsEnabled: root.imageDocument.containerNavigationAvailable && !root.imageDocument.fileDeletionInProgress && root.viewerShortcutsEnabled

    readonly property var previousImageQAction: root.application.actionForId(KiriViewApplication.GoPreviousImageAction)
    readonly property var nextImageQAction: root.application.actionForId(KiriViewApplication.GoNextImageAction)
    readonly property var zoomInQAction: root.application.actionForId(KiriViewApplication.ViewZoomInAction)
    readonly property var zoomOutQAction: root.application.actionForId(KiriViewApplication.ViewZoomOutAction)
    readonly property var panTopLeftQAction: root.application.actionForId(KiriViewApplication.ViewPanTopLeftAction)
    readonly property var panBottomRightQAction: root.application.actionForId(KiriViewApplication.ViewPanBottomRightAction)
    readonly property var scanForwardQAction: root.application.actionForId(KiriViewApplication.ViewScanForwardAction)
    readonly property var scanBackwardQAction: root.application.actionForId(KiriViewApplication.ViewScanBackwardAction)
    readonly property var showMenubarQAction: root.application.actionForId(KiriViewApplication.OptionsShowMenubarAction)

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

    function openLeftSinglePage() {
        if (root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable) {
            root.imageDocument.openNextSinglePage();
            return;
        }

        root.imageDocument.openPreviousSinglePage();
    }

    function openRightSinglePage() {
        if (root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable) {
            root.imageDocument.openPreviousSinglePage();
            return;
        }

        root.imageDocument.openNextSinglePage();
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

    function zoomByStep(stepCount, viewportX, viewportY) {
        return imageViewport.zoomByStep(stepCount, viewportX, viewportY);
    }

    function zoomByStepAtCenter(stepCount) {
        return root.zoomByStep(stepCount, root.imageViewport.viewportWidth / 2, root.imageViewport.viewportHeight / 2);
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileOpenAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileOpenAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.viewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileQuitAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileQuitAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.viewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileQuitAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.viewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.FileMoveToTrashAction, KiriViewApplication.FileDeleteAction]
        application: root.application
        shortcutsEnabled: root.readyViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewZoomInAction, KiriViewApplication.ViewZoomOutAction, KiriViewApplication.ViewFitAction, KiriViewApplication.ViewFitHeightAction, KiriViewApplication.ViewFitWidthAction, KiriViewApplication.ViewActualSizeAction, KiriViewApplication.ViewToggleTwoPageModeAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.readyShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewZoomInAction, KiriViewApplication.ViewZoomOutAction, KiriViewApplication.ViewFitAction, KiriViewApplication.ViewFitHeightAction, KiriViewApplication.ViewFitWidthAction, KiriViewApplication.ViewActualSizeAction, KiriViewApplication.ViewToggleTwoPageModeAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.readyViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewZoomInAction, KiriViewApplication.ViewZoomOutAction, KiriViewApplication.ViewFitAction, KiriViewApplication.ViewFitHeightAction, KiriViewApplication.ViewFitWidthAction, KiriViewApplication.ViewActualSizeAction, KiriViewApplication.ViewToggleTwoPageModeAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.readyViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewRotateClockwiseAction, KiriViewApplication.ViewRotateCounterclockwiseAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.rotateShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewRotateClockwiseAction, KiriViewApplication.ViewRotateCounterclockwiseAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.rotateViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewRotateClockwiseAction, KiriViewApplication.ViewRotateCounterclockwiseAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.rotateViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewToggleRightToLeftReadingAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.rightToLeftReadingShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewToggleRightToLeftReadingAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.rightToLeftReadingViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewToggleRightToLeftReadingAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.rightToLeftReadingViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewPanTopLeftAction, KiriViewApplication.ViewPanBottomRightAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.pannableShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewPanTopLeftAction, KiriViewApplication.ViewPanBottomRightAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.pannableViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewPanTopLeftAction, KiriViewApplication.ViewPanBottomRightAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.pannableViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewScanForwardAction, KiriViewApplication.ViewScanBackwardAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.readyShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewScanForwardAction, KiriViewApplication.ViewScanBackwardAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.readyViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.ViewScanForwardAction, KiriViewApplication.ViewScanBackwardAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.readyViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousImageAction, KiriViewApplication.GoNextImageAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.imageSelectionShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousImageAction, KiriViewApplication.GoNextImageAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.imageSelectionViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousImageAction, KiriViewApplication.GoNextImageAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.imageSelectionViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoFirstImageAction, KiriViewApplication.GoLastImageAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.pageShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoFirstImageAction, KiriViewApplication.GoLastImageAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.pageViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoFirstImageAction, KiriViewApplication.GoLastImageAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.pageViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousArchiveAction, KiriViewApplication.GoNextArchiveAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.containerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousArchiveAction, KiriViewApplication.GoNextArchiveAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.containerViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.GoPreviousArchiveAction, KiriViewApplication.GoNextArchiveAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.containerViewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.WindowFullscreenAction, KiriViewApplication.HelpShortcutsAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithCommandModifier
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.WindowFullscreenAction, KiriViewApplication.HelpShortcutsAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.WithoutCommandModifier
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.WindowFullscreenAction, KiriViewApplication.HelpShortcutsAction]
        application: root.application
        shortcutFilter: ConfiguredActionShortcut.ShortcutAliases
        shortcutsEnabled: root.viewerShortcutsEnabled
    }

    ActionShortcutGroup {
        actionIds: [KiriViewApplication.OptionsConfigureKeybindingAction, KiriViewApplication.OptionsShowMenubarAction]
        application: root.application
        shortcutsEnabled: root.helpShortcutsEnabled
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.helpShortcutsEnabled
        sequence: "Ctrl+M"

        onActivated: root.showMenubarQAction.trigger()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.readyViewerShortcutsEnabled
        sequence: "Left"

        onActivated: root.panLeftOrOpenPreviousImage()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.readyViewerShortcutsEnabled
        sequence: "Right"

        onActivated: root.panRightOrOpenNextImage()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.twoPageViewerShortcutsEnabled
        sequence: "Shift+Left"

        onActivated: root.openLeftSinglePage()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.twoPageViewerShortcutsEnabled
        sequence: "Shift+Right"

        onActivated: root.openRightSinglePage()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.pannableViewerShortcutsEnabled
        sequence: "Up"

        onActivated: root.panBy(0, -root.keyboardPanDistance)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.pannableViewerShortcutsEnabled
        sequence: "Down"

        onActivated: root.panBy(0, root.keyboardPanDistance)
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
