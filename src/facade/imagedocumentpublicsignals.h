// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPUBLICSIGNALS_H
#define KIRIVIEW_IMAGEDOCUMENTPUBLICSIGNALS_H

#include "document/imagedocumenttypes.h"

#include <functional>
#include <vector>

namespace KiriView {
enum class ImageDocumentPublicSignal {
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
    ZoomPercentKnown,
    ZoomPercent,
    ZoomMode,
    MaximumManualZoomPercent,
    PageNavigation,
    ContainerNavigation,
    FileDeletionInProgress,
    TwoPageMode,
    RightToLeftReading,
    PresentationTransitionState,
    RotationDegrees,
    ImageDocumentSourceScope,
    UnsupportedOpenedCollectionVideo,
    EmbeddedMetadata,
    DisplaySource,
    Repaint,
};

struct ImageDocumentPublicSignalOperations {
    std::function<void()> sourceUrlChanged;
    std::function<void()> statusChanged;
    std::function<void()> loadingChanged;
    std::function<void()> errorStringChanged;
    std::function<void()> windowTitleFileNameChanged;
    std::function<void()> displayedUrlChanged;
    std::function<void()> imageSizeChanged;
    std::function<void()> viewportSizeChanged;
    std::function<void()> viewportFrameChanged;
    std::function<void()> visibleItemRectChanged;
    std::function<void()> displaySizeChanged;
    std::function<void()> zoomPercentKnownChanged;
    std::function<void()> zoomPercentChanged;
    std::function<void()> zoomModeChanged;
    std::function<void()> maximumManualZoomPercentChanged;
    std::function<void()> pageNavigationChanged;
    std::function<void()> containerNavigationChanged;
    std::function<void()> fileDeletionInProgressChanged;
    std::function<void()> twoPageModeChanged;
    std::function<void()> rightToLeftReadingChanged;
    std::function<void()> presentationTransitionStateChanged;
    std::function<void()> rotationDegreesChanged;
    std::function<void()> imageDocumentSourceScopeChanged;
    std::function<void()> unsupportedOpenedCollectionVideoChanged;
    std::function<void()> embeddedMetadataChanged;
    std::function<void()> displaySourceChanged;
    std::function<void()> repaintRequested;
};

class ImageDocumentPublicSignalEmitter final
{
public:
    explicit ImageDocumentPublicSignalEmitter(ImageDocumentPublicSignalOperations operations);

    void emitChanges(const std::vector<ImageDocumentChange> &changes) const;
    void emitChange(ImageDocumentChange change) const;
    void emitSignal(ImageDocumentPublicSignal signal) const;

private:
    ImageDocumentPublicSignalOperations m_operations;
};

std::vector<ImageDocumentPublicSignal> imageDocumentPublicSignalsForChanges(
    const std::vector<ImageDocumentChange> &changes);
std::vector<ImageDocumentPublicSignal> imageDocumentPublicSignals(ImageDocumentChange change);
}

#endif
