// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnotifications.h"

#include "imagezoomstate.h"

namespace KiriView {
std::vector<ImageDocumentPublicSignal> imageDocumentPublicSignals(ImageDocumentChange change)
{
    switch (change) {
    case ImageDocumentChange::SourceUrl:
        return { ImageDocumentPublicSignal::SourceUrl };
    case ImageDocumentChange::Status:
        return { ImageDocumentPublicSignal::Status };
    case ImageDocumentChange::Loading:
        return { ImageDocumentPublicSignal::Loading };
    case ImageDocumentChange::ErrorString:
        return { ImageDocumentPublicSignal::ErrorString };
    case ImageDocumentChange::WindowTitleFileName:
        return { ImageDocumentPublicSignal::WindowTitleFileName };
    case ImageDocumentChange::DisplayedUrl:
        return { ImageDocumentPublicSignal::DisplayedUrl };
    case ImageDocumentChange::ImageSize:
        return { ImageDocumentPublicSignal::ImageSize };
    case ImageDocumentChange::ViewportSize:
        return { ImageDocumentPublicSignal::ViewportSize };
    case ImageDocumentChange::VisibleItemRect:
        return { ImageDocumentPublicSignal::VisibleItemRect };
    case ImageDocumentChange::DisplaySize:
        return { ImageDocumentPublicSignal::DisplaySize };
    case ImageDocumentChange::ZoomPercent:
        return { ImageDocumentPublicSignal::ZoomPercent };
    case ImageDocumentChange::ZoomMode:
        return { ImageDocumentPublicSignal::ZoomMode };
    case ImageDocumentChange::MaximumManualZoomPercent:
        return { ImageDocumentPublicSignal::MaximumManualZoomPercent };
    case ImageDocumentChange::PageNavigation:
        return { ImageDocumentPublicSignal::PageNavigation };
    case ImageDocumentChange::ContainerNavigation:
        return { ImageDocumentPublicSignal::ContainerNavigation };
    case ImageDocumentChange::FileDeletionInProgress:
        return { ImageDocumentPublicSignal::FileDeletionInProgress };
    case ImageDocumentChange::TwoPageMode:
        return { ImageDocumentPublicSignal::TwoPageMode,
            ImageDocumentPublicSignal::PageNavigation };
    case ImageDocumentChange::RightToLeftReading:
        return { ImageDocumentPublicSignal::RightToLeftReading };
    case ImageDocumentChange::Rotation:
        return { ImageDocumentPublicSignal::RotationDegrees };
    case ImageDocumentChange::Repaint:
        return { ImageDocumentPublicSignal::Repaint };
    }

    return {};
}

std::vector<ImageDocumentChange> imageDocumentSpreadTransitionNotifications()
{
    return { ImageDocumentChange::Status, ImageDocumentChange::Loading,
        ImageDocumentChange::Repaint };
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
        ImageDocumentChange::Repaint };
}

std::vector<ImageDocumentChange> imageDocumentSpreadZoomNotifications()
{
    return { ImageDocumentChange::ZoomMode, ImageDocumentChange::ZoomPercent,
        ImageDocumentChange::DisplaySize, ImageDocumentChange::MaximumManualZoomPercent,
        ImageDocumentChange::Repaint, ImageDocumentChange::TwoPageMode };
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
