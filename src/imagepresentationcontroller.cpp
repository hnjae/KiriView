// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationcontroller.h"

#include "displayedimagestate.h"
#include "imagecallback.h"
#include "imagedocumentnotifications.h"
#include "imagerendering.h"
#include "imagetiledecodescheduler.h"

#include <memory>
#include <utility>

namespace KiriView {
ImagePresentationController::ImagePresentationController(QObject *context,
    RenderContextProvider renderContextProvider, ImagePresentationController::Callbacks callbacks)
    : m_renderContextProvider(std::move(renderContextProvider))
    , m_callbacks(std::move(callbacks))
{
    m_renderContext = renderContext();
    m_displayedImageState = std::make_unique<DisplayedImageState>(
        context,
        [this](const QSize &imageSize) {
            setImageSize(imageSize);
            notify(ImageDocumentChange::Repaint);
        },
        [this](
            const QString &errorString) { invokeIfSet(m_callbacks.animationError, errorString); });
    m_tileDecodeScheduler
        = std::make_unique<ImageTileDecodeScheduler>(context, [this](DecodedTile tile) {
              if (m_displayedImageState->insertTile(std::move(tile))) {
                  notify(ImageDocumentChange::Repaint);
              }
          });
}

ImagePresentationController::~ImagePresentationController() = default;

QSize ImagePresentationController::imageSize() const { return m_zoomState.imageSize(); }

QSizeF ImagePresentationController::viewportSize() const { return m_zoomState.viewportSize(); }

void ImagePresentationController::setViewportSize(const QSizeF &viewportSize)
{
    mutateZoomState([&viewportSize](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setViewportSize(viewportSize, devicePixelRatio);
    });
}

QSizeF ImagePresentationController::displaySize() const { return m_zoomState.displaySize(); }

QRectF ImagePresentationController::visibleItemRect() const { return m_visibleItemRect; }

void ImagePresentationController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    if (m_visibleItemRect == visibleItemRect) {
        return;
    }

    m_visibleItemRect = visibleItemRect;
    notify(ImageDocumentChange::VisibleItemRect);
    scheduleVisibleTileDecode(renderContext());
}

qreal ImagePresentationController::zoomPercent() const { return m_zoomState.zoomPercent(); }

void ImagePresentationController::setZoomPercent(qreal zoomPercent)
{
    mutateZoomState([zoomPercent](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setManualZoomPercent(zoomPercent, devicePixelRatio);
    });
}

ImageZoomMode ImagePresentationController::zoomMode() const { return m_zoomState.zoomMode(); }

qreal ImagePresentationController::maximumManualZoomPercent() const
{
    return m_zoomState.maximumManualZoomPercent(renderContext().devicePixelRatio);
}

qreal ImagePresentationController::clampedManualZoomPercent(qreal zoomPercent) const
{
    return m_zoomState.clampedManualZoomPercent(zoomPercent, renderContext().devicePixelRatio);
}

qreal ImagePresentationController::steppedManualZoomPercent(qreal stepCount) const
{
    return m_zoomState.steppedManualZoomPercent(stepCount, renderContext().devicePixelRatio);
}

std::shared_ptr<DisplayedImageSurface> ImagePresentationController::imageSurface() const
{
    return m_displayedImageState->imageSurface();
}

const QImage &ImagePresentationController::image() const { return m_displayedImageState->image(); }

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
    const ImageDocumentRenderContext context = renderContext();
    return imageFirstDisplayDecodeContext(viewportSize(), context.devicePixelRatio);
}

void ImagePresentationController::resetZoom()
{
    mutateZoomState([](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.resetZoom(devicePixelRatio);
    });
}

void ImagePresentationController::setFitMode(ImageZoomMode zoomMode)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return;
    }

    mutateZoomState([zoomMode](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setFitMode(zoomMode, devicePixelRatio);
    });
}

void ImagePresentationController::updateRenderContext()
{
    mutateZoomState([](ImageZoomState &zoomState,
                        qreal devicePixelRatio) { zoomState.update(devicePixelRatio); },
        TileRefresh::Always);
}

