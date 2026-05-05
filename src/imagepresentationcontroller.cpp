// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationcontroller.h"

#include "displayedimagestate.h"
#include "imagerendering.h"
#include "imagetilevisibility.h"

#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <Qt>
#include <cmath>
#include <memory>
#include <utility>

namespace {
using KiriView::imageZoomApproximatelyEqual;
}

namespace KiriView {
struct ImagePresentationController::TileDecodeLifetime {
};

ImagePresentationController::ImagePresentationController(QObject *context,
    RenderContextProvider renderContextProvider, ImagePresentationController::Callbacks callbacks)
    : m_renderContextProvider(std::move(renderContextProvider))
    , m_callbacks(std::move(callbacks))
    , m_tileDecodeLifetime(std::make_shared<TileDecodeLifetime>())
    , m_context(context)
{
    m_displayedImageState = std::make_unique<DisplayedImageState>(
        context,
        [this](const QSize &imageSize) {
            setImageSize(imageSize);
            notify(ImageDocumentChange::Repaint);
        },
        [this](const QString &errorString) {
            if (m_callbacks.animationError) {
                m_callbacks.animationError(errorString);
            }
        });
}

ImagePresentationController::~ImagePresentationController() { m_tileDecodeLifetime.reset(); }

QSize ImagePresentationController::imageSize() const { return m_zoomState.imageSize(); }

QSizeF ImagePresentationController::viewportSize() const { return m_zoomState.viewportSize(); }

void ImagePresentationController::setViewportSize(const QSizeF &viewportSize)
{
    const qreal devicePixelRatio = displayDevicePixelRatio();
    mutateZoomState([&viewportSize, devicePixelRatio](ImageZoomState &zoomState) {
        return zoomState.setViewportSize(viewportSize, devicePixelRatio);
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
    scheduleVisibleTileDecode();
}

qreal ImagePresentationController::zoomPercent() const { return m_zoomState.zoomPercent(); }

void ImagePresentationController::setZoomPercent(qreal zoomPercent)
{
    const qreal devicePixelRatio = displayDevicePixelRatio();
    mutateZoomState([zoomPercent, devicePixelRatio](ImageZoomState &zoomState) {
        return zoomState.setManualZoomPercent(zoomPercent, devicePixelRatio);
    });
}

ImageZoomMode ImagePresentationController::zoomMode() const { return m_zoomState.zoomMode(); }

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

std::shared_ptr<ImageTileSource> ImagePresentationController::staticImageSource() const
{
    return m_displayedImageState->staticImageSource();
}

const QImage &ImagePresentationController::staticImagePreview() const
{
    return m_displayedImageState->staticImagePreview();
}

const StaticImageDisplayHints &ImagePresentationController::staticImageDisplayHints() const
{
    return m_displayedImageState->staticImageDisplayHints();
}

ImageFirstDisplayDecodeContext ImagePresentationController::firstDisplayDecodeContext() const
{
    const QSizeF viewport = viewportSize();
    const qreal devicePixelRatio = displayDevicePixelRatio();
    if (viewport.isEmpty() || !std::isfinite(devicePixelRatio) || devicePixelRatio <= 0.0) {
        return {};
    }

    const int width = static_cast<int>(std::ceil(viewport.width() * devicePixelRatio));
    const int height = static_cast<int>(std::ceil(viewport.height() * devicePixelRatio));
    if (width <= 0 || height <= 0) {
        return {};
    }

    return ImageFirstDisplayDecodeContext { QSize(width, height) };
}

void ImagePresentationController::resetZoom()
{
    const qreal devicePixelRatio = displayDevicePixelRatio();
    mutateZoomState([devicePixelRatio](ImageZoomState &zoomState) {
        zoomState.resetZoom(devicePixelRatio);
        return true;
    });
}

void ImagePresentationController::setFitMode(ImageZoomMode zoomMode)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return;
    }

    const qreal devicePixelRatio = displayDevicePixelRatio();
    mutateZoomState([zoomMode, devicePixelRatio](ImageZoomState &zoomState) {
        return zoomState.setFitMode(zoomMode, devicePixelRatio);
    });
}

void ImagePresentationController::updateRenderContext()
{
    const qreal devicePixelRatio = displayDevicePixelRatio();
    mutateZoomState([devicePixelRatio](ImageZoomState &zoomState) {
        zoomState.update(devicePixelRatio);
        return true;
    });
}

void ImagePresentationController::prepareImageContainer(const QUrl &containerUrl)
{
    mutateZoomState([&containerUrl](ImageZoomState &zoomState) {
        zoomState.prepareImageContainer(containerUrl);
        return true;
    });
}

void ImagePresentationController::prepareFailedContainer(const QUrl &containerUrl)
{
    const qreal devicePixelRatio = displayDevicePixelRatio();
    mutateZoomState([&containerUrl, devicePixelRatio](ImageZoomState &zoomState) {
        zoomState.clearContainer();
        zoomState.prepareImageContainer(containerUrl);
        zoomState.resetZoom(devicePixelRatio);
        return true;
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

void ImagePresentationController::setStaticImage(std::shared_ptr<ImageTileSource> source,
    const QImage &preview, StaticImageDisplayHints displayHints)
{
    stopAnimation();
    invalidateTiles();
    const bool useFullImageSurface = source != nullptr
        && staticImageFitsFullImageSurface(*source, preview, maximumTextureSize());
    m_displayedImageState->setStaticImage(
        std::move(source), preview, displayHints, useFullImageSurface);
    if (!useFullImageSurface) {
        scheduleVisibleTileDecode();
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

void ImagePresentationController::startHeifSequenceAnimation(
    const QByteArray &data, int firstFrameDelay)
{
    m_displayedImageState->startHeifSequenceAnimation(data, firstFrameDelay);
}

void ImagePresentationController::stopAnimation() { m_displayedImageState->stopAnimation(); }

void ImagePresentationController::setImageSize(const QSize &imageSize)
{
    const qreal devicePixelRatio = displayDevicePixelRatio();
    mutateZoomState([&imageSize, devicePixelRatio](ImageZoomState &zoomState) {
        return zoomState.setImageSize(imageSize, devicePixelRatio);
    });
}

void ImagePresentationController::invalidateTiles()
{
    m_tileGeneration.invalidate();
    m_pendingTileKeys.clear();
    m_failedTileKeys.clear();
}

void ImagePresentationController::scheduleVisibleTileDecode()
{
    const std::shared_ptr<DisplayedImageSurface> displayedSurface
        = m_displayedImageState->imageSurface();
    if (displayedSurface == nullptr) {
        return;
    }

    auto *surface = std::get_if<StaticTileSurface>(displayedSurface.get());
    if (surface == nullptr || !surface->isValid()) {
        return;
    }
    if (staticTileSurfaceFirstDisplayIsSufficient(*surface)) {
        return;
    }

    std::shared_ptr<ImageTileSource> source = surface->source();
    if (source == nullptr) {
        return;
    }

    const quint64 generation = m_tileGeneration.current();
    const TileVisibilityContext visibilityContext {
        m_zoomState.displaySize(),
        m_visibleItemRect,
        displayDevicePixelRatio(),
    };
    for (const TileKey &key : visibleTileKeys(surface->pyramid(), visibilityContext)) {
        if (surface->containsTile(key) || m_pendingTileKeys.contains(key)
            || m_failedTileKeys.contains(key)) {
            continue;
        }

        const TileRequest request = surface->pyramid().requestForTile(key);
        if (request.textureLevelRect.isEmpty() || request.sourceRect.isEmpty()) {
            continue;
        }

        m_pendingTileKeys.insert(key);
        const QPointer<QObject> context(m_context);
        // Tile workers can outlive this non-QObject controller while the Qt context survives.
        const std::weak_ptr<TileDecodeLifetime> lifetime = m_tileDecodeLifetime;
        QThreadPool::globalInstance()->start(QRunnable::create(
            [this, context, lifetime, source, request, generation, key]() mutable {
                QString errorString;
                std::optional<DecodedTile> tile = source->decodeTile(request, &errorString);
                if (context == nullptr || lifetime.expired()) {
                    return;
                }

                QMetaObject::invokeMethod(
                    context.data(),
                    [this, context, lifetime, generation, key, tile = std::move(tile)]() mutable {
                        if (context == nullptr || lifetime.expired()) {
                            return;
                        }
                        finishTileDecode(generation, key, std::move(tile));
                    },
                    Qt::QueuedConnection);
            }));
    }
}

bool ImagePresentationController::staticTileSurfaceFirstDisplayIsSufficient(
    const StaticTileSurface &surface) const
{
    const qreal firstDisplayPixelsPerSourcePixel
        = surface.displayHints().firstDisplayPixelsPerSourcePixel;
    if (!std::isfinite(firstDisplayPixelsPerSourcePixel)
        || firstDisplayPixelsPerSourcePixel <= 0.0) {
        return false;
    }

    const qreal currentDisplayPixelsPerSourcePixel = tileDisplayPixelsPerSourcePixel(
        surface.pyramid(), m_zoomState.displaySize(), displayDevicePixelRatio());
    if (!std::isfinite(currentDisplayPixelsPerSourcePixel)
        || currentDisplayPixelsPerSourcePixel <= 0.0) {
        return false;
    }

    return currentDisplayPixelsPerSourcePixel <= firstDisplayPixelsPerSourcePixel + 0.001;
}

bool ImagePresentationController::tileRequestIsCurrent(quint64 generation, const TileKey &key) const
{
    return m_tileGeneration.accepts(generation) && m_pendingTileKeys.contains(key);
}

void ImagePresentationController::finishTileDecode(
    quint64 generation, TileKey key, std::optional<DecodedTile> tile)
{
    if (!tileRequestIsCurrent(generation, key)) {
        return;
    }

    m_pendingTileKeys.remove(key);
    if (!tile.has_value()) {
        m_failedTileKeys.insert(key);
        return;
    }

    if (m_displayedImageState->insertTile(std::move(*tile))) {
        notify(ImageDocumentChange::Repaint);
    }
}

bool ImagePresentationController::mutateZoomState(const ZoomStateMutation &mutation)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!mutation(m_zoomState)) {
        return false;
    }

    applyZoomStateChanges(previous);
    return true;
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
        notify(ImageDocumentChange::Repaint);
    }
    scheduleVisibleTileDecode();
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
    if (m_callbacks.change) {
        m_callbacks.change(change);
    }
}
}
