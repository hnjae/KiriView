// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionstatepolicy.h"

#include "imageactionavailabilitypolicy.h"

#include <KLocalizedString>

namespace {
using ActionId = KiriView::ApplicationActions::ActionId;

bool activeNavigationActionEnabled(
    const KiriView::ApplicationActions::ApplicationActionStateInput &input)
{
    return input.helpActionsEnabled && input.activeNavigationAvailable
        && input.activeNavigationKnown && input.activeNavigationHasTargets
        && !input.fileDeletionInProgress;
}

KiriView::ApplicationActions::ApplicationActionState state(bool enabled)
{
    return KiriView::ApplicationActions::ApplicationActionState { enabled, enabled, false, false };
}

KiriView::ApplicationActions::ApplicationActionState checkableState(bool enabled, bool checked)
{
    return KiriView::ApplicationActions::ApplicationActionState {
        enabled,
        enabled,
        true,
        checked,
    };
}

QString emptyText() { return {}; }
}

namespace KiriView::ApplicationActions {
ApplicationActionState applicationActionState(
    ActionId actionId, const ApplicationActionStateInput &input)
{
    switch (actionId) {
    case ActionId::FileOpenAction:
    case ActionId::HelpShortcutsAction:
        return state(input.helpActionsEnabled);
    case ActionId::FileOpenWithAction:
        return state(input.helpActionsEnabled && input.displayedMediaOpenWithAvailable);
    case ActionId::FileMoveToTrashAction:
    case ActionId::FileDeleteAction:
        return state(input.helpActionsEnabled && input.displayedFileDeletionAvailable);
    case ActionId::GoPreviousArchiveAction:
    case ActionId::GoNextArchiveAction:
        return state(input.containerNavigationActionsEnabled);
    case ActionId::GoPreviousImageAction: {
        ApplicationActionState actionState = state(activeNavigationActionEnabled(input));
        actionState.placementEnabled
            = actionState.actionEnabled && input.canOpenPreviousActiveNavigation;
        return actionState;
    }
    case ActionId::GoNextImageAction: {
        ApplicationActionState actionState = state(activeNavigationActionEnabled(input));
        actionState.placementEnabled
            = actionState.actionEnabled && input.canOpenNextActiveNavigation;
        return actionState;
    }
    case ActionId::GoFirstImageAction:
    case ActionId::GoLastImageAction:
        return state(activeNavigationActionEnabled(input));
    case ActionId::ViewZoomInAction:
    case ActionId::ViewZoomOutAction:
    case ActionId::ViewZoom50PercentAction:
    case ActionId::ViewZoom100PercentAction:
    case ActionId::ViewZoom200PercentAction:
    case ActionId::ViewScanForwardAction:
    case ActionId::ViewScanBackwardAction:
        return state(input.readyActionsEnabled);
    case ActionId::ViewFitAction:
        return checkableState(input.readyActionsEnabled, input.fitModeSelected);
    case ActionId::ViewFitHeightAction:
        return checkableState(input.readyActionsEnabled, input.fitHeightModeSelected);
    case ActionId::ViewFitWidthAction:
        return checkableState(input.readyActionsEnabled, input.fitWidthModeSelected);
    case ActionId::ViewRotateClockwiseAction:
    case ActionId::ViewRotateCounterclockwiseAction:
        return state(input.rotateActionsEnabled);
    case ActionId::ViewToggleTwoPageModeAction:
        return checkableState(input.twoPageModeActionsEnabled, input.twoPageModeActive);
    case ActionId::ViewToggleRightToLeftReadingAction:
        return checkableState(
            input.rightToLeftReadingActionsEnabled, input.rightToLeftReadingActive);
    case ActionId::ViewToggleInfoPanelAction:
        return checkableState(input.helpActionsEnabled, input.infoPanelVisible);
    case ActionId::ViewToggleThumbnailPanelAction:
        return checkableState(input.helpActionsEnabled, input.thumbnailPanelVisible);
    case ActionId::WindowFullscreenAction:
        return checkableState(input.helpActionsEnabled, input.fullscreen);
    case ActionId::OptionsConfigureKeybindingAction:
        return state(true);
    case ActionId::OptionsShowMenubarAction:
        return state(input.showMenubarActionEnabled);
    case ActionId::OpenApplicationMenuAction:
        return state(input.applicationMenuShortcutEnabled);
    case ActionId::FileQuitAction:
        return state(true);
    case ActionId::ViewPanTopLeftAction:
    case ActionId::ViewPanBottomRightAction:
        return state(input.readyActionsEnabled);
    case ActionId::ActionCount:
        return state(false);
    }

    return state(false);
}

QString applicationActionMenuText(ActionId actionId, const ApplicationActionStateInput &input)
{
    switch (actionId) {
    case ActionId::FileOpenAction:
        return i18nc("@action:inmenu", "&Open");
    case ActionId::FileOpenWithAction:
        return i18nc("@action:inmenu", "Open &With...");
    case ActionId::FileMoveToTrashAction:
        return i18nc("@action:inmenu", "Move to &Trash");
    case ActionId::FileDeleteAction:
        return i18nc("@action:inmenu", "&Delete Permanently");
    case ActionId::GoPreviousArchiveAction:
        return i18nc("@action:inmenu", "&Previous Archive");
    case ActionId::GoNextArchiveAction:
        return i18nc("@action:inmenu", "&Next Archive");
    case ActionId::GoPreviousImageAction:
        return i18nc("@action:inmenu", "&Previous");
    case ActionId::GoNextImageAction:
        return i18nc("@action:inmenu", "&Next");
    case ActionId::GoFirstImageAction:
        return input.directMediaNavigationBoundaryActive
            ? i18nc("@action:inmenu", "&First Media Item")
            : i18nc("@action:inmenu", "&First Image");
    case ActionId::GoLastImageAction:
        return input.directMediaNavigationBoundaryActive
            ? i18nc("@action:inmenu", "&Last Media Item")
            : i18nc("@action:inmenu", "&Last Image");
    case ActionId::ViewZoomInAction:
        return i18nc("@action:inmenu", "&Zoom In");
    case ActionId::ViewZoomOutAction:
        return i18nc("@action:inmenu", "Zoom &Out");
    case ActionId::ViewZoom50PercentAction:
        return i18nc("@action:inmenu", "Zoom to &50%");
    case ActionId::ViewZoom100PercentAction:
        return i18nc("@action:inmenu", "Zoom to &100%");
    case ActionId::ViewZoom200PercentAction:
        return i18nc("@action:inmenu", "Zoom to &200%");
    case ActionId::ViewFitAction:
        return i18nc("@action:inmenu", "Fit to &Window");
    case ActionId::ViewFitHeightAction:
        return i18nc("@action:inmenu", "Fit &Height");
    case ActionId::ViewFitWidthAction:
        return i18nc("@action:inmenu", "Fit &Width");
    case ActionId::ViewRotateClockwiseAction:
        return i18nc("@action:inmenu", "Rotate &Clockwise");
    case ActionId::ViewRotateCounterclockwiseAction:
        return i18nc("@action:inmenu", "Rotate C&ounterclockwise");
    case ActionId::ViewToggleTwoPageModeAction:
        return i18nc("@action:inmenu", "Two-Page &Spread");
    case ActionId::ViewToggleRightToLeftReadingAction:
        return i18nc("@action:inmenu", "&Right-to-Left Reading");
    case ActionId::ViewToggleInfoPanelAction:
        return i18nc("@action:inmenu", "Show &Info Panel");
    case ActionId::ViewToggleThumbnailPanelAction:
        return i18nc("@action:inmenu", "Show &Thumbnail Panel");
    case ActionId::WindowFullscreenAction:
        return i18nc("@action:inmenu", "&Fullscreen");
    case ActionId::HelpShortcutsAction:
        return i18nc("@action:inmenu", "&Keyboard Shortcuts");
    case ActionId::OptionsConfigureKeybindingAction:
        return i18nc("@action:inmenu", "&Configure Shortcuts...");
    case ActionId::OptionsShowMenubarAction:
        return i18nc("@action:inmenu", "&Show Menubar");
    case ActionId::FileQuitAction:
        return i18nc("@action:inmenu", "&Quit");
    default:
        return emptyText();
    }
}

QString applicationActionToolbarText(ActionId actionId)
{
    switch (actionId) {
    case ActionId::ViewToggleTwoPageModeAction:
        return i18nc("@action:button", "Two-Page &Spread");
    case ActionId::ViewToggleRightToLeftReadingAction:
        return i18nc("@action:button", "&Right-to-Left");
    default:
        return emptyText();
    }
}

QString applicationActionToolbarTooltipText(ActionId actionId)
{
    switch (actionId) {
    case ActionId::ViewToggleTwoPageModeAction:
        return i18nc("@action", "Two-Page Spread");
    case ActionId::ViewToggleRightToLeftReadingAction:
        return i18nc("@action", "Right-to-Left Reading");
    default:
        return emptyText();
    }
}

bool applicationShortcutsEnabledForScope(
    const ApplicationActionStateInput &input, ImageShortcutScope scope)
{
    const ImageActionAvailabilityProjection projection {
        input.readyActionsEnabled,
        input.rotateActionsEnabled,
        input.twoPageModeActionsEnabled,
        input.rightToLeftReadingActionsEnabled,
        input.rightToLeftReadingActive,
        input.twoPageModeActive,
        input.helpActionsEnabled,
        input.viewerShortcutsEnabled,
        input.readyShortcutsEnabled,
        input.readyViewerShortcutsEnabled,
        input.twoPageViewerShortcutsEnabled,
        input.rightToLeftReadingShortcutsEnabled,
        input.rightToLeftReadingViewerShortcutsEnabled,
        input.rotateShortcutsEnabled,
        input.rotateViewerShortcutsEnabled,
        input.pannableShortcutsEnabled,
        input.pannableViewerShortcutsEnabled,
        input.containerNavigationActionsEnabled,
        input.containerViewerShortcutsEnabled,
    };

    return activeMediaShortcutsEnabledForScope(
        ActiveMediaShortcutAvailabilityInput {
            projection,
            input.videoMode,
            input.activeNavigationActionsAvailable,
            input.videoFileDeletionInProgress,
        },
        scope);
}
}
