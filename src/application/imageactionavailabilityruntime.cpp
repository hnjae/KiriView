// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailabilityruntime.h"

#include <utility>

namespace KiriView::ApplicationActions {
ImageActionAvailabilityRuntime::ImageActionAvailabilityRuntime(ChangeCallback changeCallback)
    : m_changeCallback(std::move(changeCallback))
    , m_projection(imageActionAvailabilityProjection(m_input))
{
}

bool ImageActionAvailabilityRuntime::imageReady() const { return m_input.imageReady; }

void ImageActionAvailabilityRuntime::setImageReady(bool imageReady)
{
    setBool(m_input.imageReady, imageReady);
}

bool ImageActionAvailabilityRuntime::fileDeletionInProgress() const
{
    return m_input.fileDeletionInProgress;
}

void ImageActionAvailabilityRuntime::setFileDeletionInProgress(bool fileDeletionInProgress)
{
    setBool(m_input.fileDeletionInProgress, fileDeletionInProgress);
}

bool ImageActionAvailabilityRuntime::helpDialogOpen() const { return m_input.helpDialogOpen; }

void ImageActionAvailabilityRuntime::setHelpDialogOpen(bool helpDialogOpen)
{
    setBool(m_input.helpDialogOpen, helpDialogOpen);
}

bool ImageActionAvailabilityRuntime::textInputFocused() const { return m_input.textInputFocused; }

void ImageActionAvailabilityRuntime::setTextInputFocused(bool textInputFocused)
{
    setBool(m_input.textInputFocused, textInputFocused);
}

bool ImageActionAvailabilityRuntime::imagePannable() const { return m_input.imagePannable; }

void ImageActionAvailabilityRuntime::setImagePannable(bool imagePannable)
{
    setBool(m_input.imagePannable, imagePannable);
}

bool ImageActionAvailabilityRuntime::containerNavigationAvailable() const
{
    return m_input.containerNavigationAvailable;
}

void ImageActionAvailabilityRuntime::setContainerNavigationAvailable(
    bool containerNavigationAvailable)
{
    setBool(m_input.containerNavigationAvailable, containerNavigationAvailable);
}

bool ImageActionAvailabilityRuntime::twoPageModeEnabled() const
{
    return m_input.twoPageModeEnabled;
}

void ImageActionAvailabilityRuntime::setTwoPageModeEnabled(bool twoPageModeEnabled)
{
    setBool(m_input.twoPageModeEnabled, twoPageModeEnabled);
}

bool ImageActionAvailabilityRuntime::twoPageModeAvailable() const
{
    return m_input.twoPageModeAvailable;
}

void ImageActionAvailabilityRuntime::setTwoPageModeAvailable(bool twoPageModeAvailable)
{
    setBool(m_input.twoPageModeAvailable, twoPageModeAvailable);
}

bool ImageActionAvailabilityRuntime::rightToLeftReadingEnabled() const
{
    return m_input.rightToLeftReadingEnabled;
}

void ImageActionAvailabilityRuntime::setRightToLeftReadingEnabled(bool rightToLeftReadingEnabled)
{
    setBool(m_input.rightToLeftReadingEnabled, rightToLeftReadingEnabled);
}

bool ImageActionAvailabilityRuntime::rightToLeftReadingAvailable() const
{
    return m_input.rightToLeftReadingAvailable;
}

void ImageActionAvailabilityRuntime::setRightToLeftReadingAvailable(
    bool rightToLeftReadingAvailable)
{
    setBool(m_input.rightToLeftReadingAvailable, rightToLeftReadingAvailable);
}

bool ImageActionAvailabilityRuntime::canUseReadyActions() const
{
    return m_projection.canUseReadyActions;
}

bool ImageActionAvailabilityRuntime::canUseRotateActions() const
{
    return m_projection.canUseRotateActions;
}

bool ImageActionAvailabilityRuntime::canUseTwoPageModeActions() const
{
    return m_projection.canUseTwoPageModeActions;
}

bool ImageActionAvailabilityRuntime::canUseRightToLeftReadingActions() const
{
    return m_projection.canUseRightToLeftReadingActions;
}

bool ImageActionAvailabilityRuntime::rightToLeftReadingActive() const
{
    return m_projection.rightToLeftReadingActive;
}

bool ImageActionAvailabilityRuntime::twoPageModeActive() const
{
    return m_projection.twoPageModeActive;
}

bool ImageActionAvailabilityRuntime::helpShortcutsEnabled() const
{
    return m_projection.helpShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::viewerShortcutsEnabled() const
{
    return m_projection.viewerShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::readyShortcutsEnabled() const
{
    return m_projection.readyShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::readyViewerShortcutsEnabled() const
{
    return m_projection.readyViewerShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::twoPageViewerShortcutsEnabled() const
{
    return m_projection.twoPageViewerShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::rightToLeftReadingShortcutsEnabled() const
{
    return m_projection.rightToLeftReadingShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::rightToLeftReadingViewerShortcutsEnabled() const
{
    return m_projection.rightToLeftReadingViewerShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::rotateShortcutsEnabled() const
{
    return m_projection.rotateShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::rotateViewerShortcutsEnabled() const
{
    return m_projection.rotateViewerShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::pannableShortcutsEnabled() const
{
    return m_projection.pannableShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::pannableViewerShortcutsEnabled() const
{
    return m_projection.pannableViewerShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::containerShortcutsEnabled() const
{
    return m_projection.containerShortcutsEnabled;
}

bool ImageActionAvailabilityRuntime::containerViewerShortcutsEnabled() const
{
    return m_projection.containerViewerShortcutsEnabled;
}

int ImageActionAvailabilityRuntime::availabilityRevision() const { return m_availabilityRevision; }

bool ImageActionAvailabilityRuntime::shortcutsEnabledForScope(ImageShortcutScope scope) const
{
    return imageActionAvailabilityShortcutsEnabledForScope(m_projection, scope);
}

bool ImageActionAvailabilityRuntime::mediaShortcutsEnabledForScope(ImageShortcutScope scope,
    bool videoMode, bool activeNavigationActionsAvailable, bool videoFileDeletionInProgress) const
{
    return activeMediaShortcutsEnabledForScope(
        ActiveMediaShortcutAvailabilityInput {
            m_projection,
            videoMode,
            activeNavigationActionsAvailable,
            videoFileDeletionInProgress,
        },
        scope);
}

void ImageActionAvailabilityRuntime::setBool(bool &target, bool value)
{
    if (target == value) {
        return;
    }
    target = value;
    publishInputChange();
}

void ImageActionAvailabilityRuntime::publishInputChange()
{
    m_projection = imageActionAvailabilityProjection(m_input);
    ++m_availabilityRevision;
    notifyChanged();
}

void ImageActionAvailabilityRuntime::notifyChanged() const
{
    if (m_changeCallback) {
        m_changeCallback();
    }
}
}
