// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONACTIVESTATE_H
#define KIRIVIEW_IMAGEPRESENTATIONACTIVESTATE_H

#include "presentation/imagepresentationstate.h"
#include "presentation/imagespreadgeometry.h"
#include "presentation/imageviewportcommandstate.h"
#include "presentation/imagezoomworkflowstate.h"

#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <functional>

namespace KiriView {
class ImagePresentationController;

enum class ImagePresentationActiveMode {
    SinglePage,
    TwoPageSpread,
};

struct ImagePresentationPageProjection {
    QSizeF displaySize;
    QRectF visibleItemRect;
    int rotationDegrees = 0;
};

class ImagePresentationActiveState final
{
public:
    using RenderContextProvider = ImageZoomWorkflowState::RenderContextProvider;
    using ZoomStateMutation = ImageZoomWorkflowState::ZoomStateMutation;

    ImagePresentationActiveState(RenderContextProvider renderContextProvider,
        ImagePresentationController &primaryPresentation,
        ImagePresentationController &secondaryPresentation);

    ImagePresentationActiveMode mode() const;
    void setMode(ImagePresentationActiveMode mode);

    bool transitionInProgress() const;
    ImagePresentationTransitionState transitionState() const;
    bool beginTransition();
    bool showTransitionPlaceholder();
    bool finishTransition();
    bool abortTransition();

    QSize imageSize() const;
    QSize spreadImageSize() const;
    QSizeF displaySize() const;
    QSizeF primaryDisplaySize() const;
    QSizeF secondaryDisplaySize() const;
    qreal zoomPercent() const;
    ImageZoomMode zoomMode() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;
    int rotationDegrees() const;

    const ImageViewportFrame &viewportFrame() const;
    const ImageViewportProjection &viewportProjection() const;
    ImageViewportCommand requestViewportContentPosition(const QPointF &contentPosition);
    bool acknowledgeViewportCommand(quint64 commandRevision, const QPointF &actualContentPosition);
    bool observeViewportContentPosition(
        const QPointF &contentPosition, ImageViewportObservationOrigin origin);
    bool setViewportGeometry(const QSizeF &viewportSize, const QSizeF &displaySize,
        ImageViewportObservationOrigin origin = ImageViewportObservationOrigin::Resize);

    ImageZoomChangeSet setZoomPercent(qreal zoomPercent);
    ImageZoomChangeSet setFitMode(ImageZoomMode zoomMode);
    ImageZoomChangeSet resetZoom();
    ImageZoomChangeSet updateFromPageState(bool resetZoomOnPrimaryImageChange = false);
    ImageZoomChangeSet updateRenderContext();
    bool resetRotation();
    bool rotateClockwise();
    bool rotateCounterclockwise();

    ImagePresentationPageProjection primaryProjection(bool rightToLeftReading) const;
    ImagePresentationPageProjection secondaryProjection(bool rightToLeftReading) const;
    void applyVisibleItemRects(bool rightToLeftReading);

private:
    QSize sourceImageSize() const;
    QSize logicalSinglePageImageSize() const;
    QRectF primaryPageRect(bool rightToLeftReading) const;
    QRectF secondaryPageRect(bool rightToLeftReading) const;
    ImageZoomChangeSet mutateZoomState(const ZoomStateMutation &mutation);
    qreal devicePixelRatio() const;

    ImageZoomWorkflowState m_zoomWorkflowState;
    ImageViewportCommandState m_viewportCommands;
    ImagePresentationController &m_primaryPresentation;
    ImagePresentationController &m_secondaryPresentation;
    quint64 m_primaryImageRevision = 0;
    ImagePresentationActiveMode m_mode = ImagePresentationActiveMode::SinglePage;
    ImagePresentationTransitionState m_transitionState
        = ImagePresentationTransitionState::CommittedActive;
    int m_rotationDegrees = 0;
};
}

#endif
