// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "applicationactionstatepolicy.h"

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
    case ActionId::ViewActualSizeAction:
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
}
