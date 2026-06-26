// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATION_IMAGEACTIONAVAILABILITYPOLICY_H
#define KIRIVIEW_APPLICATION_IMAGEACTIONAVAILABILITYPOLICY_H

#include "application/applicationtypes.h"

namespace kiriview::ApplicationActions {
struct VideoShortcutAvailabilityInput
{
    bool helpShortcutsEnabled = false;
    bool viewerShortcutsEnabled = false;
    bool fileDeletionInProgress = false;
    bool directMediaNavigationActive = false;
};
}

struct ImageActionAvailabilityInput
{
    bool imageReady = false;
    bool fileDeletionInProgress = false;
    bool helpDialogOpen = false;
    bool textInputFocused = false;
    bool imagePannable = false;
    bool containerNavigationAvailable = false;
    bool twoPageModeEnabled = false;
    bool twoPageModeAvailable = false;
    bool rightToLeftReadingEnabled = false;
    bool rightToLeftReadingAvailable = false;
};

struct ImageActionAvailabilityProjection
{
    bool canUseReadyActions = false;
    bool canUseRotateActions = false;
    bool canUseTwoPageModeActions = false;
    bool canUseRightToLeftReadingActions = false;
    bool rightToLeftReadingActive = false;
    bool twoPageModeActive = false;
    bool helpShortcutsEnabled = false;
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
    bool containerShortcutsEnabled = false;
    bool containerViewerShortcutsEnabled = false;
};

struct ActiveMediaShortcutAvailabilityInput
{
    ImageActionAvailabilityProjection imageProjection;
    bool videoMode = false;
    bool activeNavigationActionsAvailable = false;
    bool videoFileDeletionInProgress = false;
};

ImageActionAvailabilityProjection imageActionAvailabilityProjection(
    const ImageActionAvailabilityInput& input);
bool imageActionAvailabilityShortcutsEnabledForScope(
    const ImageActionAvailabilityProjection& projection,
    kiriview::ApplicationActions::ImageShortcutScope scope);
bool activeMediaShortcutsEnabledForScope(const ActiveMediaShortcutAvailabilityInput& input,
    kiriview::ApplicationActions::ImageShortcutScope scope);

namespace kiriview::ApplicationActions {
bool videoShortcutsEnabledForScope(
    const VideoShortcutAvailabilityInput& input, ImageShortcutScope scope);
bool mediaHorizontalArrowShortcutsEnabled(bool videoMode, bool imageReadyViewerShortcutsEnabled,
    const VideoShortcutAvailabilityInput& videoInput);
}

#endif
