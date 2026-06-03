// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepresentationactivestate.h"

#include "presentation/imagepresentationcontroller.h"
#include "presentation/imagespreadgeometry.h"
#include "rendering/imagerotation.h"

#include <utility>

namespace {
void applyPageProjection(KiriView::ImagePresentationController &presentation,
    const KiriView::ImagePresentationPageProjection &projection)
{
    presentation.setVisibleItemRect(projection.visibleItemRect);
    if (projection.visibleItemRect.isEmpty()) {
        presentation.discardDecodedTiles();
        return;
    }

    presentation.scheduleProjectedVisibleTileDecode(
        projection.displaySize, projection.visibleItemRect, projection.rotationDegrees);
}
}

namespace KiriView {
ImagePresentationActiveState::ImagePresentationActiveState(
    RenderContextProvider renderContextProvider, ImagePresentationController &primaryPresentation,
    ImagePresentationController &secondaryPresentation)
    : m_zoomWorkflowState(std::move(renderContextProvider))
    , m_primaryPresentation(primaryPresentation)
    , m_secondaryPresentation(secondaryPresentation)
{
}

ImagePresentationActiveMode ImagePresentationActiveState::mode() const { return m_mode; }

void ImagePresentationActiveState::setMode(ImagePresentationActiveMode mode)
{
    if (m_mode == mode) {
        return;
    }

    m_mode = mode;
    if (m_mode == ImagePresentationActiveMode::TwoPageSpread) {
        m_rotationDegrees = 0;
    }
}

bool ImagePresentationActiveState::transitionInProgress() const
{
    return m_transitionState != ImagePresentationTransitionState::CommittedActive;
}

ImagePresentationTransitionState ImagePresentationActiveState::transitionState() const
{
    return m_transitionState;
}

bool ImagePresentationActiveState::beginTransition()
{
    if (transitionInProgress()) {
        return false;
    }

    m_transitionState = ImagePresentationTransitionState::PreviousActive;
    return true;
}

bool ImagePresentationActiveState::showTransitionPlaceholder()
{
    if (!transitionInProgress()) {
        return false;
    }

    m_transitionState = ImagePresentationTransitionState::TransitioningPlaceholder;
    return true;
}

bool ImagePresentationActiveState::finishTransition()
{
    if (!transitionInProgress()) {
        return false;
    }

    m_transitionState = ImagePresentationTransitionState::CommittedActive;
    return true;
}

bool ImagePresentationActiveState::abortTransition()
{
    if (!transitionInProgress()) {
        return false;
    }

    m_transitionState = ImagePresentationTransitionState::CommittedActive;
    return true;
}

QSize ImagePresentationActiveState::imageSize() const { return sourceImageSize(); }

QSize ImagePresentationActiveState::spreadImageSize() const
{
    return imageSpreadImageSize(
        m_primaryPresentation.imageSize(), m_secondaryPresentation.imageSize());
}

QSizeF ImagePresentationActiveState::displaySize() const
{
    return m_zoomWorkflowState.zoomState().displaySize();
}

QSizeF ImagePresentationActiveState::primaryDisplaySize() const
{
    if (m_mode == ImagePresentationActiveMode::SinglePage) {
        return displaySize();
    }

    return imageSpreadScaledPageDisplaySize(
        m_primaryPresentation.imageSize(), spreadImageSize(), displaySize());
}

QSizeF ImagePresentationActiveState::secondaryDisplaySize() const
{
    if (m_mode == ImagePresentationActiveMode::SinglePage) {
        return {};
    }

    return imageSpreadScaledPageDisplaySize(
        m_secondaryPresentation.imageSize(), spreadImageSize(), displaySize());
}

qreal ImagePresentationActiveState::zoomPercent() const
{
    return m_zoomWorkflowState.zoomState().zoomPercent();
}

ImageZoomMode ImagePresentationActiveState::zoomMode() const
{
    return m_zoomWorkflowState.zoomState().zoomMode();
}

qreal ImagePresentationActiveState::maximumManualZoomPercent() const
{
    return m_zoomWorkflowState.zoomState().maximumManualZoomPercent(devicePixelRatio());
}

qreal ImagePresentationActiveState::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_zoomWorkflowState.zoomState().clampedManualZoomPercent(
        zoomPercent, devicePixelRatio());
}

