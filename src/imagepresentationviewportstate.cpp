// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationviewportstate.h"

#include "imagedocumentnotifications.h"
#include "imagerendering.h"

#include <utility>

namespace KiriView {
ImagePresentationViewportState::ImagePresentationViewportState(
    RenderContextProvider renderContextProvider)
    : m_renderContextState(std::move(renderContextProvider))
{
}

QSize ImagePresentationViewportState::imageSize() const { return m_geometry.logicalImageSize(); }

QSizeF ImagePresentationViewportState::viewportSize() const { return m_zoomState.viewportSize(); }

QSizeF ImagePresentationViewportState::displaySize() const { return m_zoomState.displaySize(); }

const QRectF &ImagePresentationViewportState::visibleItemRect() const { return m_visibleItemRect; }

qreal ImagePresentationViewportState::zoomPercent() const { return m_zoomState.zoomPercent(); }

ImageZoomMode ImagePresentationViewportState::zoomMode() const { return m_zoomState.zoomMode(); }

int ImagePresentationViewportState::rotationDegrees() const { return m_geometry.rotationDegrees(); }

ImageDocumentRenderContext ImagePresentationViewportState::renderContext() const
{
    return m_renderContextState.current();
}

ImageFirstDisplayDecodeContext ImagePresentationViewportState::firstDisplayDecodeContext() const
{
    const ImageDocumentRenderContext context = renderContext();
    return imageFirstDisplayDecodeContext(viewportSize(), context.devicePixelRatio);
}

qreal ImagePresentationViewportState::maximumManualZoomPercent() const
{
    return m_zoomState.maximumManualZoomPercent(renderContext().devicePixelRatio);
}

qreal ImagePresentationViewportState::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_zoomState.clampedManualZoomPercent(zoomPercent, renderContext().devicePixelRatio);
}

qreal ImagePresentationViewportState::steppedManualZoomPercent(qreal stepCount) const
{
    return m_zoomState.steppedManualZoomPercent(stepCount, renderContext().devicePixelRatio);
}

ImagePresentationViewportPlan ImagePresentationViewportState::setViewportSize(
    const QSizeF &viewportSize)
{
    return mutateZoomState([&viewportSize](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setViewportSize(viewportSize, devicePixelRatio);
    });
}

ImagePresentationViewportPlan ImagePresentationViewportState::setVisibleItemRect(
    const QRectF &visibleItemRect)
{
    if (m_visibleItemRect == visibleItemRect) {
        return {};
    }

    m_visibleItemRect = visibleItemRect;
    return ImagePresentationViewportPlan { { ImageDocumentChange::VisibleItemRect }, true };
}

ImagePresentationViewportPlan ImagePresentationViewportState::setZoomPercent(qreal zoomPercent)
{
    return mutateZoomState([zoomPercent](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setManualZoomPercent(zoomPercent, devicePixelRatio);
    });
}

ImagePresentationViewportPlan ImagePresentationViewportState::resetZoom()
{
    return mutateZoomState([](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.resetZoom(devicePixelRatio);
    });
}

ImagePresentationViewportPlan ImagePresentationViewportState::setFitMode(ImageZoomMode zoomMode)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return {};
    }

    return mutateZoomState([zoomMode](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setFitMode(zoomMode, devicePixelRatio);
    });
}

ImagePresentationViewportPlan ImagePresentationViewportState::resetRotation()
{
    if (!m_geometry.resetRotation()) {
        return {};
    }

    return applyGeometryRotationChange();
}

ImagePresentationViewportPlan ImagePresentationViewportState::resetRotationForNewImage()
{
    if (!m_geometry.resetRotation()) {
        return {};
    }

    return ImagePresentationViewportPlan { { ImageDocumentChange::Rotation }, false };
}

ImagePresentationViewportPlan ImagePresentationViewportState::rotateClockwise()
{
    if (!m_geometry.rotateClockwise()) {
        return {};
    }

    return applyGeometryRotationChange();
}

ImagePresentationViewportPlan ImagePresentationViewportState::rotateCounterclockwise()
{
    if (!m_geometry.rotateCounterclockwise()) {
        return {};
    }

    return applyGeometryRotationChange();
}

ImagePresentationViewportPlan ImagePresentationViewportState::updateRenderContext()
{
    return mutateZoomState([](ImageZoomState &zoomState,
                               qreal devicePixelRatio) { zoomState.update(devicePixelRatio); },
        TileRefresh::Always);
}

ImagePresentationViewportPlan ImagePresentationViewportState::prepareImageContainer(
    const QUrl &containerUrl)
{
    return mutateZoomState([&containerUrl](ImageZoomState &zoomState, qreal) {
        zoomState.prepareImageContainer(containerUrl);
    });
}

ImagePresentationViewportPlan ImagePresentationViewportState::prepareFailedContainer(
    const QUrl &containerUrl)
{
    return mutateZoomState([&containerUrl](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.clearContainer();
        zoomState.prepareImageContainer(containerUrl);
        zoomState.resetZoom(devicePixelRatio);
    });
}

void ImagePresentationViewportState::clearContainer() { m_zoomState.clearContainer(); }

ImagePresentationViewportPlan ImagePresentationViewportState::setDisplayedImageSize(
    const QSize &imageSize)
{
    m_geometry.setSourceImageSize(imageSize);
    return applyGeometryImageSize();
}

ImagePresentationViewportPlan ImagePresentationViewportState::applyGeometryRotationChange()
{
    ImagePresentationViewportPlan plan = applyGeometryImageSize(TileRefresh::Always);
    plan.changes.push_back(ImageDocumentChange::Rotation);
    plan.changes.push_back(ImageDocumentChange::Repaint);
    return plan;
}

ImagePresentationViewportPlan ImagePresentationViewportState::applyGeometryImageSize(
    TileRefresh tileRefresh)
{
    const QSize logicalImageSize = m_geometry.logicalImageSize();
    return mutateZoomState(
        [&logicalImageSize](ImageZoomState &zoomState, qreal devicePixelRatio) {
            zoomState.setImageSize(logicalImageSize, devicePixelRatio);
        },
        tileRefresh);
}

ImagePresentationViewportPlan ImagePresentationViewportState::mutateZoomState(
    const ZoomStateMutation &mutation, TileRefresh tileRefresh)
{
    const ImageRenderContextChange renderContextChange = m_renderContextState.refresh();
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    mutation(m_zoomState, renderContextChange.current.devicePixelRatio);

    const ImageZoomSnapshot current = m_zoomState.snapshot();
    const ImageZoomChangeSet changes = ImageZoomState::changeSet(previous,
        renderContextChange.previous.devicePixelRatio, current,
        renderContextChange.current.devicePixelRatio, tileRefresh == TileRefresh::Always);
    return ImagePresentationViewportPlan {
        imageDocumentPresentationZoomNotifications(changes),
        changes.scheduleVisibleTileDecode,
    };
}
}
