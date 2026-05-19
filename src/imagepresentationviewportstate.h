// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONVIEWPORTSTATE_H
#define KIRIVIEW_IMAGEPRESENTATIONVIEWPORTSTATE_H

#include "imagedocumenttypes.h"
#include "imagepresentationgeometry.h"
#include "imagerendercontextstate.h"
#include "imagezoomstate.h"
#include "staticimage.h"

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
    using RenderContextProvider = ImageRenderContextState::Provider;

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

    using ZoomStateMutation = std::function<void(ImageZoomState &, qreal devicePixelRatio)>;

    ImagePresentationViewportPlan applyGeometryRotationChange();
    ImagePresentationViewportPlan applyGeometryImageSize(
        TileRefresh tileRefresh = TileRefresh::WhenZoomStateChanges);
    ImagePresentationViewportPlan mutateZoomState(const ZoomStateMutation &mutation,
        TileRefresh tileRefresh = TileRefresh::WhenZoomStateChanges);

    ImageRenderContextState m_renderContextState;
    ImagePresentationGeometry m_geometry;
    ImageZoomState m_zoomState;
    QRectF m_visibleItemRect;
};
}

#endif