qreal ImagePresentationActiveState::steppedManualZoomPercent(qreal stepCount) const
{
    return m_zoomWorkflowState.zoomState().steppedManualZoomPercent(stepCount, devicePixelRatio());
}

int ImagePresentationActiveState::rotationDegrees() const
{
    return m_mode == ImagePresentationActiveMode::TwoPageSpread ? 0 : m_rotationDegrees;
}

const ImageViewportFrame &ImagePresentationActiveState::viewportFrame() const
{
    return m_viewportCommands.projection().frame;
}

const ImageViewportProjection &ImagePresentationActiveState::viewportProjection() const
{
    return m_viewportCommands.projection();
}

ImageViewportCommand ImagePresentationActiveState::requestViewportContentPosition(
    const QPointF &contentPosition)
{
    return m_viewportCommands.requestContentPosition(contentPosition);
}

bool ImagePresentationActiveState::beginViewportCommandApplication(quint64 commandRevision)
{
    return m_viewportCommands.markCommandApplying(commandRevision);
}

bool ImagePresentationActiveState::completeViewportCommandApplication(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    return m_viewportCommands.completeCommandApplication(commandRevision, actualContentPosition);
}

bool ImagePresentationActiveState::acknowledgeViewportCommand(
    quint64 commandRevision, const QPointF &actualContentPosition)
{
    return m_viewportCommands.acknowledgeCommand(commandRevision, actualContentPosition);
}

bool ImagePresentationActiveState::observeViewportContentPosition(
    const QPointF &contentPosition, ImageViewportObservationOrigin origin)
{
    return m_viewportCommands.observeContentPosition(contentPosition, origin);
}

bool ImagePresentationActiveState::setViewportGeometry(
    const QSizeF &viewportSize, const QSizeF &displaySize, ImageViewportObservationOrigin origin)
{
    return m_viewportCommands.setGeometry(viewportSize, displaySize, origin);
}

ImageZoomChangeSet ImagePresentationActiveState::setZoomPercent(qreal zoomPercent)
{
    return mutateZoomState([zoomPercent](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setManualZoomPercent(zoomPercent, devicePixelRatio);
    });
}

ImageZoomChangeSet ImagePresentationActiveState::setFitMode(ImageZoomMode zoomMode)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return {};
    }

    return mutateZoomState([zoomMode](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setFitMode(zoomMode, devicePixelRatio);
    });
}

ImageZoomChangeSet ImagePresentationActiveState::resetZoom()
{
    return mutateZoomState([](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.resetZoom(devicePixelRatio);
    });
}

ImageZoomChangeSet ImagePresentationActiveState::updateFromPageState(
    bool resetZoomOnPrimaryImageChange)
{
    const quint64 primaryImageRevision = m_primaryPresentation.imageRevision();
    const bool primaryImageChanged = m_primaryImageRevision != primaryImageRevision;
    m_primaryImageRevision = primaryImageRevision;
    if (primaryImageChanged && m_mode == ImagePresentationActiveMode::SinglePage) {
        m_rotationDegrees = m_primaryPresentation.rotationDegrees();
    }

    const QSize nextImageSize = sourceImageSize();
    const QSizeF nextViewportSize = m_primaryPresentation.viewportSize();
    const ImageZoomState &zoomState = m_zoomWorkflowState.zoomState();
    const bool usePrimaryZoom
        = (primaryImageChanged && resetZoomOnPrimaryImageChange) || zoomState.imageSize().isEmpty();
    const ImageZoomMode activeZoomMode
        = usePrimaryZoom ? m_primaryPresentation.zoomMode() : zoomState.zoomMode();
    const qreal activeZoomPercent
        = usePrimaryZoom ? m_primaryPresentation.zoomPercent() : zoomState.zoomPercent();

    return mutateZoomState([nextImageSize, nextViewportSize, activeZoomMode, activeZoomPercent](
                               ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setViewportSize(nextViewportSize, devicePixelRatio);
        zoomState.setImageSize(nextImageSize, devicePixelRatio);
        if (activeZoomMode == ImageZoomMode::Manual) {
            zoomState.setManualZoomPercent(activeZoomPercent, devicePixelRatio);
            return;
        }

        zoomState.setFitMode(activeZoomMode, devicePixelRatio);
    });
}