void ImagePresentationController::prepareImageContainer(const QUrl &containerUrl)
{
    mutateZoomState([&containerUrl](ImageZoomState &zoomState, qreal) {
        zoomState.prepareImageContainer(containerUrl);
    });
}

void ImagePresentationController::prepareFailedContainer(const QUrl &containerUrl)
{
    mutateZoomState([&containerUrl](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.clearContainer();
        zoomState.prepareImageContainer(containerUrl);
        zoomState.resetZoom(devicePixelRatio);
    });
}

void ImagePresentationController::setPredecodeCacheable(bool cacheable)
{
    m_displayedImageState->setPredecodeCacheable(cacheable);
}

void ImagePresentationController::setImage(const QImage &image)
{
    invalidateTiles();
    m_displayedImageState->setImage(image);
}

void ImagePresentationController::setStaticImage(StaticImagePayload staticImage)
{
    const ImageDocumentRenderContext context = renderContext();
    stopAnimation();
    invalidateTiles();
    const bool useFullImageSurface
        = staticImageFitsFullImageSurface(staticImage, context.maximumTextureSize);
    m_displayedImageState->setStaticImage(std::move(staticImage), useFullImageSurface);
    if (!useFullImageSurface) {
        scheduleVisibleTileDecode(context);
    }
}

void ImagePresentationController::clearImage()
{
    invalidateTiles();
    m_zoomState.clearContainer();
    m_displayedImageState->clear();
    setImageSize(QSize());
}

void ImagePresentationController::startAnimation(
    const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay)
{
    m_displayedImageState->startAnimation(data, format, loopCount, firstFrameDelay);
}

void ImagePresentationController::startDecodedAnimation(
    std::vector<AnimationFrame> frames, int loopCount)
{
    m_displayedImageState->startDecodedAnimation(std::move(frames), loopCount);
}

void ImagePresentationController::startHeifSequenceAnimation(const QByteArray &data)
{
    m_displayedImageState->startHeifSequenceAnimation(data);
}

void ImagePresentationController::stopAnimation() { m_displayedImageState->stopAnimation(); }

void ImagePresentationController::setImageSize(const QSize &imageSize)
{
    mutateZoomState([&imageSize](ImageZoomState &zoomState, qreal devicePixelRatio) {
        zoomState.setImageSize(imageSize, devicePixelRatio);
    });
}

void ImagePresentationController::invalidateTiles() { m_tileDecodeScheduler->invalidate(); }

void ImagePresentationController::scheduleVisibleTileDecode(
    const ImageDocumentRenderContext &context)
{
    m_tileDecodeScheduler->schedule(m_displayedImageState->imageSurface(),
        m_zoomState.displaySize(), m_visibleItemRect, context);
}

void ImagePresentationController::mutateZoomState(
    const ZoomStateMutation &mutation, TileRefresh tileRefresh)
{
    const ImageDocumentRenderContext previousContext = m_renderContext;
    const ImageDocumentRenderContext context = renderContext();
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    mutation(m_zoomState, context.devicePixelRatio);

    applyZoomStateChanges(previous, previousContext, context, tileRefresh);
    m_renderContext = context;
}

void ImagePresentationController::applyZoomStateChanges(const ImageZoomSnapshot &previous,
    const ImageDocumentRenderContext &previousContext, const ImageDocumentRenderContext &context,
    TileRefresh tileRefresh)
{
    const ImageZoomSnapshot current = m_zoomState.snapshot();
    const ImageZoomChangeSet changes
        = ImageZoomState::changeSet(previous, previousContext.devicePixelRatio, current,
            context.devicePixelRatio, tileRefresh == TileRefresh::Always);
    for (ImageDocumentChange change : imageDocumentPresentationZoomNotifications(changes)) {
        notify(change);
    }

    if (changes.scheduleVisibleTileDecode) {
        scheduleVisibleTileDecode(context);
    }
}

ImageDocumentRenderContext ImagePresentationController::renderContext() const
{
    ImageDocumentRenderContext context
        = m_renderContextProvider ? m_renderContextProvider() : ImageDocumentRenderContext {};
    return normalizedImageDocumentRenderContext(context);
}

void ImagePresentationController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_callbacks.change, change);
}
}
