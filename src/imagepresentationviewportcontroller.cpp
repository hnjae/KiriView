// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationviewportcontroller.h"

#include "imagecallback.h"
#include "imagedocumentnotifications.h"
#include "imagerendering.h"
#include "imagetiledecodescheduler.h"

#include <utility>

namespace KiriView {
ImagePresentationViewportController::ImagePresentationViewportController(QObject *context,
    RenderContextProvider renderContextProvider, ImageSurfaceProvider imageSurfaceProvider,
    TileDecodedCallback tileDecoded, ChangeCallback changeCallback)
    : m_renderContextProvider(std::move(renderContextProvider))
    , m_imageSurfaceProvider(std::move(imageSurfaceProvider))
    , m_changeCallback(std::move(changeCallback))
{
    m_renderContext = renderContext();
    m_tileDecodeScheduler
        = std::make_unique<ImageTileDecodeScheduler>(context, std::move(tileDecoded));
}

ImagePresentationViewportController::~ImagePresentationViewportController() = default;

QSize ImagePresentationViewportController::imageSize() const
{
    return m_geometry.logicalImageSize();
}

QSizeF ImagePresentationViewportController::viewportSize() const
{
    return m_zoomState.viewportSize();
}

QSizeF ImagePresentationViewportController::displaySize() const
{
    return m_zoomState.displaySize();
}

QRectF ImagePresentationViewportController::visibleItemRect() const { return m_visibleItemRect; }

qreal ImagePresentationViewportController::zoomPercent() const { return m_zoomState.zoomPercent(); }

ImageZoomMode ImagePresentationViewportController::zoomMode() const
{
    return m_zoomState.zoomMode();
}

qreal ImagePresentationViewportController::maximumManualZoomPercent() const
{
    return m_zoomState.maximumManualZoomPercent(renderContext().devicePixelRatio);
}

qreal ImagePresentationViewportController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_zoomState.clampedManualZoomPercent(zoomPercent, renderContext().devicePixelRatio);
}

qreal ImagePresentationViewportController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_zoomState.steppedManualZoomPercent(stepCount, renderContext().devicePixelRatio);
}

int ImagePresentationViewportController::rotationDegrees() const
{
    return m_geometry.rotationDegrees();
}

ImageDocumentRenderContext ImagePresentationViewportController::renderContext() const
{
    ImageDocumentRenderContext context
        = m_renderContextProvider ? m_renderContextProvider() : ImageDocumentRenderContext {};
    return normalizedImageDocumentRenderContext(context);
}

ImageFirstDisplayDecodeContext
ImagePresentationViewportController::firstDisplayDecodeContext() const
{
    const ImageDocumentRenderContext context = renderContext();
    return imageFirstDisplayDecodeContext(viewportSize(), context.devicePixelRatio);
}

void ImagePresentationViewportController::setViewportSize(const QSizeF &viewportSize)
{
    mutateZoomState([&viewportSize](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setViewportSize(viewportSize, devicePixelRatio);
    });
}

void ImagePresentationViewportController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    if (m_visibleItemRect == visibleItemRect) {
        return;
    }

    m_visibleItemRect = visibleItemRect;
    notify(ImageDocumentChange::VisibleItemRect);
    scheduleVisibleTileDecode();
}

void ImagePresentationViewportController::setZoomPercent(qreal zoomPercent)
{
    mutateZoomState([zoomPercent](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setManualZoomPercent(zoomPercent, devicePixelRatio);
    });
}

void ImagePresentationViewportController::resetZoom()
{
    mutateZoomState([](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.resetZoom(devicePixelRatio);
    });
}

void ImagePresentationViewportController::setFitMode(ImageZoomMode zoomMode)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return;
    }

    mutateZoomState([zoomMode](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setFitMode(zoomMode, devicePixelRatio);
    });
}

bool ImagePresentationViewportController::resetRotation()
{
    if (!m_geometry.resetRotation()) {
        return false;
    }

    applyGeometryRotationChange();
    return true;
}

void ImagePresentationViewportController::resetRotationForNewImage()
{
    if (!m_geometry.resetRotation()) {
        return;
    }

    notify(ImageDocumentChange::Rotation);
}

void ImagePresentationViewportController::rotateClockwise()
{
    if (m_geometry.rotateClockwise()) {
        applyGeometryRotationChange();
    }
}

void ImagePresentationViewportController::rotateCounterclockwise()
{
    if (m_geometry.rotateCounterclockwise()) {
        applyGeometryRotationChange();
    }
}

void ImagePresentationViewportController::updateRenderContext()
{
    mutateZoomState([](ImageZoomState &zoomState,
                        qreal devicePixelRatio) { zoomState.update(devicePixelRatio); },
        TileRefresh::Always);
}

void ImagePresentationViewportController::prepareImageContainer(const QUrl &containerUrl)
{
    mutateZoomState([&containerUrl](ImageZoomState &zoomState, qreal) {
        zoomState.prepareImageContainer(containerUrl);
    });
}

void ImagePresentationViewportController::prepareFailedContainer(const QUrl &containerUrl)
{
    mutateZoomState([&containerUrl](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.clearContainer();
        zoomState.prepareImageContainer(containerUrl);
        zoomState.resetZoom(devicePixelRatio);
    });
}

void ImagePresentationViewportController::clearContainer() { m_zoomState.clearContainer(); }

void ImagePresentationViewportController::setDisplayedImageSize(const QSize &imageSize)
{
    m_geometry.setSourceImageSize(imageSize);
    applyGeometryImageSize();
}

void ImagePresentationViewportController::invalidateTiles() { m_tileDecodeScheduler->invalidate(); }

void ImagePresentationViewportController::scheduleVisibleTileDecode()
{
    m_tileDecodeScheduler->schedule(
        imageSurface(), displaySize(), m_visibleItemRect, renderContext(), rotationDegrees());
}

void ImagePresentationViewportController::applyGeometryRotationChange()
{
    applyGeometryImageSize(TileRefresh::Always);
    notify(ImageDocumentChange::Rotation);
    notify(ImageDocumentChange::Repaint);
}

void ImagePresentationViewportController::applyGeometryImageSize(TileRefresh tileRefresh)
{
    const QSize logicalImageSize = m_geometry.logicalImageSize();
    mutateZoomState(
        [&logicalImageSize](ImageZoomState &zoomState, qreal devicePixelRatio) {
            zoomState.setImageSize(logicalImageSize, devicePixelRatio);
        },
        tileRefresh);
}

void ImagePresentationViewportController::mutateZoomState(
    const ZoomStateMutation &mutation, TileRefresh tileRefresh)
{
    const ImageDocumentRenderContext previousContext = m_renderContext;
    const ImageDocumentRenderContext context = renderContext();
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    mutation(m_zoomState, context.devicePixelRatio);

    applyZoomStateChanges(previous, previousContext, context, tileRefresh);
    m_renderContext = context;
}

void ImagePresentationViewportController::applyZoomStateChanges(const ImageZoomSnapshot &previous,
    const ImageDocumentRenderContext &previousContext, const ImageDocumentRenderContext &context,
    TileRefresh tileRefresh)
{
    const ImageZoomSnapshot current = m_zoomState.snapshot();
    const ImageZoomChangeSet changes
        = ImageZoomState::changeSet(previous, previousContext.devicePixelRatio, current,
            context.devicePixelRatio, tileRefresh == TileRefresh::Always);
    for (ImageDocumentChange change : imageDocumentPresentationZoomNotifications(changes)) {
        notify(change);
    }

    if (changes.scheduleVisibleTileDecode) {
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
