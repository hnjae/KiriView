// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepagesurfacecontroller.h"

#include "async/imagecallback.h"
#include "presentation/imageanimationplayer.h"
#include "rendering/displayedimagesurfacestate.h"
#include "rendering/imagerendering.h"
#include "rendering/imagetiledecodescheduler.h"

#include <memory>
#include <utility>

namespace KiriView {
ImagePageSurfaceController::ImagePageSurfaceController(QObject *context,
    ImagePageSurfaceController::Callbacks callbacks, ImageCacheBudgets cacheBudgets)
    : m_callbacks(std::move(callbacks))
    , m_predecodeCacheByteBudget(cacheBudgets.predecodeCacheByteBudget)
    , m_staticTileCacheByteBudget(cacheBudgets.staticTileCacheByteBudget)
{
    m_displayedSurfaceState = std::make_unique<DisplayedImageSurfaceState>();
    m_tileDecodeScheduler
        = std::make_unique<ImageTileDecodeScheduler>(context, [this](DecodedTile tile) {
              const std::optional<DisplayedImageSurfaceStateChange> change
                  = m_displayedSurfaceState->insertTile(std::move(tile));
              if (change.has_value()) {
                  applyDisplayedImageTileChange(*change);
              }
          });
    m_animationPlayer = std::make_unique<ImageAnimationPlayer>(
        context, [this](const QImage &image) { setImage(image, false); },
        [this](
            const QString &errorString) { invokeIfSet(m_callbacks.animationError, errorString); });
}

ImagePageSurfaceController::~ImagePageSurfaceController() = default;

QSize ImagePageSurfaceController::imageSize() const { return m_displayedSurfaceState->imageSize(); }

std::shared_ptr<DisplayedImageSurface> ImagePageSurfaceController::imageSurface() const
{
    return m_displayedSurfaceState->imageSurface();
}

quint64 ImagePageSurfaceController::imageRevision() const
{
    return m_displayedSurfaceState->revision();
}

bool ImagePageSurfaceController::hasImage() const { return m_displayedSurfaceState->hasImage(); }

bool ImagePageSurfaceController::isPredecodeCacheable() const
{
    return m_displayedSurfaceState->isPredecodeCacheable();
}

qsizetype ImagePageSurfaceController::predecodeCacheByteBudget() const
{
    return m_predecodeCacheByteBudget;
}

std::optional<StaticImagePayload> ImagePageSurfaceController::staticImage() const
{
    return m_displayedSurfaceState->staticImage();
}

ImagePresentationPageSlotSnapshot ImagePageSurfaceController::snapshot() const
{
    return ImagePresentationPageSlotSnapshot {
        imageSurface(),
        imageRevision(),
        imageSize(),
        hasImage(),
        {},
    };
}

void ImagePageSurfaceController::setImage(const QImage &image, bool predecodeCacheable)
{
    m_tileDecodeScheduler->invalidate();
    applyDisplayedImageChange(m_displayedSurfaceState->setImage(image, predecodeCacheable));
}

void ImagePageSurfaceController::setStaticImage(StaticImagePayload staticImage,
    bool predecodeCacheable, const ImageDocumentRenderContext &renderContext)
{
    stopAnimation();
    m_tileDecodeScheduler->invalidate();
    const bool useFullImageSurface
        = staticImageFitsFullImageSurface(staticImage, renderContext.maximumTextureSize);
    const DisplayedImageSurfaceStateChange change
        = m_displayedSurfaceState->setStaticImage(std::move(staticImage), useFullImageSurface,
            predecodeCacheable, m_staticTileCacheByteBudget);
    applyDisplayedImageChange(change);
}

void ImagePageSurfaceController::discardDecodedTiles()
{
    m_tileDecodeScheduler->invalidate();
    const std::optional<DisplayedImageSurfaceStateChange> change
        = m_displayedSurfaceState->clearTiles();
    if (change.has_value()) {
        applyDisplayedImageTileChange(*change);
    }
}

void ImagePageSurfaceController::scheduleVisibleTileDecode(
    const ImagePresentationRenderProjection &projection)
{
    if (!projection.visible || projection.visibleItemRect.isEmpty()) {
        discardDecodedTiles();
        return;
    }

    m_tileDecodeScheduler->schedule(imageSurface(), projection.displaySize,
        projection.visibleItemRect,
        ImageDocumentRenderContext { projection.devicePixelRatio, projection.maximumTextureSize,
            projection.renderContextGeneration },
        projection.rotationDegrees);
}

void ImagePageSurfaceController::clearImage()
{
    stopAnimation();
    m_tileDecodeScheduler->invalidate();
    const std::optional<DisplayedImageSurfaceStateChange> change = m_displayedSurfaceState->clear();
    if (change.has_value()) {
        applyDisplayedImageChange(*change);
    }
}

void ImagePageSurfaceController::startAnimation(ImageAnimationPlaybackRequest request)
{
    m_animationPlayer->start(std::move(request));
}

void ImagePageSurfaceController::stopAnimation() { m_animationPlayer->stop(); }

void ImagePageSurfaceController::applyDisplayedImageChange(const DisplayedImageSurfaceStateChange &)
{
    notify(ImageDocumentChange::Repaint);
}

void ImagePageSurfaceController::applyDisplayedImageTileChange(
    const DisplayedImageSurfaceStateChange &)
{
    notify(ImageDocumentChange::Repaint);
}

void ImagePageSurfaceController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_callbacks.change, change);
}
}
