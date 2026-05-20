// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailability.h"

ImageActionAvailability::ImageActionAvailability(QObject *parent)
    : QObject(parent)
{
}

bool ImageActionAvailability::imageReady() const { return m_imageReady; }

void ImageActionAvailability::setImageReady(bool imageReady) { setBool(m_imageReady, imageReady); }

int ImageActionAvailability::imageCount() const { return m_imageCount; }

void ImageActionAvailability::setImageCount(int imageCount) { setInt(m_imageCount, imageCount); }

int ImageActionAvailability::currentPageNumber() const { return m_currentPageNumber; }

void ImageActionAvailability::setCurrentPageNumber(int currentPageNumber)
{
    setInt(m_currentPageNumber, currentPageNumber);
}

int ImageActionAvailability::currentLastPageNumber() const { return m_currentLastPageNumber; }

void ImageActionAvailability::setCurrentLastPageNumber(int currentLastPageNumber)
{
    setInt(m_currentLastPageNumber, currentLastPageNumber);
}

bool ImageActionAvailability::fileDeletionInProgress() const { return m_fileDeletionInProgress; }

void ImageActionAvailability::setFileDeletionInProgress(bool fileDeletionInProgress)
{
    setBool(m_fileDeletionInProgress, fileDeletionInProgress);
}

bool ImageActionAvailability::helpDialogOpen() const { return m_helpDialogOpen; }

void ImageActionAvailability::setHelpDialogOpen(bool helpDialogOpen)
{
    setBool(m_helpDialogOpen, helpDialogOpen);
}

bool ImageActionAvailability::textInputFocused() const { return m_textInputFocused; }

void ImageActionAvailability::setTextInputFocused(bool textInputFocused)
{
    setBool(m_textInputFocused, textInputFocused);
}

bool ImageActionAvailability::imagePannable() const { return m_imagePannable; }

void ImageActionAvailability::setImagePannable(bool imagePannable)
{
    setBool(m_imagePannable, imagePannable);
}

bool ImageActionAvailability::imageHorizontallyPannable() const
{
    return m_imageHorizontallyPannable;
}

void ImageActionAvailability::setImageHorizontallyPannable(bool imageHorizontallyPannable)
{
    setBool(m_imageHorizontallyPannable, imageHorizontallyPannable);
}

bool ImageActionAvailability::containerNavigationAvailable() const
{
    return m_containerNavigationAvailable;
}

void ImageActionAvailability::setContainerNavigationAvailable(bool containerNavigationAvailable)
{
    setBool(m_containerNavigationAvailable, containerNavigationAvailable);
}

bool ImageActionAvailability::twoPageModeEnabled() const { return m_twoPageModeEnabled; }

void ImageActionAvailability::setTwoPageModeEnabled(bool twoPageModeEnabled)
{
    setBool(m_twoPageModeEnabled, twoPageModeEnabled);
}

bool ImageActionAvailability::twoPageModeAvailable() const { return m_twoPageModeAvailable; }

void ImageActionAvailability::setTwoPageModeAvailable(bool twoPageModeAvailable)
{
    setBool(m_twoPageModeAvailable, twoPageModeAvailable);
}

bool ImageActionAvailability::rightToLeftReadingEnabled() const
{
    return m_rightToLeftReadingEnabled;
}

void ImageActionAvailability::setRightToLeftReadingEnabled(bool rightToLeftReadingEnabled)
{
    setBool(m_rightToLeftReadingEnabled, rightToLeftReadingEnabled);
}

bool ImageActionAvailability::rightToLeftReadingAvailable() const
{
    return m_rightToLeftReadingAvailable;
}

void ImageActionAvailability::setRightToLeftReadingAvailable(bool rightToLeftReadingAvailable)
{
    setBool(m_rightToLeftReadingAvailable, rightToLeftReadingAvailable);
}

bool ImageActionAvailability::canOpenNextImage() const
{
    return m_currentPageNumber > 0 && m_currentLastPageNumber < m_imageCount;
}

bool ImageActionAvailability::canOpenPreviousImage() const { return m_currentPageNumber > 1; }

bool ImageActionAvailability::atKnownFirstImage() const
{
    return m_imageCount > 0 && m_currentPageNumber == 1;
}

bool ImageActionAvailability::atKnownLastImage() const
{
    return m_imageCount > 0 && m_currentPageNumber > 0 && m_currentLastPageNumber > 0
        && m_currentLastPageNumber >= m_imageCount;
}

