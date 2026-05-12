// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnotifications.h"

#include "imagezoomstate.h"
#include "kiriview/src/imagedocumentnotifications.cxx.h"

namespace {
KiriView::ImageDocumentChange imageDocumentChange(
    KiriView::RustImageDocumentNotificationChange change)
{
    switch (change) {
    case KiriView::RustImageDocumentNotificationChange::Status:
        return KiriView::ImageDocumentChange::Status;
    case KiriView::RustImageDocumentNotificationChange::Loading:
        return KiriView::ImageDocumentChange::Loading;
    case KiriView::RustImageDocumentNotificationChange::ImageSize:
        return KiriView::ImageDocumentChange::ImageSize;
    case KiriView::RustImageDocumentNotificationChange::ViewportSize:
        return KiriView::ImageDocumentChange::ViewportSize;
    case KiriView::RustImageDocumentNotificationChange::DisplaySize:
        return KiriView::ImageDocumentChange::DisplaySize;
    case KiriView::RustImageDocumentNotificationChange::ZoomPercent:
        return KiriView::ImageDocumentChange::ZoomPercent;
    case KiriView::RustImageDocumentNotificationChange::ZoomMode:
        return KiriView::ImageDocumentChange::ZoomMode;
    case KiriView::RustImageDocumentNotificationChange::MaximumManualZoomPercent:
        return KiriView::ImageDocumentChange::MaximumManualZoomPercent;
    case KiriView::RustImageDocumentNotificationChange::TwoPageMode:
        return KiriView::ImageDocumentChange::TwoPageMode;
    case KiriView::RustImageDocumentNotificationChange::RightToLeftReading:
        return KiriView::ImageDocumentChange::RightToLeftReading;
    case KiriView::RustImageDocumentNotificationChange::Repaint:
        return KiriView::ImageDocumentChange::Repaint;
    case KiriView::RustImageDocumentNotificationChange::DisplayedUrl:
        return KiriView::ImageDocumentChange::DisplayedUrl;
    case KiriView::RustImageDocumentNotificationChange::WindowTitleFileName:
        return KiriView::ImageDocumentChange::WindowTitleFileName;
    }

    return KiriView::ImageDocumentChange::Repaint;
}

std::vector<KiriView::ImageDocumentChange> imageDocumentChanges(
    rust::Vec<KiriView::RustImageDocumentNotificationChange> rustChanges)
{
    std::vector<KiriView::ImageDocumentChange> changes;
    changes.reserve(rustChanges.size());
    for (KiriView::RustImageDocumentNotificationChange change : rustChanges) {
        changes.push_back(imageDocumentChange(change));
    }
    return changes;
}

KiriView::RustImageDocumentZoomChangeSet rustImageDocumentZoomChangeSet(
    const KiriView::ImageZoomChangeSet &changes)
{
    return KiriView::RustImageDocumentZoomChangeSet { changes.imageSizeChanged,
        changes.viewportSizeChanged, changes.zoomModeChanged, changes.zoomPercentChanged,
        changes.displaySizeChanged, changes.maximumManualZoomPercentChanged };
}
}

namespace KiriView {
std::vector<ImageDocumentChange> imageDocumentSpreadTransitionNotifications()
{
    return imageDocumentChanges(rustImageDocumentSpreadTransitionNotifications());
}

std::vector<ImageDocumentChange> imageDocumentDisplayedLocationNotifications(
    bool displayedUrlChanged, bool windowTitleFileNameChanged)
{
    return imageDocumentChanges(rustImageDocumentDisplayedLocationNotifications(
        displayedUrlChanged, windowTitleFileNameChanged));
}

std::vector<ImageDocumentChange> imageDocumentTwoPageModeNotifications()
{
    return imageDocumentChanges(rustImageDocumentTwoPageModeNotifications());
}

std::vector<ImageDocumentChange> imageDocumentSpreadZoomNotifications()
{
    return imageDocumentChanges(rustImageDocumentSpreadZoomNotifications());
}

std::vector<ImageDocumentChange> imageDocumentRightToLeftReadingNotifications(
    bool secondaryPageVisible)
{
    return imageDocumentChanges(
        rustImageDocumentRightToLeftReadingNotifications(secondaryPageVisible));
}

std::vector<ImageDocumentChange> imageDocumentPresentationZoomNotifications(
    const ImageZoomChangeSet &changes)
{
    return imageDocumentChanges(
        rustImageDocumentPresentationZoomNotifications(rustImageDocumentZoomChangeSet(changes)));
}
}
