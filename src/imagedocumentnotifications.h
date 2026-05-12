// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNOTIFICATIONS_H
#define KIRIVIEW_IMAGEDOCUMENTNOTIFICATIONS_H

#include "imagedocumenttypes.h"

#include <vector>

namespace KiriView {
struct ImageZoomChangeSet;

std::vector<ImageDocumentChange> imageDocumentSpreadTransitionNotifications();
std::vector<ImageDocumentChange> imageDocumentDisplayedLocationNotifications(
    bool displayedUrlChanged, bool windowTitleFileNameChanged);
std::vector<ImageDocumentChange> imageDocumentTwoPageModeNotifications();
std::vector<ImageDocumentChange> imageDocumentSpreadZoomNotifications();
std::vector<ImageDocumentChange> imageDocumentRightToLeftReadingNotifications(
    bool secondaryPageVisible);
std::vector<ImageDocumentChange> imageDocumentPresentationZoomNotifications(
    const ImageZoomChangeSet &changes);
}

#endif
