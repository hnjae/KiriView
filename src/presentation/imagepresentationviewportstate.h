// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONVIEWPORTSTATE_H
#define KIRIVIEW_IMAGEPRESENTATIONVIEWPORTSTATE_H

#include "document/imagedocumenttypes.h"
#include "presentation/imagezoomworkflowstate.h"
#include "rendering/imagerendercontext.h"
#include "rendering/staticimage.h"

#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QUrl>
#include <functional>
#include <vector>

namespace KiriView {
struct ImagePresentationViewportPlan {
    std::vector<ImageDocumentChange> changes;
    bool scheduleVisibleTileDecode = false;
};

class ImagePresentationViewportState final
{
public:
    using RenderContextProvider = ImageZoomWorkflowState::RenderContextProvider;

    explicit ImagePresentationViewportState(RenderContextProvider renderContextProvider = {});

    QSize imageSize() const;
    QSizeF viewportSize() const;
    QSizeF displaySize() const;
    const QRectF &visibleItemRect() const;
    qreal zoomPercent() const;
    ImageZoomMode zoomMode() const;
    int rotationDegrees() const;
    ImageDocumentRenderContext renderContext() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;

    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;

    ImagePresentationViewportPlan setViewportSize(const QSizeF &viewportSize);
    ImagePresentationViewportPlan setVisibleItemRect(const QRectF &visibleItemRect);
    ImagePresentationViewportPlan setZoomPercent(qreal zoomPercent);
    ImagePresentationViewportPlan resetZoom();
    ImagePresentationViewportPlan setFitMode(ImageZoomMode zoomMode);
    ImagePresentationViewportPlan resetRotation();
    ImagePresentationViewportPlan resetRotationForNewImage();
    ImagePresentationViewportPlan rotateClockwise();
    ImagePresentationViewportPlan rotateCounterclockwise();
    ImagePresentationViewportPlan updateRenderContext();
    ImagePresentationViewportPlan prepareImageContainer(const QUrl &containerUrl);
    ImagePresentationViewportPlan prepareFailedContainer(const QUrl &containerUrl);
    void clearContainer();
    ImagePresentationViewportPlan setDisplayedImageSize(const QSize &imageSize);

private:
    enum class TileRefresh {
        WhenZoomStateChanges,
        Always,
    };

    using ZoomStateMutation = ImageZoomWorkflowState::ZoomStateMutation;

    QSize logicalImageSize() const;
    bool setSourceImageSize(const QSize &sourceImageSize);
    bool setRotationDegrees(int rotationDegrees);
    bool resetPresentationRotation();
    bool rotatePresentationClockwise();
    bool rotatePresentationCounterclockwise();
    ImagePresentationViewportPlan applyGeometryRotationChange();
    ImagePresentationViewportPlan applyGeometryImageSize(
        TileRefresh tileRefresh = TileRefresh::WhenZoomStateChanges);
    ImagePresentationViewportPlan mutateZoomState(const ZoomStateMutation &mutation,
        TileRefresh tileRefresh = TileRefresh::WhenZoomStateChanges);

    ImageZoomWorkflowState m_zoomWorkflowState;
    QSize m_sourceImageSize;
    int m_rotationDegrees = 0;
    QRectF m_visibleItemRect;
};
}

#endif
