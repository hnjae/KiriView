// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationviewportcontroller.h"

#include "imagecallback.h"
#include "imagetiledecodescheduler.h"

#include <utility>

namespace KiriView {
ImagePresentationViewportController::ImagePresentationViewportController(QObject *context,
    RenderContextProvider renderContextProvider, ImageSurfaceProvider imageSurfaceProvider,
    TileDecodedCallback tileDecoded, ChangeCallback changeCallback)
    : m_state(std::move(renderContextProvider))
    , m_imageSurfaceProvider(std::move(imageSurfaceProvider))
    , m_changeCallback(std::move(changeCallback))
{
    m_tileDecodeScheduler
        = std::make_unique<ImageTileDecodeScheduler>(context, std::move(tileDecoded));
}

ImagePresentationViewportController::~ImagePresentationViewportController() = default;

QSize ImagePresentationViewportController::imageSize() const { return m_state.imageSize(); }

QSizeF ImagePresentationViewportController::viewportSize() const { return m_state.viewportSize(); }

QSizeF ImagePresentationViewportController::displaySize() const { return m_state.displaySize(); }

QRectF ImagePresentationViewportController::visibleItemRect() const
{
    return m_state.visibleItemRect();
}

qreal ImagePresentationViewportController::zoomPercent() const { return m_state.zoomPercent(); }

ImageZoomMode ImagePresentationViewportController::zoomMode() const { return m_state.zoomMode(); }

qreal ImagePresentationViewportController::maximumManualZoomPercent() const
{
    return m_state.maximumManualZoomPercent();
}

qreal ImagePresentationViewportController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_state.clampedManualZoomPercent(zoomPercent);
}

qreal ImagePresentationViewportController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_state.steppedManualZoomPercent(stepCount);
}

int ImagePresentationViewportController::rotationDegrees() const
{
    return m_state.rotationDegrees();
}

ImageDocumentRenderContext ImagePresentationViewportController::renderContext() const
{
    return m_state.renderContext();
}

ImageFirstDisplayDecodeContext
ImagePresentationViewportController::firstDisplayDecodeContext() const
{
    return m_state.firstDisplayDecodeContext();
}

void ImagePresentationViewportController::setViewportSize(const QSizeF &viewportSize)
{
    applyPlan(m_state.setViewportSize(viewportSize));
}

void ImagePresentationViewportController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    applyPlan(m_state.setVisibleItemRect(visibleItemRect));
}

void ImagePresentationViewportController::setZoomPercent(qreal zoomPercent)
{
    applyPlan(m_state.setZoomPercent(zoomPercent));
}

void ImagePresentationViewportController::resetZoom() { applyPlan(m_state.resetZoom()); }

void ImagePresentationViewportController::setFitMode(ImageZoomMode zoomMode)
{
    applyPlan(m_state.setFitMode(zoomMode));
}

bool ImagePresentationViewportController::resetRotation()
{
    ImagePresentationViewportPlan plan = m_state.resetRotation();
    const bool changed = !plan.changes.empty() || plan.scheduleVisibleTileDecode;
    applyPlan(plan);
    return changed;
}

void ImagePresentationViewportController::resetRotationForNewImage()
{
    applyPlan(m_state.resetRotationForNewImage());
}

void ImagePresentationViewportController::rotateClockwise()
{
    applyPlan(m_state.rotateClockwise());
}

void ImagePresentationViewportController::rotateCounterclockwise()
{
    applyPlan(m_state.rotateCounterclockwise());
}

void ImagePresentationViewportController::updateRenderContext()
{
    applyPlan(m_state.updateRenderContext());
}

void ImagePresentationViewportController::prepareImageContainer(const QUrl &containerUrl)
{
    applyPlan(m_state.prepareImageContainer(containerUrl));
}

void ImagePresentationViewportController::prepareFailedContainer(const QUrl &containerUrl)
{
    applyPlan(m_state.prepareFailedContainer(containerUrl));
}

void ImagePresentationViewportController::clearContainer() { m_state.clearContainer(); }

void ImagePresentationViewportController::setDisplayedImageSize(const QSize &imageSize)
{
    applyPlan(m_state.setDisplayedImageSize(imageSize));
}

void ImagePresentationViewportController::invalidateTiles() { m_tileDecodeScheduler->invalidate(); }

void ImagePresentationViewportController::scheduleVisibleTileDecode()
{
    m_tileDecodeScheduler->schedule(
        imageSurface(), displaySize(), visibleItemRect(), renderContext(), rotationDegrees());
}

void ImagePresentationViewportController::applyPlan(const ImagePresentationViewportPlan &plan)
{
    for (ImageDocumentChange change : plan.changes) {
        notify(change);
    }

    if (plan.scheduleVisibleTileDecode) {
        scheduleVisibleTileDecode();
    }
}

std::shared_ptr<DisplayedImageSurface> ImagePresentationViewportController::imageSurface() const
{
    return m_imageSurfaceProvider ? m_imageSurfaceProvider() : nullptr;
}

void ImagePresentationViewportController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_changeCallback, change);
}
}
