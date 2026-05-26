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
    , m_runtime([this]() { Q_EMIT availabilityChanged(); })
{
}

bool ImageActionAvailability::imageReady() const { return m_runtime.imageReady(); }

void ImageActionAvailability::setImageReady(bool imageReady)
{
    m_runtime.setImageReady(imageReady);
}

bool ImageActionAvailability::fileDeletionInProgress() const
{
    return m_runtime.fileDeletionInProgress();
}

void ImageActionAvailability::setFileDeletionInProgress(bool fileDeletionInProgress)
{
    m_runtime.setFileDeletionInProgress(fileDeletionInProgress);
}

bool ImageActionAvailability::helpDialogOpen() const { return m_runtime.helpDialogOpen(); }

void ImageActionAvailability::setHelpDialogOpen(bool helpDialogOpen)
{
    m_runtime.setHelpDialogOpen(helpDialogOpen);
}

bool ImageActionAvailability::textInputFocused() const { return m_runtime.textInputFocused(); }

void ImageActionAvailability::setTextInputFocused(bool textInputFocused)
{
    m_runtime.setTextInputFocused(textInputFocused);
}

bool ImageActionAvailability::imagePannable() const { return m_runtime.imagePannable(); }

void ImageActionAvailability::setImagePannable(bool imagePannable)
{
    m_runtime.setImagePannable(imagePannable);
}

bool ImageActionAvailability::imageHorizontallyPannable() const
{
    return m_runtime.imageHorizontallyPannable();
}

void ImageActionAvailability::setImageHorizontallyPannable(bool imageHorizontallyPannable)
{
    m_runtime.setImageHorizontallyPannable(imageHorizontallyPannable);
}

bool ImageActionAvailability::containerNavigationAvailable() const
{
    return m_runtime.containerNavigationAvailable();
}

void ImageActionAvailability::setContainerNavigationAvailable(bool containerNavigationAvailable)
{
    m_runtime.setContainerNavigationAvailable(containerNavigationAvailable);
}

bool ImageActionAvailability::twoPageModeEnabled() const { return m_runtime.twoPageModeEnabled(); }

void ImageActionAvailability::setTwoPageModeEnabled(bool twoPageModeEnabled)
{
    m_runtime.setTwoPageModeEnabled(twoPageModeEnabled);
}

bool ImageActionAvailability::twoPageModeAvailable() const
{
    return m_runtime.twoPageModeAvailable();
}

void ImageActionAvailability::setTwoPageModeAvailable(bool twoPageModeAvailable)
{
    m_runtime.setTwoPageModeAvailable(twoPageModeAvailable);
}

bool ImageActionAvailability::rightToLeftReadingEnabled() const
{
    return m_runtime.rightToLeftReadingEnabled();
}

void ImageActionAvailability::setRightToLeftReadingEnabled(bool rightToLeftReadingEnabled)
{
    m_runtime.setRightToLeftReadingEnabled(rightToLeftReadingEnabled);
}

bool ImageActionAvailability::rightToLeftReadingAvailable() const
{
    return m_runtime.rightToLeftReadingAvailable();
}

void ImageActionAvailability::setRightToLeftReadingAvailable(bool rightToLeftReadingAvailable)
{
    m_runtime.setRightToLeftReadingAvailable(rightToLeftReadingAvailable);
}

bool ImageActionAvailability::scanBackwardAtFirstImageBoundary() const
{
    return m_runtime.scanBackwardAtFirstImageBoundary();
}

void ImageActionAvailability::setScanBackwardAtFirstImageBoundary(
    bool scanBackwardAtFirstImageBoundary)
{
    m_runtime.setScanBackwardAtFirstImageBoundary(scanBackwardAtFirstImageBoundary);
}

bool ImageActionAvailability::canUseReadyActions() const { return m_runtime.canUseReadyActions(); }

bool ImageActionAvailability::canUseRotateActions() const
{
    return m_runtime.canUseRotateActions();
}

bool ImageActionAvailability::canUseTwoPageModeActions() const
{
    return m_runtime.canUseTwoPageModeActions();
}

bool ImageActionAvailability::canUseRightToLeftReadingActions() const
{
    return m_runtime.canUseRightToLeftReadingActions();
}

bool ImageActionAvailability::rightToLeftReadingActive() const
{
    return m_runtime.rightToLeftReadingActive();
}

bool ImageActionAvailability::twoPageModeActive() const { return m_runtime.twoPageModeActive(); }

bool ImageActionAvailability::helpShortcutsEnabled() const
{
    return m_runtime.helpShortcutsEnabled();
}

bool ImageActionAvailability::viewerShortcutsEnabled() const
{
    return m_runtime.viewerShortcutsEnabled();
}

bool ImageActionAvailability::readyShortcutsEnabled() const
{
    return m_runtime.readyShortcutsEnabled();
}

bool ImageActionAvailability::readyViewerShortcutsEnabled() const
{
    return m_runtime.readyViewerShortcutsEnabled();
}

bool ImageActionAvailability::twoPageViewerShortcutsEnabled() const
{
    return m_runtime.twoPageViewerShortcutsEnabled();
}

bool ImageActionAvailability::rightToLeftReadingShortcutsEnabled() const
{
    return m_runtime.rightToLeftReadingShortcutsEnabled();
}

bool ImageActionAvailability::rightToLeftReadingViewerShortcutsEnabled() const
{
    return m_runtime.rightToLeftReadingViewerShortcutsEnabled();
}

bool ImageActionAvailability::rotateShortcutsEnabled() const
{
    return m_runtime.rotateShortcutsEnabled();
}

bool ImageActionAvailability::rotateViewerShortcutsEnabled() const
{
    return m_runtime.rotateViewerShortcutsEnabled();
}

bool ImageActionAvailability::pannableShortcutsEnabled() const
{
    return m_runtime.pannableShortcutsEnabled();
}

bool ImageActionAvailability::pannableViewerShortcutsEnabled() const
{
    return m_runtime.pannableViewerShortcutsEnabled();
}

bool ImageActionAvailability::containerShortcutsEnabled() const
{
    return m_runtime.containerShortcutsEnabled();
}

bool ImageActionAvailability::containerViewerShortcutsEnabled() const
{
    return m_runtime.containerViewerShortcutsEnabled();
}

int ImageActionAvailability::availabilityRevision() const
{
    return m_runtime.availabilityRevision();
}

bool ImageActionAvailability::shortcutsEnabledForScope(
    ImageActionAvailability::ShortcutScope scope) const
{
    return m_runtime.shortcutsEnabledForScope(applicationShortcutScope(scope));
}
