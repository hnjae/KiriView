// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONACTIONSTATEPOLICY_H
#define KIRIVIEW_APPLICATIONACTIONSTATEPOLICY_H

#include "application/applicationtypes.h"

#include <QString>
#include <QtGlobal>

namespace KiriView::ApplicationActions {
struct ApplicationActionStateInput {
    quint64 uiGateRevision = 0;
    bool helpActionsEnabled = false;
    bool readyActionsEnabled = false;
    bool rotateActionsEnabled = false;
    bool twoPageModeActionsEnabled = false;
    bool rightToLeftReadingActionsEnabled = false;
    bool containerNavigationActionsEnabled = false;
    bool displayedMediaOpenWithAvailable = false;
    bool displayedFileDeletionAvailable = false;
    bool fileDeletionInProgress = false;
    bool activeNavigationAvailable = false;
    bool activeNavigationKnown = false;
    bool activeNavigationHasTargets = false;
    bool canOpenPreviousActiveNavigation = false;
    bool canOpenNextActiveNavigation = false;
    bool fitModeSelected = false;
    bool fitHeightModeSelected = false;
    bool fitWidthModeSelected = false;
    bool twoPageModeActive = false;
    bool rightToLeftReadingActive = false;
    bool infoPanelVisible = false;
    bool thumbnailPanelVisible = false;
    bool fullscreen = false;
    bool applicationMenuShortcutEnabled = false;
    bool showMenubarActionEnabled = true;
    bool directMediaNavigationBoundaryActive = false;
    bool viewerShortcutsEnabled = false;
    bool readyShortcutsEnabled = false;
    bool readyViewerShortcutsEnabled = false;
    bool twoPageViewerShortcutsEnabled = false;
    bool rightToLeftReadingShortcutsEnabled = false;
    bool rightToLeftReadingViewerShortcutsEnabled = false;
    bool rotateShortcutsEnabled = false;
    bool rotateViewerShortcutsEnabled = false;
    bool pannableShortcutsEnabled = false;
    bool pannableViewerShortcutsEnabled = false;
    bool containerViewerShortcutsEnabled = false;
    bool activeNavigationActionsAvailable = false;
    bool videoMode = false;
    bool videoFileDeletionInProgress = false;
};

struct ApplicationActionState {
    bool actionEnabled = true;
    bool placementEnabled = true;
    bool checkable = false;
    bool checked = false;
};

ApplicationActionState applicationActionState(
    ActionId actionId, const ApplicationActionStateInput &input);
QString applicationActionMenuText(ActionId actionId, const ApplicationActionStateInput &input);
QString applicationActionToolbarText(ActionId actionId);
QString applicationActionToolbarTooltipText(ActionId actionId);
bool applicationShortcutsEnabledForScope(
    const ApplicationActionStateInput &input, ImageShortcutScope scope);
}

#endif
