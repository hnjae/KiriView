// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPRESENTATIONCONTROLLER_H
#define KIRIVIEW_IMAGEPRESENTATIONCONTROLLER_H

#include "imagedocumenttypes.h"
#include "imagesurface.h"
#include "imagezoomstate.h"

#include <QByteArray>
#include <QImage>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <functional>
#include <memory>
#include <optional>

class QObject;

namespace KiriView {
class DisplayedImageState;
class ImagePresentationViewportController;

class ImagePresentationController final
{
public:
    using RenderContextProvider = std::function<ImageDocumentRenderContext()>;
    using ChangeCallback = std::function<void(ImageDocumentChange)>;
    using AnimationErrorCallback = std::function<void(const QString &)>;

    struct Callbacks {
        ChangeCallback change;
        AnimationErrorCallback animationError;
    };

    ImagePresentationController(
        QObject *context, RenderContextProvider renderContextProvider, Callbacks callbacks);
    ~ImagePresentationController();

    QSize imageSize() const;
    QSizeF viewportSize() const;
    void setViewportSize(const QSizeF &viewportSize);
    QSizeF displaySize() const;
    QRectF visibleItemRect() const;
    void setVisibleItemRect(const QRectF &visibleItemRect);
    qreal zoomPercent() const;
    void setZoomPercent(qreal zoomPercent);
    ImageZoomMode zoomMode() const;
    qreal maximumManualZoomPercent() const;
    qreal clampedManualZoomPercent(qreal zoomPercent) const;
    qreal steppedManualZoomPercent(qreal stepCount) const;
    int rotationDegrees() const;
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    DisplayedImageRenderSnapshot renderSnapshot() const;
    quint64 imageRevision() const;
    bool hasImage() const;
    bool isPredecodeCacheable() const;
    std::optional<StaticImagePayload> staticImage() const;
    ImageFirstDisplayDecodeContext firstDisplayDecodeContext() const;

    void resetZoom();
    void setFitMode(ImageZoomMode zoomMode);
    void rotateClockwise();
    void rotateCounterclockwise();
    bool resetRotation();
    void updateRenderContext();
    void prepareImageContainer(const QUrl &containerUrl);
    void prepareFailedContainer(const QUrl &containerUrl);
    void setImage(const QImage &image, bool predecodeCacheable);
    void setStaticImage(StaticImagePayload staticImage, bool predecodeCacheable);
    void clearImage();

    void startAnimation(const QByteArray &data, const QByteArray &format);
    void startApngAnimation(const QByteArray &data);
    void startHeifSequenceAnimation(const QByteArray &data);
    void stopAnimation();

private:
    void resetRotationForNewImage();
    void notify(ImageDocumentChange change);

    Callbacks m_callbacks;
    std::unique_ptr<DisplayedImageState> m_displayedImageState;
    std::unique_ptr<ImagePresentationViewportController> m_viewportController;
};
}

#endif
