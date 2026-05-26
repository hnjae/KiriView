// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationviewportstate.h"

#include "document/imagedocumentnotifications.h"
#include "rendering/imagerendering.h"
#include "rendering/imagerotation.h"

#include <utility>
#include <vector>

namespace KiriView {
ImagePresentationViewportState::ImagePresentationViewportState(
    RenderContextProvider renderContextProvider)
    : m_zoomWorkflowState(std::move(renderContextProvider))
{
}

QSize ImagePresentationViewportState::imageSize() const { return logicalImageSize(); }

QSizeF ImagePresentationViewportState::viewportSize() const
{
    return m_zoomWorkflowState.zoomState().viewportSize();
}

QSizeF ImagePresentationViewportState::displaySize() const
{
    return m_zoomWorkflowState.zoomState().displaySize();
}

const QRectF &ImagePresentationViewportState::visibleItemRect() const { return m_visibleItemRect; }

qreal ImagePresentationViewportState::zoomPercent() const
{
    return m_zoomWorkflowState.zoomState().zoomPercent();
}

ImageZoomMode ImagePresentationViewportState::zoomMode() const
{
    return m_zoomWorkflowState.zoomState().zoomMode();
}

int ImagePresentationViewportState::rotationDegrees() const { return m_rotationDegrees; }

ImageDocumentRenderContext ImagePresentationViewportState::renderContext() const
{
    return m_zoomWorkflowState.renderContext();
}

ImageFirstDisplayDecodeContext ImagePresentationViewportState::firstDisplayDecodeContext() const
{
    const ImageDocumentRenderContext context = renderContext();
    return imageFirstDisplayDecodeContext(viewportSize(), context.devicePixelRatio);
}

qreal ImagePresentationViewportState::maximumManualZoomPercent() const
{
    return m_zoomWorkflowState.zoomState().maximumManualZoomPercent(
        renderContext().devicePixelRatio);
}

qreal ImagePresentationViewportState::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_zoomWorkflowState.zoomState().clampedManualZoomPercent(
        zoomPercent, renderContext().devicePixelRatio);
}

qreal ImagePresentationViewportState::steppedManualZoomPercent(qreal stepCount) const
{
    return m_zoomWorkflowState.zoomState().steppedManualZoomPercent(
        stepCount, renderContext().devicePixelRatio);
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
    return ImagePresentationViewportPlan {
        { ImageDocumentChange::VisibleItemRect, ImageDocumentChange::RenderFrame },
        true,
    };
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
    if (!resetPresentationRotation()) {
        return {};
    }

    return applyGeometryRotationChange();
}

ImagePresentationViewportPlan ImagePresentationViewportState::resetRotationForNewImage()
{
    if (!resetPresentationRotation()) {
        return {};
    }

    return ImagePresentationViewportPlan { { ImageDocumentChange::Rotation }, false };
}

ImagePresentationViewportPlan ImagePresentationViewportState::rotateClockwise()
{
    if (!rotatePresentationClockwise()) {
        return {};
    }

    return applyGeometryRotationChange();
}

ImagePresentationViewportPlan ImagePresentationViewportState::rotateCounterclockwise()
{
    if (!rotatePresentationCounterclockwise()) {
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

void ImagePresentationViewportState::clearContainer() { m_zoomWorkflowState.clearContainer(); }

ImagePresentationViewportPlan ImagePresentationViewportState::setDisplayedImageSize(
    const QSize &imageSize)
{
    setSourceImageSize(imageSize);
    return applyGeometryImageSize();
}

QSize ImagePresentationViewportState::logicalImageSize() const
{
    return rotatedImageSize(m_sourceImageSize, m_rotationDegrees);
}

bool ImagePresentationViewportState::setSourceImageSize(const QSize &sourceImageSize)
{
    if (m_sourceImageSize == sourceImageSize) {
        return false;
    }

    m_sourceImageSize = sourceImageSize;
    return true;
}

bool ImagePresentationViewportState::setRotationDegrees(int rotationDegrees)
{
    const int normalizedRotationDegrees = normalizedImageRotationDegrees(rotationDegrees);
    if (m_rotationDegrees == normalizedRotationDegrees) {
        return false;
    }

    m_rotationDegrees = normalizedRotationDegrees;
    return true;
}

bool ImagePresentationViewportState::resetPresentationRotation() { return setRotationDegrees(0); }

bool ImagePresentationViewportState::rotatePresentationClockwise()
{
    return setRotationDegrees(imageRotationClockwise(m_rotationDegrees));
}

bool ImagePresentationViewportState::rotatePresentationCounterclockwise()
{
    return setRotationDegrees(imageRotationCounterclockwise(m_rotationDegrees));
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
    const QSize nextLogicalImageSize = logicalImageSize();
    return mutateZoomState(
        [&nextLogicalImageSize](ImageZoomState &zoomState, qreal devicePixelRatio) {
            zoomState.setImageSize(nextLogicalImageSize, devicePixelRatio);
        },
        tileRefresh);
}

ImagePresentationViewportPlan ImagePresentationViewportState::mutateZoomState(
    const ZoomStateMutation &mutation, TileRefresh tileRefresh)
{
    const ImageZoomWorkflowMutationResult result
        = m_zoomWorkflowState.mutate(mutation, tileRefresh == TileRefresh::Always);
    std::vector<ImageDocumentChange> changes
        = imageDocumentPresentationZoomNotifications(result.changes);
    if (result.changes.scheduleVisibleTileDecode) {
        changes.push_back(ImageDocumentChange::RenderFrame);
    }
    return ImagePresentationViewportPlan {
        std::move(changes),
        result.changes.scheduleVisibleTileDecode,
    };
}
}
