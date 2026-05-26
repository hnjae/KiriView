// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONVIEWPORTCONTROLLER_H
#define KIRIVIEW_IMAGEPRESENTATIONVIEWPORTCONTROLLER_H

#include "document/imagedocumenttypes.h"
#include "presentation/imagepresentationviewportstate.h"
#include "rendering/imagerendercontext.h"
#include "rendering/imagesurface.h"

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
    void applyPlan(const ImagePresentationViewportPlan &plan);
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    void notify(ImageDocumentChange change);

    ImagePresentationViewportState m_state;
    ImageSurfaceProvider m_imageSurfaceProvider;
    ChangeCallback m_changeCallback;
    std::unique_ptr<ImageTileDecodeScheduler> m_tileDecodeScheduler;
};
}

#endif
