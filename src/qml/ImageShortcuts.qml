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

    readonly property int keyboardPanDistance: 64

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
    readonly property var actionShortcutRoutes: root.createActionShortcutRoutes()

    signal imageBoundaryReached(string message)

    ImageActionAvailability {
        id: actionAvailability

        containerNavigationAvailable: root.imageDocument.containerNavigationAvailable
        currentLastPageNumber: root.imageDocument.currentLastPageNumber
        currentPageNumber: root.imageDocument.currentPageNumber
        fileDeletionInProgress: root.imageDocument.fileDeletionInProgress
        helpDialogOpen: root.helpDialogOpen
        imageCount: root.imageDocument.imageCount
        imageHorizontallyPannable: root.imageViewport.imageHorizontallyPannable
        imagePannable: root.imageViewport.imagePannable
        imageReady: root.imageDocument.status === KiriImageDocument.Ready
        rightToLeftReadingAvailable: root.imageDocument.rightToLeftReadingAvailable
        rightToLeftReadingEnabled: root.imageDocument.rightToLeftReadingEnabled
        textInputFocused: root.imageToolBar.textInputFocused()
        twoPageModeAvailable: root.imageDocument.twoPageModeAvailable
        twoPageModeEnabled: root.imageDocument.twoPageModeEnabled
    }

    function shortcutRoute(actionIds, shortcutFilter, shortcutScope) {
        return {
            "actionIds": actionIds,
            "shortcutFilter": shortcutFilter,
            "shortcutScope": shortcutScope
        };
    }

    function createActionShortcutRoutes() {
        const routes = [];
        routes.push(root.shortcutRoute(root.fileOpenActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.HelpShortcutScope));
        routes.push(root.shortcutRoute(root.fileOpenActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.ViewerShortcutScope));
        routes.push(root.shortcutRoute(root.fileQuitActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.HelpShortcutScope));
        routes.push(root.shortcutRoute(root.fileQuitActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.ViewerShortcutScope));
        routes.push(root.shortcutRoute(root.fileQuitActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.ViewerShortcutScope));
        routes.push(root.shortcutRoute(root.fileRemovalActionIds, ConfiguredActionShortcut.AllShortcuts, ImageActionAvailability.ReadyViewerShortcutScope));
        routes.push(root.shortcutRoute(root.zoomActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.ReadyShortcutScope));
        routes.push(root.shortcutRoute(root.zoomActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.ReadyViewerShortcutScope));
        routes.push(root.shortcutRoute(root.zoomActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.ReadyViewerShortcutScope));
        routes.push(root.shortcutRoute(root.rotateActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.RotateShortcutScope));
        routes.push(root.shortcutRoute(root.rotateActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.RotateViewerShortcutScope));
        routes.push(root.shortcutRoute(root.rotateActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.RotateViewerShortcutScope));
        routes.push(root.shortcutRoute(root.rightToLeftReadingActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.RightToLeftReadingShortcutScope));
        routes.push(root.shortcutRoute(root.rightToLeftReadingActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.RightToLeftReadingViewerShortcutScope));
        routes.push(root.shortcutRoute(root.rightToLeftReadingActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.RightToLeftReadingViewerShortcutScope));
        routes.push(root.shortcutRoute(root.panActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.PannableShortcutScope));
        routes.push(root.shortcutRoute(root.panActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.PannableViewerShortcutScope));
        routes.push(root.shortcutRoute(root.panActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.PannableViewerShortcutScope));
        routes.push(root.shortcutRoute(root.scanActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.ReadyShortcutScope));
        routes.push(root.shortcutRoute(root.scanActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.ReadyViewerShortcutScope));
        routes.push(root.shortcutRoute(root.scanActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.ReadyViewerShortcutScope));
        routes.push(root.shortcutRoute(root.imageNavigationActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.ImageSelectionShortcutScope));
        routes.push(root.shortcutRoute(root.imageNavigationActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.ImageSelectionViewerShortcutScope));
        routes.push(root.shortcutRoute(root.imageNavigationActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.ImageSelectionViewerShortcutScope));
        routes.push(root.shortcutRoute(root.pageNavigationActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.PageShortcutScope));
        routes.push(root.shortcutRoute(root.pageNavigationActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.PageViewerShortcutScope));
        routes.push(root.shortcutRoute(root.pageNavigationActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.PageViewerShortcutScope));
        routes.push(root.shortcutRoute(root.containerNavigationActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.ContainerShortcutScope));
        routes.push(root.shortcutRoute(root.containerNavigationActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.ContainerViewerShortcutScope));
        routes.push(root.shortcutRoute(root.containerNavigationActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.ContainerViewerShortcutScope));
        routes.push(root.shortcutRoute(root.globalActionIds, ConfiguredActionShortcut.WithCommandModifier, ImageActionAvailability.HelpShortcutScope));
        routes.push(root.shortcutRoute(root.globalActionIds, ConfiguredActionShortcut.WithoutCommandModifier, ImageActionAvailability.HelpShortcutScope));
        routes.push(root.shortcutRoute(root.globalActionIds, ConfiguredActionShortcut.ShortcutAliases, ImageActionAvailability.ViewerShortcutScope));
        routes.push(root.shortcutRoute(root.optionsActionIds, ConfiguredActionShortcut.AllShortcuts, ImageActionAvailability.HelpShortcutScope));
        return routes;
    }

    function shortcutsEnabledForScope(shortcutScope, availabilityRevision) {
        if (availabilityRevision < 0) {
            return false;
        }
        return actionAvailability.shortcutsEnabledForScope(shortcutScope);
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
        const action = navigationPolicy.horizontalArrowAction(leftArrow, actionAvailability.imageHorizontallyPannable, actionAvailability.rightToLeftReadingActive);
        root.applyHorizontalArrowAction(action);
    }

    function handleSinglePageArrow(leftArrow) {
        const action = navigationPolicy.singlePageArrowAction(leftArrow, actionAvailability.rightToLeftReadingActive);
        root.applySinglePageArrowAction(action);
    }

    function scanForward() {
        const action = navigationPolicy.scanForwardAction(actionAvailability.imagePannable, actionAvailability.imagePannable && root.imageViewport.scanForward());
        root.applyScanAction(action);
    }

    function scanBackward() {
        const action = navigationPolicy.scanBackwardAction(actionAvailability.imagePannable, actionAvailability.imagePannable && root.imageViewport.scanBackward(), actionAvailability.atKnownFirstImage, root.imageDocument.currentPageNumber);
        root.applyScanAction(action);
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
            shortcutsEnabled: root.shortcutsEnabledForScope(modelData.shortcutScope, actionAvailability.availabilityRevision)
        }
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: actionAvailability.helpShortcutsEnabled
        sequence: "Ctrl+M"

        onActivated: root.showMenubarQAction.trigger()
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: actionAvailability.readyViewerShortcutsEnabled
        sequence: "Left"

        onActivated: root.handleHorizontalArrow(true)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: actionAvailability.readyViewerShortcutsEnabled
        sequence: "Right"

        onActivated: root.handleHorizontalArrow(false)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: actionAvailability.twoPageViewerShortcutsEnabled
        sequence: "Shift+Left"

        onActivated: root.handleSinglePageArrow(true)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: actionAvailability.twoPageViewerShortcutsEnabled
        sequence: "Shift+Right"

        onActivated: root.handleSinglePageArrow(false)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: actionAvailability.pannableViewerShortcutsEnabled
        sequence: "Up"

        onActivated: root.panBy(0, -root.keyboardPanDistance)
    }

    Shortcut {
        context: Qt.WindowShortcut
        enabled: actionAvailability.pannableViewerShortcutsEnabled
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
