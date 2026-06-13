// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTTYPES_H
#define KIRIVIEW_IMAGEDOCUMENTTYPES_H

namespace kiriview {
enum class ImageDocumentStatus {
    Null,
    Loading,
    Ready,
    Error,
};

enum class ImageDocumentChange {
    SourceUrl,
    Status,
    Loading,
    ErrorString,
    WindowTitleFileName,
    DisplayedUrl,
    ImageSize,
    ViewportSize,
    ViewportFrame,
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
    PresentationTransitionState,
    Rotation,
    UnsupportedOpenedCollectionVideo,
    EmbeddedMetadata,
    DisplaySource,
};
}

#endif
