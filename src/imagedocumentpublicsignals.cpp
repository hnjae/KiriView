// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpublicsignals.h"

#include <utility>

namespace {
template <typename Operation> void run(const Operation &operation)
{
    if (operation) {
        operation();
    }
}
}

namespace KiriView {
ImageDocumentPublicSignalEmitter::ImageDocumentPublicSignalEmitter(
    ImageDocumentPublicSignalOperations operations)
    : m_operations(std::move(operations))
{
}

void ImageDocumentPublicSignalEmitter::emitChange(ImageDocumentChange change) const
{
    for (ImageDocumentPublicSignal signal : imageDocumentPublicSignals(change)) {
        emitSignal(signal);
    }
}

void ImageDocumentPublicSignalEmitter::emitSignal(ImageDocumentPublicSignal signal) const
{
    switch (signal) {
    case ImageDocumentPublicSignal::SourceUrl:
        run(m_operations.sourceUrlChanged);
        return;
    case ImageDocumentPublicSignal::Status:
        run(m_operations.statusChanged);
        return;
    case ImageDocumentPublicSignal::Loading:
        run(m_operations.loadingChanged);
        return;
    case ImageDocumentPublicSignal::ErrorString:
        run(m_operations.errorStringChanged);
        return;
    case ImageDocumentPublicSignal::WindowTitleFileName:
        run(m_operations.windowTitleFileNameChanged);
        return;
    case ImageDocumentPublicSignal::DisplayedUrl:
        run(m_operations.displayedUrlChanged);
        return;
    case ImageDocumentPublicSignal::ImageSize:
        run(m_operations.imageSizeChanged);
        return;
    case ImageDocumentPublicSignal::ViewportSize:
        run(m_operations.viewportSizeChanged);
        return;
    case ImageDocumentPublicSignal::VisibleItemRect:
        run(m_operations.visibleItemRectChanged);
        return;
    case ImageDocumentPublicSignal::DisplaySize:
        run(m_operations.displaySizeChanged);
        return;
    case ImageDocumentPublicSignal::ZoomPercent:
        run(m_operations.zoomPercentChanged);
        return;
    case ImageDocumentPublicSignal::ZoomMode:
        run(m_operations.zoomModeChanged);
        return;
    case ImageDocumentPublicSignal::MaximumManualZoomPercent:
        run(m_operations.maximumManualZoomPercentChanged);
        return;
    case ImageDocumentPublicSignal::PageNavigation:
        run(m_operations.pageNavigationChanged);
        return;
    case ImageDocumentPublicSignal::ContainerNavigation:
        run(m_operations.containerNavigationChanged);
        return;
    case ImageDocumentPublicSignal::FileDeletionInProgress:
        run(m_operations.fileDeletionInProgressChanged);
        return;
    case ImageDocumentPublicSignal::TwoPageMode:
        run(m_operations.twoPageModeChanged);
        return;
    case ImageDocumentPublicSignal::RightToLeftReading:
        run(m_operations.rightToLeftReadingChanged);
        return;
    case ImageDocumentPublicSignal::RotationDegrees:
        run(m_operations.rotationDegreesChanged);
        return;
    case ImageDocumentPublicSignal::Repaint:
        run(m_operations.repaintRequested);
        return;
    }
}

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
}
