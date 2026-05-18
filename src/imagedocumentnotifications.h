// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNOTIFICATIONS_H
#define KIRIVIEW_IMAGEDOCUMENTNOTIFICATIONS_H

#include "imagedocumenttypes.h"

#include <vector>

namespace KiriView {
struct ImageZoomChangeSet;

enum class ImageDocumentPublicSignal {
    SourceUrl,
    Status,
    Loading,
    ErrorString,
    WindowTitleFileName,
    DisplayedUrl,
    ImageSize,
    ViewportSize,
    VisibleItemRect,
    DisplaySize,
    ZoomPercent,
    ZoomMode,
    MaximumManualZoomPercent,
    PageNavigation,
    ContainerNavigation,
    FileDeletionInProgress,
    TwoPageMode,
    RightToLeftReading,
    RotationDegrees,
    Repaint,
};

std::vector<ImageDocumentChange> imageDocumentSpreadTransitionNotifications();
std::vector<ImageDocumentChange> imageDocumentDisplayedLocationNotifications(
    bool displayedUrlChanged, bool windowTitleFileNameChanged);
std::vector<ImageDocumentChange> imageDocumentTwoPageModeNotifications();
std::vector<ImageDocumentChange> imageDocumentSpreadZoomNotifications();
std::vector<ImageDocumentChange> imageDocumentRightToLeftReadingNotifications(
    bool secondaryPageVisible);
std::vector<ImageDocumentChange> imageDocumentPresentationZoomNotifications(
    const ImageZoomChangeSet &changes);
std::vector<ImageDocumentPublicSignal> imageDocumentPublicSignals(ImageDocumentChange change);
}

#endif
