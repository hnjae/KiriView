// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnotifications.h"

#include "presentation/imagezoomstate.h"

namespace KiriView {
std::vector<ImageDocumentChange> imageDocumentSpreadTransitionNotifications()
{
    return { ImageDocumentChange::PresentationTransitionState, ImageDocumentChange::Status,
        ImageDocumentChange::Loading, ImageDocumentChange::Repaint };
}

std::vector<ImageDocumentChange> imageDocumentDisplayedLocationNotifications(
    bool displayedUrlChanged, bool windowTitleFileNameChanged)
{
    std::vector<ImageDocumentChange> changes;
    if (displayedUrlChanged) {
        changes.push_back(ImageDocumentChange::DisplayedUrl);
    }
    if (windowTitleFileNameChanged) {
        changes.push_back(ImageDocumentChange::WindowTitleFileName);
    }
    return changes;
}

std::vector<ImageDocumentChange> imageDocumentTwoPageModeNotifications()
{
    return { ImageDocumentChange::TwoPageMode, ImageDocumentChange::ImageSize,
        ImageDocumentChange::DisplaySize, ImageDocumentChange::ZoomPercent,
        ImageDocumentChange::ZoomMode, ImageDocumentChange::MaximumManualZoomPercent,
        ImageDocumentChange::DisplaySource, ImageDocumentChange::Repaint };
}

std::vector<ImageDocumentChange> imageDocumentSpreadZoomNotifications(
    const ImageZoomChangeSet &changes)
{
    std::vector<ImageDocumentChange> notifications;
    bool repaint = false;
    bool twoPageMode = false;

    if (changes.zoomModeChanged) {
        notifications.push_back(ImageDocumentChange::ZoomMode);
    }
    if (changes.zoomPercentChanged) {
        notifications.push_back(ImageDocumentChange::ZoomPercent);
    }
    if (changes.displaySizeChanged) {
        notifications.push_back(ImageDocumentChange::DisplaySize);
        repaint = true;
        twoPageMode = true;
    }
    if (changes.maximumManualZoomPercentChanged) {
        notifications.push_back(ImageDocumentChange::MaximumManualZoomPercent);
    }
    if (repaint) {
        notifications.push_back(ImageDocumentChange::Repaint);
    }
    if (twoPageMode) {
        notifications.push_back(ImageDocumentChange::TwoPageMode);
    }

    return notifications;
}

std::vector<ImageDocumentChange> imageDocumentRightToLeftReadingNotifications(
    bool secondaryPageVisible)
{
    std::vector<ImageDocumentChange> changes { ImageDocumentChange::RightToLeftReading,
        ImageDocumentChange::Repaint };
    if (secondaryPageVisible) {
        changes.push_back(ImageDocumentChange::TwoPageMode);
    }
    return changes;
}

std::vector<ImageDocumentChange> imageDocumentPresentationZoomNotifications(
    const ImageZoomChangeSet &changes)
{
    std::vector<ImageDocumentChange> notifications;
    if (changes.imageSizeChanged) {
        notifications.push_back(ImageDocumentChange::ImageSize);
    }
    if (changes.viewportSizeChanged) {
        notifications.push_back(ImageDocumentChange::ViewportSize);
    }
    if (changes.zoomModeChanged) {
        notifications.push_back(ImageDocumentChange::ZoomMode);
    }
    if (changes.zoomPercentChanged) {
        notifications.push_back(ImageDocumentChange::ZoomPercent);
    }
    if (changes.displaySizeChanged) {
        notifications.push_back(ImageDocumentChange::DisplaySize);
        notifications.push_back(ImageDocumentChange::Repaint);
    }
    if (changes.maximumManualZoomPercentChanged) {
        notifications.push_back(ImageDocumentChange::MaximumManualZoomPercent);
    }
    return notifications;
}
}
