// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONVIEWPORTCONTROLLER_H
#define KIRIVIEW_IMAGEPRESENTATIONVIEWPORTCONTROLLER_H

#include "imagedocumenttypes.h"
#include "imagepresentationgeometry.h"
#include "imagesurface.h"
#include "imagezoomstate.h"

#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QUrl>
#include <QtGlobal>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
class ImageTileDecodeScheduler;

class ImagePresentationViewportController final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ImageSurfaceProvider = std::function<std::shared_ptr<DisplayedImageSurface>()>;
    using TileDecodedCallback = std::function<void(DecodedTile)>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;

    ImagePresentationViewportController(QObject *context,
        RenderContextProvider renderContextProvider, ImageSurfaceProvider imageSurfaceProvider,
        TileDecodedCallback tileDecoded, ChangeCallback changeCallback);
    ~ImagePresentationViewportController();

    QSize imageSize() const;
    QSizeF viewportSize() const;
    QSizeF displaySize() const;
    QRectF visibleItemRect() const;
    qreal zoomPercent() const;
    ImageZoomMode zoomMode() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;
    int rotationDegrees() const;
    ImageDocumentRenderContext renderContext() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;

    void setViewportSize(const QSizeF &viewportSize);
    void setVisibleItemRect(const QRectF &visibleItemRect);
    void setZoomPercent(qreal zoomPercent);
    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    bool resetRotation();
    void resetRotationForNewImage();
    void rotateClockwise();
    void rotateCounterclockwise();
    void updateRenderContext();
    void prepareImageContainer(const QUrl &containerUrl);
    void prepareFailedContainer(const QUrl &containerUrl);
    void clearContainer();
    void setDisplayedImageSize(const QSize &imageSize);
    void invalidateTiles();
    void scheduleVisibleTileDecode();

private:
    enum class TileRefresh {
        WhenZoomStateChanges,
        Always,
    };

    using ZoomStateMutation = std::function<void(ImageZoomState &, qreal devicePixelRatio)>;

    void applyGeometryRotationChange();
    void applyGeometryImageSize(TileRefresh tileRefresh = TileRefresh::WhenZoomStateChanges);
    void mutateZoomState(const ZoomStateMutation &mutation,
        TileRefresh tileRefresh = TileRefresh::WhenZoomStateChanges);
    void applyZoomStateChanges(const ImageZoomSnapshot &previous,
        const ImageDocumentRenderContext &previousContext,
        const ImageDocumentRenderContext &context, TileRefresh tileRefresh);
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    void notify(ImageDocumentChange change);

    RenderContextProvider m_renderContextProvider;
    ImageSurfaceProvider m_imageSurfaceProvider;
    ChangeCallback m_changeCallback;
    ImagePresentationGeometry m_geometry;
    ImageZoomState m_zoomState;
    ImageDocumentRenderContext m_renderContext;
    std::unique_ptr<ImageTileDecodeScheduler> m_tileDecodeScheduler;
    QRectF m_visibleItemRect;
};
}

#endif