ImageZoomChangeSet ImagePresentationActiveState::updateRenderContext()
{
    return mutateZoomState([](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.update(devicePixelRatio);
    });
}

bool ImagePresentationActiveState::resetRotation()
{
    if (m_rotationDegrees == 0) {
        return false;
    }

    m_rotationDegrees = 0;
    return true;
}

bool ImagePresentationActiveState::rotateClockwise()
{
    if (m_mode == ImagePresentationActiveMode::TwoPageSpread || !m_primaryPresentation.hasImage()) {
        return false;
    }

    m_rotationDegrees = imageRotationClockwise(m_rotationDegrees);
    return true;
}

bool ImagePresentationActiveState::rotateCounterclockwise()
{
    if (m_mode == ImagePresentationActiveMode::TwoPageSpread || !m_primaryPresentation.hasImage()) {
        return false;
    }

    m_rotationDegrees = imageRotationCounterclockwise(m_rotationDegrees);
    return true;
}

ImagePresentationPageProjection ImagePresentationActiveState::primaryProjection(
    bool rightToLeftReading) const
{
    if (m_mode == ImagePresentationActiveMode::SinglePage) {
        return ImagePresentationPageProjection {
            displaySize(),
            viewportFrame().visibleItemRect,
            rotationDegrees(),
        };
    }

    return ImagePresentationPageProjection {
        primaryDisplaySize(),
        imageSpreadVisiblePageRect(
            viewportFrame().visibleItemRect, primaryPageRect(rightToLeftReading)),
        0,
    };
}

ImagePresentationPageProjection ImagePresentationActiveState::secondaryProjection(
    bool rightToLeftReading) const
{
    if (m_mode == ImagePresentationActiveMode::SinglePage) {
        return {};
    }

    return ImagePresentationPageProjection {
        secondaryDisplaySize(),
        imageSpreadVisiblePageRect(
            viewportFrame().visibleItemRect, secondaryPageRect(rightToLeftReading)),
        0,
    };
}

void ImagePresentationActiveState::applyVisibleItemRects(bool rightToLeftReading)
{
    applyPageProjection(m_primaryPresentation, primaryProjection(rightToLeftReading));
    if (m_mode == ImagePresentationActiveMode::TwoPageSpread) {
        applyPageProjection(m_secondaryPresentation, secondaryProjection(rightToLeftReading));
    }
}

QSize ImagePresentationActiveState::sourceImageSize() const
{
    return m_mode == ImagePresentationActiveMode::TwoPageSpread ? spreadImageSize()
                                                                : logicalSinglePageImageSize();
}

QSize ImagePresentationActiveState::logicalSinglePageImageSize() const
{
    return rotatedImageSize(m_primaryPresentation.imageSize(), m_rotationDegrees);
}

QRectF ImagePresentationActiveState::primaryPageRect(bool rightToLeftReading) const
{
    return imageSpreadPrimaryPageRect(
        primaryDisplaySize(), secondaryDisplaySize(), displaySize(), rightToLeftReading);
}

QRectF ImagePresentationActiveState::secondaryPageRect(bool rightToLeftReading) const
{
    return imageSpreadSecondaryPageRect(
        primaryDisplaySize(), secondaryDisplaySize(), displaySize(), rightToLeftReading);
}

ImageZoomChangeSet ImagePresentationActiveState::mutateZoomState(const ZoomStateMutation &mutation)
{
    return m_zoomWorkflowState.mutate(mutation).changes;
}

qreal ImagePresentationActiveState::devicePixelRatio() const
{
    return m_zoomWorkflowState.devicePixelRatio();
}
}
