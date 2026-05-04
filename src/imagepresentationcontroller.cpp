// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepresentationcontroller.h"

#include "displayedimagestate.h"
#include "imagerendering.h"

#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <Qt>
#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace {
using KiriView::imageZoomApproximatelyEqual;
}

namespace KiriView {
ImagePresentationController::ImagePresentationController(QObject *context,
    RenderContextProvider renderContextProvider, ImagePresentationController::Callbacks callbacks)
    : m_renderContextProvider(std::move(renderContextProvider))
    , m_callbacks(std::move(callbacks))
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
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setManualZoomPercent(zoomPercent, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
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
    ++m_tileGeneration;
    m_pendingTileKeys.clear();
    m_failedTileKeys.clear();
    m_displayedImageState->setImage(image);
}

void ImagePresentationController::setStaticImage(
    std::shared_ptr<ImageTileSource> source, const QImage &preview)
{
    stopAnimation();
    ++m_tileGeneration;
    m_pendingTileKeys.clear();
    m_failedTileKeys.clear();
    m_displayedImageState->setStaticImage(std::move(source), preview);
    scheduleVisibleTileDecode();
}

void ImagePresentationController::clearImage()
{
    ++m_tileGeneration;
    m_pendingTileKeys.clear();
    m_failedTileKeys.clear();
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
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setImageSize(imageSize, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
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

    std::shared_ptr<ImageTileSource> source = surface->source();
    if (source == nullptr) {
        return;
    }

    const quint64 generation = m_tileGeneration;
    for (const TileKey &key : visibleTileKeys(*surface)) {
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
        QThreadPool::globalInstance()->start(
            QRunnable::create([this, context, source, request, generation, key]() mutable {
                QString errorString;
                std::optional<DecodedTile> tile = source->decodeTile(request, &errorString);
                if (context == nullptr) {
                    return;
                }

                QMetaObject::invokeMethod(
                    context.data(),
                    [this, context, generation, key, tile = std::move(tile)]() mutable {
                        if (context == nullptr) {
                            return;
                        }
                        finishTileDecode(generation, key, std::move(tile));
                    },
                    Qt::QueuedConnection);
            }));
    }
}

std::vector<TileKey> ImagePresentationController::visibleTileKeys(
    const StaticTileSurface &surface) const
{
    std::vector<TileKey> keys;
    const TilePyramid &pyramid = surface.pyramid();
    if (pyramid.imageSize().isEmpty() || m_zoomState.displaySize().isEmpty()
        || m_visibleItemRect.isEmpty()) {
        return keys;
    }

    const qreal displayPixelsPerSourcePixel
        = std::min((m_zoomState.displaySize().width() * displayDevicePixelRatio())
                / pyramid.imageSize().width(),
            (m_zoomState.displaySize().height() * displayDevicePixelRatio())
                / pyramid.imageSize().height());
    const int level = pyramid.selectLevelForDisplayScale(displayPixelsPerSourcePixel);
    const QRect currentLevelRect = levelRectForItemRect(pyramid, level, m_visibleItemRect);
    keys = pyramid.tilesIntersectingLevelRect(level, currentLevelRect);

    QRectF prefetchItemRect = m_visibleItemRect.adjusted(-m_visibleItemRect.width(),
        -m_visibleItemRect.height(), m_visibleItemRect.width(), m_visibleItemRect.height());
    prefetchItemRect
        = prefetchItemRect.intersected(QRectF(QPointF(0.0, 0.0), m_zoomState.displaySize()));
    const QRect prefetchLevelRect = levelRectForItemRect(pyramid, level, prefetchItemRect);
    for (const TileKey &key : pyramid.tilesIntersectingLevelRect(level, prefetchLevelRect)) {
        if (std::find(keys.cbegin(), keys.cend(), key) == keys.cend()) {
            keys.push_back(key);
        }
    }

    return keys;
}

QRect ImagePresentationController::levelRectForItemRect(
    const TilePyramid &pyramid, int level, const QRectF &itemRect) const
{
    if (itemRect.isEmpty() || m_zoomState.displaySize().isEmpty()
        || pyramid.imageSize().isEmpty()) {
        return {};
    }

    const QRectF bounded
        = itemRect.intersected(QRectF(QPointF(0.0, 0.0), m_zoomState.displaySize()));
    if (bounded.isEmpty()) {
        return {};
    }

    const QSize levelSize = pyramid.levelSize(level);
    const qreal xScale = static_cast<qreal>(levelSize.width()) / m_zoomState.displaySize().width();
    const qreal yScale
        = static_cast<qreal>(levelSize.height()) / m_zoomState.displaySize().height();
    const int left
        = std::clamp(static_cast<int>(std::floor(bounded.left() * xScale)), 0, levelSize.width());
    const int top
        = std::clamp(static_cast<int>(std::floor(bounded.top() * yScale)), 0, levelSize.height());
    const int right = std::clamp(
        static_cast<int>(std::ceil(bounded.right() * xScale)), left, levelSize.width());
    const int bottom = std::clamp(
        static_cast<int>(std::ceil(bounded.bottom() * yScale)), top, levelSize.height());
    return QRect(left, top, right - left, bottom - top);
}

bool ImagePresentationController::tileRequestIsCurrent(quint64 generation, const TileKey &key) const
{
    return generation == m_tileGeneration && m_pendingTileKeys.contains(key);
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
