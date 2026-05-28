// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATION_IMAGEACTIONAVAILABILITYRUNTIME_H
#define KIRIVIEW_APPLICATION_IMAGEACTIONAVAILABILITYRUNTIME_H

#include "application/imageactionavailabilitypolicy.h"

#include <functional>

namespace KiriView::ApplicationActions {
class ImageActionAvailabilityRuntime final
{
public:
    using ChangeCallback = std::function<void()>;

    explicit ImageActionAvailabilityRuntime(ChangeCallback changeCallback = {});

    bool imageReady() const;
    void setImageReady(bool imageReady);
    bool fileDeletionInProgress() const;
    void setFileDeletionInProgress(bool fileDeletionInProgress);
    bool helpDialogOpen() const;
    void setHelpDialogOpen(bool helpDialogOpen);
    bool textInputFocused() const;
    void setTextInputFocused(bool textInputFocused);
    bool imagePannable() const;
    void setImagePannable(bool imagePannable);
    bool containerNavigationAvailable() const;
    void setContainerNavigationAvailable(bool containerNavigationAvailable);
    bool twoPageModeEnabled() const;
    void setTwoPageModeEnabled(bool twoPageModeEnabled);
    bool twoPageModeAvailable() const;
    void setTwoPageModeAvailable(bool twoPageModeAvailable);
    bool rightToLeftReadingEnabled() const;
    void setRightToLeftReadingEnabled(bool rightToLeftReadingEnabled);
    bool rightToLeftReadingAvailable() const;
    void setRightToLeftReadingAvailable(bool rightToLeftReadingAvailable);

    bool canUseReadyActions() const;
    bool canUseRotateActions() const;
    bool canUseTwoPageModeActions() const;
    bool canUseRightToLeftReadingActions() const;
    bool rightToLeftReadingActive() const;
    bool twoPageModeActive() const;
    bool helpShortcutsEnabled() const;
    bool viewerShortcutsEnabled() const;
    bool readyShortcutsEnabled() const;
    bool readyViewerShortcutsEnabled() const;
    bool twoPageViewerShortcutsEnabled() const;
    bool rightToLeftReadingShortcutsEnabled() const;
    bool rightToLeftReadingViewerShortcutsEnabled() const;
    bool rotateShortcutsEnabled() const;
    bool rotateViewerShortcutsEnabled() const;
    bool pannableShortcutsEnabled() const;
    bool pannableViewerShortcutsEnabled() const;
    bool containerShortcutsEnabled() const;
    bool containerViewerShortcutsEnabled() const;
    int availabilityRevision() const;

    bool shortcutsEnabledForScope(ImageShortcutScope scope) const;
    bool mediaShortcutsEnabledForScope(ImageShortcutScope scope, bool videoMode,
        bool activeNavigationActionsAvailable, bool videoFileDeletionInProgress) const;

private:
    void setBool(bool &target, bool value);
    void publishInputChange();
    void notifyChanged() const;

    ChangeCallback m_changeCallback;
    ImageActionAvailabilityInput m_input;
    ImageActionAvailabilityProjection m_projection;
    int m_availabilityRevision = 0;
};
}

#endif
