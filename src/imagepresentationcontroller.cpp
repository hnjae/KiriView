// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationcontroller.h"

#include "displayedimagestate.h"
#include "imagecallback.h"
#include "imagepresentationviewportcontroller.h"
#include "imagerendering.h"

#include <memory>
#include <utility>

namespace KiriView {
ImagePresentationController::ImagePresentationController(QObject *context,
    RenderContextProvider renderContextProvider, ImagePresentationController::Callbacks callbacks)
    : m_callbacks(std::move(callbacks))
    , m_staticTileCacheByteBudget(StaticTileSurface::defaultTileCacheByteBudget())
{
    m_displayedImageState = std::make_unique<DisplayedImageState>(
        context,
        [this](const QSize &imageSize) {
            m_viewportController->setDisplayedImageSize(imageSize);
            notify(ImageDocumentChange::Repaint);
        },
        [this](
            const QString &errorString) { invokeIfSet(m_callbacks.animationError, errorString); });
    m_viewportController = std::make_unique<ImagePresentationViewportController>(
        context, std::move(renderContextProvider),
        [this]() { return m_displayedImageState->imageSurface(); },
        [this](DecodedTile tile) {
            if (m_displayedImageState->insertTile(std::move(tile))) {
                notify(ImageDocumentChange::Repaint);
            }
        },
        [this](ImageDocumentChange change) { notify(change); });
}

ImagePresentationController::~ImagePresentationController() = default;

QSize ImagePresentationController::imageSize() const { return m_viewportController->imageSize(); }

QSizeF ImagePresentationController::viewportSize() const
{
    return m_viewportController->viewportSize();
}

void ImagePresentationController::setViewportSize(const QSizeF &viewportSize)
{
    m_viewportController->setViewportSize(viewportSize);
}

QSizeF ImagePresentationController::displaySize() const
{
    return m_viewportController->displaySize();
}

QRectF ImagePresentationController::visibleItemRect() const
{
    return m_viewportController->visibleItemRect();
}

void ImagePresentationController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_viewportController->setVisibleItemRect(visibleItemRect);
}

qreal ImagePresentationController::zoomPercent() const
{
    return m_viewportController->zoomPercent();
}

void ImagePresentationController::setZoomPercent(qreal zoomPercent)
{
    m_viewportController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImagePresentationController::zoomMode() const
{
    return m_viewportController->zoomMode();
}

qreal ImagePresentationController::maximumManualZoomPercent() const
{
    return m_viewportController->maximumManualZoomPercent();
}

qreal ImagePresentationController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_viewportController->clampedManualZoomPercent(zoomPercent);
}

qreal ImagePresentationController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_viewportController->steppedManualZoomPercent(stepCount);
}

int ImagePresentationController::rotationDegrees() const
{
    return m_viewportController->rotationDegrees();
}

std::shared_ptr<DisplayedImageSurface> ImagePresentationController::imageSurface() const
{
    return m_displayedImageState->imageSurface();
}

DisplayedImageRenderSnapshot ImagePresentationController::renderSnapshot() const
{
    return DisplayedImageRenderSnapshot {
        imageSurface(),
        imageRevision(),
        imageSize(),
        rotationDegrees(),
    };
}

quint64 ImagePresentationController::imageRevision() const
{
    return m_displayedImageState->revision();
}

bool ImagePresentationController::hasImage() const { return m_displayedImageState->hasImage(); }

bool ImagePresentationController::isPredecodeCacheable() const
{
    return m_displayedImageState->isPredecodeCacheable();
}

std::optional<StaticImagePayload> ImagePresentationController::staticImage() const
{
    return m_displayedImageState->staticImage();
}

ImageFirstDisplayDecodeContext ImagePresentationController::firstDisplayDecodeContext() const
{
    return m_viewportController->firstDisplayDecodeContext();
}

void ImagePresentationController::resetZoom() { m_viewportController->resetZoom(); }

void ImagePresentationController::setFitMode(ImageZoomMode zoomMode)
{
    m_viewportController->setFitMode(zoomMode);
}

void ImagePresentationController::rotateClockwise()
{
    if (!hasImage()) {
        return;
    }

    m_viewportController->rotateClockwise();
}

void ImagePresentationController::rotateCounterclockwise()
{
    if (!hasImage()) {
        return;
    }

    m_viewportController->rotateCounterclockwise();
}

bool ImagePresentationController::resetRotation() { return m_viewportController->resetRotation(); }

void ImagePresentationController::updateRenderContext()
{
    m_viewportController->updateRenderContext();
}

void ImagePresentationController::prepareImageContainer(const QUrl &containerUrl)
{
    m_viewportController->prepareImageContainer(containerUrl);
}

void ImagePresentationController::prepareFailedContainer(const QUrl &containerUrl)
{
    m_viewportController->prepareFailedContainer(containerUrl);
}

void ImagePresentationController::setImage(const QImage &image, bool predecodeCacheable)
{
    resetRotationForNewImage();
    m_viewportController->invalidateTiles();
    m_displayedImageState->setImage(image, predecodeCacheable);
}

void ImagePresentationController::setStaticImage(
    StaticImagePayload staticImage, bool predecodeCacheable)
{
    const ImageDocumentRenderContext context = m_viewportController->renderContext();
    stopAnimation();
    resetRotationForNewImage();
    m_viewportController->invalidateTiles();
    const bool useFullImageSurface
        = staticImageFitsFullImageSurface(staticImage, context.maximumTextureSize);
    m_displayedImageState->setStaticImage(std::move(staticImage), useFullImageSurface,
        predecodeCacheable, m_staticTileCacheByteBudget);
    if (!useFullImageSurface) {
        m_viewportController->scheduleVisibleTileDecode();
    }
}

void ImagePresentationController::clearImage()
{
    resetRotationForNewImage();
    m_viewportController->invalidateTiles();
    m_viewportController->clearContainer();
    m_displayedImageState->clear();
    m_viewportController->setDisplayedImageSize(QSize());
}

void ImagePresentationController::startAnimation(const QByteArray &data, const QByteArray &format)
{
    m_displayedImageState->startAnimation(data, format);
}

void ImagePresentationController::startApngAnimation(const QByteArray &data)
{
    m_displayedImageState->startApngAnimation(data);
}

void ImagePresentationController::startHeifSequenceAnimation(const QByteArray &data)
{
    m_displayedImageState->startHeifSequenceAnimation(data);
}

void ImagePresentationController::stopAnimation() { m_displayedImageState->stopAnimation(); }

void ImagePresentationController::resetRotationForNewImage()
{
    m_viewportController->resetRotationForNewImage();
}

void ImagePresentationController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_callbacks.change, change);
}
}
