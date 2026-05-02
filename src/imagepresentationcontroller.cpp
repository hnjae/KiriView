// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationcontroller.h"

#include "displayedimagestate.h"
#include "kiriimagedecoder.h"

#include <QCoreApplication>
#include <cmath>
#include <utility>

namespace {
using KiriView::imageZoomApproximatelyEqual;
using KiriView::renderSvgImage;
using KiriView::svgRasterSize;

QString imageViewText(const char *sourceText)
{
    return QCoreApplication::translate("KiriImageView", sourceText);
}
}

namespace KiriView {
ImagePresentationController::ImagePresentationController(QObject *context,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    AnimationErrorCallback animationErrorCallback)
    : m_renderContextProvider(std::move(renderContextProvider))
    , m_changeCallback(std::move(changeCallback))
    , m_animationErrorCallback(std::move(animationErrorCallback))
{
    m_displayedImageState = std::make_unique<DisplayedImageState>(
        context,
        [this](const QSize &imageSize) {
            setImageSize(imageSize);
            notify(ImageDocumentChange::Repaint);
        },
        [this](const QString &errorString) {
            if (m_animationErrorCallback) {
                m_animationErrorCallback(errorString);
            }
        });
}

ImagePresentationController::~ImagePresentationController() = default;

QSize ImagePresentationController::imageSize() const { return m_zoomState.imageSize(); }

QSizeF ImagePresentationController::viewportSize() const { return m_zoomState.viewportSize(); }

void ImagePresentationController::setViewportSize(const QSizeF &viewportSize)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setViewportSize(viewportSize, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
}

QSizeF ImagePresentationController::displaySize() const { return m_zoomState.displaySize(); }

qreal ImagePresentationController::zoomPercent() const { return m_zoomState.zoomPercent(); }

void ImagePresentationController::setZoomPercent(qreal zoomPercent)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setManualZoomPercent(zoomPercent, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
}

ImageZoomMode ImagePresentationController::zoomMode() const { return m_zoomState.zoomMode(); }

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

void ImagePresentationController::resetZoom()
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.resetZoom(displayDevicePixelRatio());
    applyZoomStateChanges(previous);
}

void ImagePresentationController::setFitMode(ImageZoomMode zoomMode)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return;
    }

    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setFitMode(zoomMode, displayDevicePixelRatio())) {
        return;
    }
    applyZoomStateChanges(previous);
}

void ImagePresentationController::updateRenderContext()
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.update(displayDevicePixelRatio());
    applyZoomStateChanges(previous);
}

void ImagePresentationController::prepareImageContainer(const QUrl &containerUrl)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.prepareImageContainer(containerUrl);
    applyZoomStateChanges(previous);
}

void ImagePresentationController::prepareFailedContainer(const QUrl &containerUrl)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.clearContainer();
    m_zoomState.prepareImageContainer(containerUrl);
    m_zoomState.resetZoom(displayDevicePixelRatio());
    applyZoomStateChanges(previous);
}

void ImagePresentationController::setPredecodeCacheable(bool cacheable)
{
    m_displayedImageState->setPredecodeCacheable(cacheable);
}

void ImagePresentationController::setImage(const QImage &image)
{
    m_displayedImageState->setImage(image);
}

std::optional<QString> ImagePresentationController::setLoadedSvgImage(
    QByteArray data, const QSize &intrinsicSize, const QUrl &containerUrl)
{
    if (data.isEmpty() || intrinsicSize.isEmpty()) {
        return imageViewText("Could not decode the selected SVG image.");
    }

    const LoadedImageZoom loadedZoom
        = m_zoomState.loadedImageZoom(containerUrl, intrinsicSize, displayDevicePixelRatio());
    const QSize rasterSize
        = svgRasterSize(loadedZoom.displaySize, displayDevicePixelRatio(), maximumTextureSize());
    const QImage image = renderSvgImage(data, rasterSize);
    if (image.isNull()) {
        return imageViewText("Could not render the selected SVG image.");
    }

    stopAnimation();
    m_displayedImageState->setPredecodeCacheable(false);
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.setLoadedSvgZoom(containerUrl, loadedZoom);
    applyZoomStateChanges(previous);
    m_displayedImageState->setSvgImage(std::move(data), intrinsicSize, image, rasterSize);
    return std::nullopt;
}

void ImagePresentationController::clearImage()
{
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

void ImagePresentationController::stopAnimation() { m_displayedImageState->stopAnimation(); }

void ImagePresentationController::setImageSize(const QSize &imageSize)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setImageSize(imageSize, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
}

bool ImagePresentationController::updateDisplayedSvgRaster()
{
    return m_displayedImageState->updateSvgRaster(
        m_zoomState.displaySize(), displayDevicePixelRatio(), maximumTextureSize());
}

void ImagePresentationController::applyZoomStateChanges(const ImageZoomSnapshot &previous)
{
    const ImageZoomSnapshot current = m_zoomState.snapshot();
    if (previous.imageSize != current.imageSize) {
        notify(ImageDocumentChange::ImageSize);
    }
    if (!imageZoomApproximatelyEqual(previous.viewportSize, current.viewportSize)) {
        notify(ImageDocumentChange::ViewportSize);
    }
    if (previous.zoomMode != current.zoomMode) {
        notify(ImageDocumentChange::ZoomMode);
    }
    if (!imageZoomApproximatelyEqual(previous.zoomPercent, current.zoomPercent)) {
        notify(ImageDocumentChange::ZoomPercent);
    }

    if (!imageZoomApproximatelyEqual(previous.displaySize, current.displaySize)) {
        notify(ImageDocumentChange::DisplaySize);
        updateDisplayedSvgRaster();
        notify(ImageDocumentChange::Repaint);
    }
}

qreal ImagePresentationController::displayDevicePixelRatio() const
{
    return renderContext().devicePixelRatio;
}

int ImagePresentationController::maximumTextureSize() const
{
    return renderContext().maximumTextureSize;
}

ImageDocumentRenderContext ImagePresentationController::renderContext() const
{
    ImageDocumentRenderContext context
        = m_renderContextProvider ? m_renderContextProvider() : ImageDocumentRenderContext {};
    if (!std::isfinite(context.devicePixelRatio) || context.devicePixelRatio <= 0.0) {
        context.devicePixelRatio = 1.0;
    }
    if (context.maximumTextureSize <= 0) {
        context.maximumTextureSize = fallbackTextureSizeMax;
    }
    return context;
}

void ImagePresentationController::notify(ImageDocumentChange change)
{
    if (m_changeCallback) {
        m_changeCallback(change);
    }
}
}