bool ImageActionAvailability::canUsePageActions() const
{
    return m_imageCount > 0 && m_currentPageNumber > 0 && !m_fileDeletionInProgress
        && !m_helpDialogOpen;
}

bool ImageActionAvailability::canUseReadyActions() const
{
    return m_imageReady && !m_fileDeletionInProgress && !m_helpDialogOpen;
}

bool ImageActionAvailability::canUseRotateActions() const
{
    return canUseReadyActions() && !twoPageModeActive();
}

bool ImageActionAvailability::canUseTwoPageModeActions() const
{
    return canUseReadyActions() && m_twoPageModeAvailable;
}

bool ImageActionAvailability::canUseRightToLeftReadingActions() const
{
    return canUseReadyActions() && m_rightToLeftReadingAvailable;
}

bool ImageActionAvailability::rightToLeftReadingActive() const
{
    return m_rightToLeftReadingEnabled && m_rightToLeftReadingAvailable;
}

bool ImageActionAvailability::twoPageModeActive() const
{
    return m_twoPageModeEnabled && m_twoPageModeAvailable;
}

bool ImageActionAvailability::helpShortcutsEnabled() const { return !m_helpDialogOpen; }

bool ImageActionAvailability::viewerShortcutsEnabled() const
{
    return !m_textInputFocused && helpShortcutsEnabled();
}

bool ImageActionAvailability::readyShortcutsEnabled() const
{
    return m_imageReady && !m_fileDeletionInProgress && helpShortcutsEnabled();
}

bool ImageActionAvailability::readyViewerShortcutsEnabled() const
{
    return m_imageReady && !m_fileDeletionInProgress && viewerShortcutsEnabled();
}

bool ImageActionAvailability::imageSelectionShortcutsEnabled() const
{
    return m_imageCount > 0 && m_currentPageNumber > 0 && !m_fileDeletionInProgress
        && helpShortcutsEnabled();
}

bool ImageActionAvailability::imageSelectionViewerShortcutsEnabled() const
{
    return m_imageCount > 0 && m_currentPageNumber > 0 && !m_fileDeletionInProgress
        && viewerShortcutsEnabled();
}

bool ImageActionAvailability::pageShortcutsEnabled() const
{
    return imageSelectionShortcutsEnabled();
}

bool ImageActionAvailability::pageViewerShortcutsEnabled() const
{
    return imageSelectionViewerShortcutsEnabled();
}

bool ImageActionAvailability::twoPageViewerShortcutsEnabled() const
{
    return imageSelectionViewerShortcutsEnabled() && twoPageModeActive();
}

bool ImageActionAvailability::rightToLeftReadingShortcutsEnabled() const
{
    return readyShortcutsEnabled() && m_rightToLeftReadingAvailable;
}

bool ImageActionAvailability::rightToLeftReadingViewerShortcutsEnabled() const
{
    return readyViewerShortcutsEnabled() && m_rightToLeftReadingAvailable;
}

bool ImageActionAvailability::rotateShortcutsEnabled() const
{
    return readyShortcutsEnabled() && !twoPageModeActive();
}

bool ImageActionAvailability::rotateViewerShortcutsEnabled() const
{
    return readyViewerShortcutsEnabled() && !twoPageModeActive();
}

bool ImageActionAvailability::pannableShortcutsEnabled() const
{
    return m_imagePannable && !m_fileDeletionInProgress && helpShortcutsEnabled();
}

bool ImageActionAvailability::pannableViewerShortcutsEnabled() const
{
    return m_imagePannable && !m_fileDeletionInProgress && viewerShortcutsEnabled();
}

bool ImageActionAvailability::containerShortcutsEnabled() const
{
    return m_containerNavigationAvailable && !m_fileDeletionInProgress && helpShortcutsEnabled();
}

bool ImageActionAvailability::containerViewerShortcutsEnabled() const
{
    return m_containerNavigationAvailable && !m_fileDeletionInProgress && viewerShortcutsEnabled();
}

void ImageActionAvailability::setBool(bool &target, bool value)
{
    if (target == value) {
        return;
    }
    target = value;
    Q_EMIT availabilityChanged();
}

void ImageActionAvailability::setInt(int &target, int value)
{
    if (target == value) {
        return;
    }
    target = value;
    Q_EMIT availabilityChanged();
}
