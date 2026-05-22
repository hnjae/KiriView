// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATION_IMAGEACTIONAVAILABILITYPOLICY_H
#define KIRIVIEW_APPLICATION_IMAGEACTIONAVAILABILITYPOLICY_H

namespace KiriView::ApplicationActions {
enum class ImageShortcutScope {
    HelpShortcutScope = 0,
    ViewerShortcutScope,
    ReadyShortcutScope,
    ReadyViewerShortcutScope,
    ImageSelectionShortcutScope,
    ImageSelectionViewerShortcutScope,
    PageShortcutScope,
    PageViewerShortcutScope,
    RightToLeftReadingShortcutScope,
    RightToLeftReadingViewerShortcutScope,
    RotateShortcutScope,
    RotateViewerShortcutScope,
    PannableShortcutScope,
    PannableViewerShortcutScope,
    ContainerShortcutScope,
    ContainerViewerShortcutScope,
};
}

struct ImageActionAvailabilityInput {
    bool imageReady = false;
    int imageCount = 0;
    int currentPageNumber = 0;
    int currentLastPageNumber = 0;
    bool fileDeletionInProgress = false;
    bool helpDialogOpen = false;
    bool textInputFocused = false;
    bool imagePannable = false;
    bool imageHorizontallyPannable = false;
    bool containerNavigationAvailable = false;
    bool twoPageModeEnabled = false;
    bool twoPageModeAvailable = false;
    bool rightToLeftReadingEnabled = false;
    bool rightToLeftReadingAvailable = false;
};

struct ImageActionAvailabilityProjection {
    bool canOpenNextImage = false;
    bool canOpenPreviousImage = false;
    bool atKnownFirstImage = false;
    bool atKnownLastImage = false;
    bool canUsePageActions = false;
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
    bool imageSelectionShortcutsEnabled = false;
    bool imageSelectionViewerShortcutsEnabled = false;
    bool pageShortcutsEnabled = false;
    bool pageViewerShortcutsEnabled = false;
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

ImageActionAvailabilityProjection imageActionAvailabilityProjection(
    const ImageActionAvailabilityInput &input);
bool imageActionAvailabilityShortcutsEnabledForScope(
    const ImageActionAvailabilityProjection &projection,
    KiriView::ApplicationActions::ImageShortcutScope scope);

#endif
