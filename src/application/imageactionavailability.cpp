// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageactionavailability.h"

#include "kiriview/src/policy/imageactionavailability.cxx.h"

namespace {
KiriView::RustImageActionAvailabilityInput rustImageActionAvailabilityInput(
    const ImageActionAvailabilityInput &input)
{
    return KiriView::RustImageActionAvailabilityInput {
        input.imageReady,
        input.imageCount,
        input.currentPageNumber,
        input.currentLastPageNumber,
        input.fileDeletionInProgress,
        input.helpDialogOpen,
        input.textInputFocused,
        input.imagePannable,
        input.containerNavigationAvailable,
        input.twoPageModeEnabled,
        input.twoPageModeAvailable,
        input.rightToLeftReadingEnabled,
        input.rightToLeftReadingAvailable,
    };
}

ImageActionAvailabilityProjection imageActionAvailabilityProjection(
    const KiriView::RustImageActionAvailabilityProjection &projection)
{
    return ImageActionAvailabilityProjection {
        projection.can_open_next_image,
        projection.can_open_previous_image,
        projection.at_known_first_image,
        projection.at_known_last_image,
        projection.can_use_page_actions,
        projection.can_use_ready_actions,
        projection.can_use_rotate_actions,
        projection.can_use_two_page_mode_actions,
        projection.can_use_right_to_left_reading_actions,
        projection.right_to_left_reading_active,
        projection.two_page_mode_active,
        projection.help_shortcuts_enabled,
        projection.viewer_shortcuts_enabled,
        projection.ready_shortcuts_enabled,
        projection.ready_viewer_shortcuts_enabled,
        projection.image_selection_shortcuts_enabled,
        projection.image_selection_viewer_shortcuts_enabled,
        projection.page_shortcuts_enabled,
        projection.page_viewer_shortcuts_enabled,
        projection.two_page_viewer_shortcuts_enabled,
        projection.right_to_left_reading_shortcuts_enabled,
        projection.right_to_left_reading_viewer_shortcuts_enabled,
        projection.rotate_shortcuts_enabled,
        projection.rotate_viewer_shortcuts_enabled,
        projection.pannable_shortcuts_enabled,
        projection.pannable_viewer_shortcuts_enabled,
        projection.container_shortcuts_enabled,
        projection.container_viewer_shortcuts_enabled,
    };
}
}

ImageActionAvailabilityProjection imageActionAvailabilityProjection(
    const ImageActionAvailabilityInput &input)
{
    return imageActionAvailabilityProjection(
        KiriView::rustImageActionAvailabilityProjection(rustImageActionAvailabilityInput(input)));
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

int ImageActionAvailability::imageCount() const { return m_input.imageCount; }

void ImageActionAvailability::setImageCount(int imageCount)
{
    setInt(m_input.imageCount, imageCount);
}

int ImageActionAvailability::currentPageNumber() const { return m_input.currentPageNumber; }

void ImageActionAvailability::setCurrentPageNumber(int currentPageNumber)
{
    setInt(m_input.currentPageNumber, currentPageNumber);
}

int ImageActionAvailability::currentLastPageNumber() const { return m_input.currentLastPageNumber; }

void ImageActionAvailability::setCurrentLastPageNumber(int currentLastPageNumber)
{
    setInt(m_input.currentLastPageNumber, currentLastPageNumber);
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

bool ImageActionAvailability::canOpenNextImage() const { return m_projection.canOpenNextImage; }

bool ImageActionAvailability::canOpenPreviousImage() const
{
    return m_projection.canOpenPreviousImage;
}

bool ImageActionAvailability::atKnownFirstImage() const { return m_projection.atKnownFirstImage; }

bool ImageActionAvailability::atKnownLastImage() const { return m_projection.atKnownLastImage; }

bool ImageActionAvailability::canUsePageActions() const { return m_projection.canUsePageActions; }

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

bool ImageActionAvailability::imageSelectionShortcutsEnabled() const
{
    return m_projection.imageSelectionShortcutsEnabled;
}

bool ImageActionAvailability::imageSelectionViewerShortcutsEnabled() const
{
    return m_projection.imageSelectionViewerShortcutsEnabled;
}

bool ImageActionAvailability::pageShortcutsEnabled() const
{
    return m_projection.pageShortcutsEnabled;
}

bool ImageActionAvailability::pageViewerShortcutsEnabled() const
{
    return m_projection.pageViewerShortcutsEnabled;
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
    switch (scope) {
    case HelpShortcutScope:
        return m_projection.helpShortcutsEnabled;
    case ViewerShortcutScope:
        return m_projection.viewerShortcutsEnabled;
    case ReadyShortcutScope:
        return m_projection.readyShortcutsEnabled;
    case ReadyViewerShortcutScope:
        return m_projection.readyViewerShortcutsEnabled;
    case ImageSelectionShortcutScope:
        return m_projection.imageSelectionShortcutsEnabled;
    case ImageSelectionViewerShortcutScope:
        return m_projection.imageSelectionViewerShortcutsEnabled;
    case PageShortcutScope:
        return m_projection.pageShortcutsEnabled;
    case PageViewerShortcutScope:
        return m_projection.pageViewerShortcutsEnabled;
    case RightToLeftReadingShortcutScope:
        return m_projection.rightToLeftReadingShortcutsEnabled;
    case RightToLeftReadingViewerShortcutScope:
        return m_projection.rightToLeftReadingViewerShortcutsEnabled;
    case RotateShortcutScope:
        return m_projection.rotateShortcutsEnabled;
    case RotateViewerShortcutScope:
        return m_projection.rotateViewerShortcutsEnabled;
    case PannableShortcutScope:
        return m_projection.pannableShortcutsEnabled;
    case PannableViewerShortcutScope:
        return m_projection.pannableViewerShortcutsEnabled;
    case ContainerShortcutScope:
        return m_projection.containerShortcutsEnabled;
    case ContainerViewerShortcutScope:
        return m_projection.containerViewerShortcutsEnabled;
    }

    return false;
}

void ImageActionAvailability::setBool(bool &target, bool value)
{
    if (target == value) {
        return;
    }
    target = value;
    publishInputChange();
}

void ImageActionAvailability::setInt(int &target, int value)
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
