// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

pragma ComponentBehavior: Bound

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
    readonly property var fileOpenActionIds: [KiriViewApplication.FileOpenAction]
    readonly property var fileQuitActionIds: [KiriViewApplication.FileQuitAction]
    readonly property var fileRemovalActionIds: [KiriViewApplication.FileMoveToTrashAction, KiriViewApplication.FileDeleteAction]
    readonly property var zoomActionIds: [KiriViewApplication.ViewZoomInAction, KiriViewApplication.ViewZoomOutAction, KiriViewApplication.ViewFitAction, KiriViewApplication.ViewFitHeightAction, KiriViewApplication.ViewFitWidthAction, KiriViewApplication.ViewActualSizeAction, KiriViewApplication.ViewToggleTwoPageModeAction]
    readonly property var rotateActionIds: [KiriViewApplication.ViewRotateClockwiseAction, KiriViewApplication.ViewRotateCounterclockwiseAction]
    readonly property var rightToLeftReadingActionIds: [KiriViewApplication.ViewToggleRightToLeftReadingAction]
    readonly property var panActionIds: [KiriViewApplication.ViewPanTopLeftAction, KiriViewApplication.ViewPanBottomRightAction]
    readonly property var scanActionIds: [KiriViewApplication.ViewScanForwardAction, KiriViewApplication.ViewScanBackwardAction]
    readonly property var imageNavigationActionIds: [KiriViewApplication.GoPreviousImageAction, KiriViewApplication.GoNextImageAction]
    readonly property var pageNavigationActionIds: [KiriViewApplication.GoFirstImageAction, KiriViewApplication.GoLastImageAction]
    readonly property var containerNavigationActionIds: [KiriViewApplication.GoPreviousArchiveAction, KiriViewApplication.GoNextArchiveAction]
    readonly property var globalActionIds: [KiriViewApplication.WindowFullscreenAction, KiriViewApplication.HelpShortcutsAction]
    readonly property var optionsActionIds: [KiriViewApplication.OptionsConfigureKeybindingAction, KiriViewApplication.OptionsShowMenubarAction]
    readonly property int shortcutScopeHelp: 0
    readonly property int shortcutScopeViewer: 1
    readonly property int shortcutScopeReady: 2
    readonly property int shortcutScopeReadyViewer: 3
    readonly property int shortcutScopeImageSelection: 4
    readonly property int shortcutScopeImageSelectionViewer: 5
    readonly property int shortcutScopePage: 6
    readonly property int shortcutScopePageViewer: 7
    readonly property int shortcutScopeRightToLeftReading: 8
    readonly property int shortcutScopeRightToLeftReadingViewer: 9
    readonly property int shortcutScopeRotate: 10
    readonly property int shortcutScopeRotateViewer: 11
    readonly property int shortcutScopePannable: 12
    readonly property int shortcutScopePannableViewer: 13
    readonly property int shortcutScopeContainer: 14
    readonly property int shortcutScopeContainerViewer: 15
    readonly property var actionShortcutRoutes: root.createActionShortcutRoutes()

    signal imageBoundaryReached(string message)

    function shortcutRoute(actionIds, shortcutFilter, shortcutScope) {
        return {
            "actionIds": actionIds,
            "shortcutFilter": shortcutFilter,
            "shortcutScope": shortcutScope
        };
    }

    function createActionShortcutRoutes() {
        const routes = [];
        routes.push(root.shortcutRoute(root.fileOpenActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopeHelp));
        routes.push(root.shortcutRoute(root.fileOpenActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopeViewer));
        routes.push(root.shortcutRoute(root.fileQuitActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopeHelp));
        routes.push(root.shortcutRoute(root.fileQuitActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopeViewer));
        routes.push(root.shortcutRoute(root.fileQuitActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopeViewer));
        routes.push(root.shortcutRoute(root.fileRemovalActionIds, ConfiguredActionShortcut.AllShortcuts, root.shortcutScopeReadyViewer));
        routes.push(root.shortcutRoute(root.zoomActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopeReady));
        routes.push(root.shortcutRoute(root.zoomActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopeReadyViewer));
        routes.push(root.shortcutRoute(root.zoomActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopeReadyViewer));
        routes.push(root.shortcutRoute(root.rotateActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopeRotate));
        routes.push(root.shortcutRoute(root.rotateActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopeRotateViewer));
        routes.push(root.shortcutRoute(root.rotateActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopeRotateViewer));
        routes.push(root.shortcutRoute(root.rightToLeftReadingActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopeRightToLeftReading));
        routes.push(root.shortcutRoute(root.rightToLeftReadingActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopeRightToLeftReadingViewer));
        routes.push(root.shortcutRoute(root.rightToLeftReadingActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopeRightToLeftReadingViewer));
        routes.push(root.shortcutRoute(root.panActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopePannable));
        routes.push(root.shortcutRoute(root.panActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopePannableViewer));
        routes.push(root.shortcutRoute(root.panActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopePannableViewer));
        routes.push(root.shortcutRoute(root.scanActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopeReady));
        routes.push(root.shortcutRoute(root.scanActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopeReadyViewer));
        routes.push(root.shortcutRoute(root.scanActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopeReadyViewer));
        routes.push(root.shortcutRoute(root.imageNavigationActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopeImageSelection));
        routes.push(root.shortcutRoute(root.imageNavigationActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopeImageSelectionViewer));
        routes.push(root.shortcutRoute(root.imageNavigationActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopeImageSelectionViewer));
        routes.push(root.shortcutRoute(root.pageNavigationActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopePage));
        routes.push(root.shortcutRoute(root.pageNavigationActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopePageViewer));
        routes.push(root.shortcutRoute(root.pageNavigationActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopePageViewer));
        routes.push(root.shortcutRoute(root.containerNavigationActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopeContainer));
        routes.push(root.shortcutRoute(root.containerNavigationActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopeContainerViewer));
        routes.push(root.shortcutRoute(root.containerNavigationActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopeContainerViewer));
        routes.push(root.shortcutRoute(root.globalActionIds, ConfiguredActionShortcut.WithCommandModifier, root.shortcutScopeHelp));
        routes.push(root.shortcutRoute(root.globalActionIds, ConfiguredActionShortcut.WithoutCommandModifier, root.shortcutScopeHelp));
        routes.push(root.shortcutRoute(root.globalActionIds, ConfiguredActionShortcut.ShortcutAliases, root.shortcutScopeViewer));
        routes.push(root.shortcutRoute(root.optionsActionIds, ConfiguredActionShortcut.AllShortcuts, root.shortcutScopeHelp));
        return routes;
    }

    function shortcutsEnabledForScope(shortcutScope) {
        switch (shortcutScope) {
        case root.shortcutScopeHelp:
            return root.helpShortcutsEnabled;
        case root.shortcutScopeViewer:
            return root.viewerShortcutsEnabled;
        case root.shortcutScopeReady:
            return root.readyShortcutsEnabled;
        case root.shortcutScopeReadyViewer:
            return root.readyViewerShortcutsEnabled;
        case root.shortcutScopeImageSelection:
            return root.imageSelectionShortcutsEnabled;
        case root.shortcutScopeImageSelectionViewer:
            return root.imageSelectionViewerShortcutsEnabled;
        case root.shortcutScopePage:
            return root.pageShortcutsEnabled;
        case root.shortcutScopePageViewer:
            return root.pageViewerShortcutsEnabled;
        case root.shortcutScopeRightToLeftReading:
            return root.rightToLeftReadingShortcutsEnabled;
        case root.shortcutScopeRightToLeftReadingViewer:
            return root.rightToLeftReadingViewerShortcutsEnabled;
        case root.shortcutScopeRotate:
            return root.rotateShortcutsEnabled;
        case root.shortcutScopeRotateViewer:
            return root.rotateViewerShortcutsEnabled;
        case root.shortcutScopePannable:
            return root.pannableShortcutsEnabled;
        case root.shortcutScopePannableViewer:
            return root.pannableViewerShortcutsEnabled;
        case root.shortcutScopeContainer:
            return root.containerShortcutsEnabled;
        case root.shortcutScopeContainerViewer:
            return root.containerViewerShortcutsEnabled;
        default:
            return false;
        }
    }

    function panBy(deltaX, deltaY) {
        return imageViewport.panBy(deltaX, deltaY);
    }

    function panToBottomRight() {
        return imageViewport.panToBottomRight();
    }

    function panToTopLeft() {
        return imageViewport.panToTopLeft();
    }

    function rightToLeftReadingActive() {
        return root.imageDocument.rightToLeftReadingEnabled && root.imageDocument.rightToLeftReadingAvailable;
    }

    function applyHorizontalArrowAction(action) {
        switch (action) {
        case ImageShortcutNavigationPolicy.PanLeft:
            root.panBy(-root.keyboardPanDistance, 0);
            return;
        case ImageShortcutNavigationPolicy.PanRight:
            root.panBy(root.keyboardPanDistance, 0);
            return;
        case ImageShortcutNavigationPolicy.OpenPreviousImage:
            root.previousImageQAction.trigger();
            return;
        case ImageShortcutNavigationPolicy.OpenNextImage:
            root.nextImageQAction.trigger();
            return;
        }
    }

    function applySinglePageArrowAction(action) {
        switch (action) {
        case ImageShortcutNavigationPolicy.OpenPreviousSinglePage:
            root.imageDocument.openPreviousSinglePage();
            return;
        case ImageShortcutNavigationPolicy.OpenNextSinglePage:
            root.imageDocument.openNextSinglePage();
            return;
        }
    }

    function applyScanAction(action) {
        switch (action) {
        case ImageShortcutNavigationPolicy.NoScanAction:
            return;
        case ImageShortcutNavigationPolicy.OpenPreviousImageFromScan:
            root.previousImageQAction.trigger();
            return;
        case ImageShortcutNavigationPolicy.OpenNextImageFromScan:
            root.nextImageQAction.trigger();
            return;
        case ImageShortcutNavigationPolicy.OpenPreviousPageFromFinalScanStart:
            root.imageViewport.setNextDisplayedImageStartToFinalScanPosition();
            root.imageDocument.openImageAtPage(root.imageDocument.currentPageNumber - 1);
            return;
        case ImageShortcutNavigationPolicy.ShowFirstImageBoundary:
            root.imageBoundaryReached(KI18n.i18nc("@info:status", "First image"));
            return;
        }
    }

    function handleHorizontalArrow(leftArrow) {
        const action = navigationPolicy.horizontalArrowAction(leftArrow, root.imageHorizontallyPannable, root.rightToLeftReadingActive());
        root.applyHorizontalArrowAction(action);
    }

    function handleSinglePageArrow(leftArrow) {
        const action = navigationPolicy.singlePageArrowAction(leftArrow, root.rightToLeftReadingActive());
        root.applySinglePageArrowAction(action);
    }

    function scanForward() {
        const action = navigationPolicy.scanForwardAction(root.imagePannable, root.imagePannable && root.imageViewport.scanForward());
        root.applyScanAction(action);
    }

    function scanBackward() {
        const action = navigationPolicy.scanBackwardAction(root.imagePannable, root.imagePannable && root.imageViewport.scanBackward(), root.atFirstImage, root.imageDocument.currentPageNumber);
        root.applyScanAction(action);
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

    ImageShortcutNavigationPolicy {
        id: navigationPolicy
    }

    Repeater {
        model: root.actionShortcutRoutes

        ActionShortcutGroup {
            required property var modelData

            actionIds: modelData.actionIds
            application: root.application
            shortcutFilter: modelData.shortcutFilter
            shortcutsEnabled: root.shortcutsEnabledForScope(modelData.shortcutScope)
        }
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

        onActivated: root.handleHorizontalArrow(true)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.readyViewerShortcutsEnabled
        sequence: "Right"

        onActivated: root.handleHorizontalArrow(false)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.twoPageViewerShortcutsEnabled
        sequence: "Shift+Left"

        onActivated: root.handleSinglePageArrow(true)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: root.twoPageViewerShortcutsEnabled
        sequence: "Shift+Right"

        onActivated: root.handleSinglePageArrow(false)
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
