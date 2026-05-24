// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailability.h"

namespace {
KiriView::ApplicationActions::ImageShortcutScope applicationShortcutScope(
    ImageActionAvailability::ShortcutScope scope)
{
    return static_cast<KiriView::ApplicationActions::ImageShortcutScope>(scope);
}
}

ImageActionAvailability::ImageActionAvailability(QObject *parent)
    : QObject(parent)
    , m_projection(imageActionAvailabilityProjection(m_input))
{
}

bool ImageActionAvailability::imageReady() const { return m_input.imageReady; }

void ImageActionAvailability::setImageReady(bool imageReady)
{
    setBool(m_input.imageReady, imageReady);
}

bool ImageActionAvailability::fileDeletionInProgress() const
{
    return m_input.fileDeletionInProgress;
}

void ImageActionAvailability::setFileDeletionInProgress(bool fileDeletionInProgress)
{
    setBool(m_input.fileDeletionInProgress, fileDeletionInProgress);
}

bool ImageActionAvailability::helpDialogOpen() const { return m_input.helpDialogOpen; }

void ImageActionAvailability::setHelpDialogOpen(bool helpDialogOpen)
{
    setBool(m_input.helpDialogOpen, helpDialogOpen);
}

bool ImageActionAvailability::textInputFocused() const { return m_input.textInputFocused; }

void ImageActionAvailability::setTextInputFocused(bool textInputFocused)
{
    setBool(m_input.textInputFocused, textInputFocused);
}

bool ImageActionAvailability::imagePannable() const { return m_input.imagePannable; }

void ImageActionAvailability::setImagePannable(bool imagePannable)
{
    setBool(m_input.imagePannable, imagePannable);
}

bool ImageActionAvailability::imageHorizontallyPannable() const
{
    return m_input.imageHorizontallyPannable;
}

void ImageActionAvailability::setImageHorizontallyPannable(bool imageHorizontallyPannable)
{
    setBool(m_input.imageHorizontallyPannable, imageHorizontallyPannable);
}

bool ImageActionAvailability::containerNavigationAvailable() const
{
    return m_input.containerNavigationAvailable;
}

void ImageActionAvailability::setContainerNavigationAvailable(bool containerNavigationAvailable)
{
    setBool(m_input.containerNavigationAvailable, containerNavigationAvailable);
}

bool ImageActionAvailability::twoPageModeEnabled() const { return m_input.twoPageModeEnabled; }

void ImageActionAvailability::setTwoPageModeEnabled(bool twoPageModeEnabled)
{
    setBool(m_input.twoPageModeEnabled, twoPageModeEnabled);
}

bool ImageActionAvailability::twoPageModeAvailable() const { return m_input.twoPageModeAvailable; }

void ImageActionAvailability::setTwoPageModeAvailable(bool twoPageModeAvailable)
{
    setBool(m_input.twoPageModeAvailable, twoPageModeAvailable);
}

bool ImageActionAvailability::rightToLeftReadingEnabled() const
{
    return m_input.rightToLeftReadingEnabled;
}

void ImageActionAvailability::setRightToLeftReadingEnabled(bool rightToLeftReadingEnabled)
{
    setBool(m_input.rightToLeftReadingEnabled, rightToLeftReadingEnabled);
}

bool ImageActionAvailability::rightToLeftReadingAvailable() const
{
    return m_input.rightToLeftReadingAvailable;
}

void ImageActionAvailability::setRightToLeftReadingAvailable(bool rightToLeftReadingAvailable)
{
    setBool(m_input.rightToLeftReadingAvailable, rightToLeftReadingAvailable);
}

bool ImageActionAvailability::scanBackwardAtFirstImageBoundary() const
{
    return m_scanBackwardAtFirstImageBoundary;
}

void ImageActionAvailability::setScanBackwardAtFirstImageBoundary(
    bool scanBackwardAtFirstImageBoundary)
{
    setBool(m_scanBackwardAtFirstImageBoundary, scanBackwardAtFirstImageBoundary);
}

bool ImageActionAvailability::canUseReadyActions() const { return m_projection.canUseReadyActions; }

bool ImageActionAvailability::canUseRotateActions() const
{
    return m_projection.canUseRotateActions;
}

bool ImageActionAvailability::canUseTwoPageModeActions() const
{
    return m_projection.canUseTwoPageModeActions;
}

bool ImageActionAvailability::canUseRightToLeftReadingActions() const
{
    return m_projection.canUseRightToLeftReadingActions;
}

bool ImageActionAvailability::rightToLeftReadingActive() const
{
    return m_projection.rightToLeftReadingActive;
}

bool ImageActionAvailability::twoPageModeActive() const { return m_projection.twoPageModeActive; }

bool ImageActionAvailability::helpShortcutsEnabled() const
{
    return m_projection.helpShortcutsEnabled;
}

bool ImageActionAvailability::viewerShortcutsEnabled() const
{
    return m_projection.viewerShortcutsEnabled;
}

bool ImageActionAvailability::readyShortcutsEnabled() const
{
    return m_projection.readyShortcutsEnabled;
}

bool ImageActionAvailability::readyViewerShortcutsEnabled() const
{
    return m_projection.readyViewerShortcutsEnabled;
}

bool ImageActionAvailability::twoPageViewerShortcutsEnabled() const
{
    return m_projection.twoPageViewerShortcutsEnabled;
}

bool ImageActionAvailability::rightToLeftReadingShortcutsEnabled() const
{
    return m_projection.rightToLeftReadingShortcutsEnabled;
}

bool ImageActionAvailability::rightToLeftReadingViewerShortcutsEnabled() const
{
    return m_projection.rightToLeftReadingViewerShortcutsEnabled;
}

bool ImageActionAvailability::rotateShortcutsEnabled() const
{
    return m_projection.rotateShortcutsEnabled;
}

bool ImageActionAvailability::rotateViewerShortcutsEnabled() const
{
    return m_projection.rotateViewerShortcutsEnabled;
}

bool ImageActionAvailability::pannableShortcutsEnabled() const
{
    return m_projection.pannableShortcutsEnabled;
}

bool ImageActionAvailability::pannableViewerShortcutsEnabled() const
{
    return m_projection.pannableViewerShortcutsEnabled;
}

bool ImageActionAvailability::containerShortcutsEnabled() const
{
    return m_projection.containerShortcutsEnabled;
}

bool ImageActionAvailability::containerViewerShortcutsEnabled() const
{
    return m_projection.containerViewerShortcutsEnabled;
}

int ImageActionAvailability::availabilityRevision() const { return m_availabilityRevision; }

bool ImageActionAvailability::shortcutsEnabledForScope(
    ImageActionAvailability::ShortcutScope scope) const
{
    return imageActionAvailabilityShortcutsEnabledForScope(
        m_projection, applicationShortcutScope(scope));
}

void ImageActionAvailability::setBool(bool &target, bool value)
{
    if (target == value) {
        return;
    }
    target = value;
    publishInputChange();
}

void ImageActionAvailability::publishInputChange()
{
    m_projection = imageActionAvailabilityProjection(m_input);
    ++m_availabilityRevision;
    Q_EMIT availabilityChanged();
}
