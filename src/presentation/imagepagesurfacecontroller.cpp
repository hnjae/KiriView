// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepagesurfacecontroller.h"

#include "async/imageasyncworker.h"
#include "async/imagecallback.h"
#include "presentation/imageanimationplayer.h"
#include "rendering/displayedimagesurfacestate.h"
#include "rendering/imagerendering.h"
#include "rendering/imagetiledecodescheduler.h"
#include "rendering/qimagereadertilesource.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

namespace KiriView {
namespace {
    struct QtRasterRefinementWork {
        quint64 ticket = 0;
        RasterDisplayRefinementDemandKey demandKey;
        ImageDocumentRenderContext renderContext;
        StaticDisplayImagePayload currentDisplay;
        std::shared_ptr<QImageReaderTileSource> source;
        QSize rasterSize;
        DisplayImageQuality quality = DisplayImageQuality::Exact;
    };

    struct QtRasterRefinementResult {
        quint64 ticket = 0;
        RasterDisplayRefinementDemandKey demandKey;
        ImageDocumentRenderContext renderContext;
        StaticDisplayImagePayload displayImage;
        bool ready = false;
    };

    RasterDisplayRefinementDemandKey rasterDemandKey(const StaticDisplayImagePayload &displayImage,
        const ImagePresentationRenderProjection &projection, quint64 displaySourceRevision,
        qsizetype displayImageByteBudget, const RasterDisplayBucketKey &bucketKey)
    {
        return RasterDisplayRefinementDemandKey {
            displayImage.sourceIdentity,
            projection.pageRole,
            displaySourceRevision,
            projection.imageRevision,
            projection.renderContextGeneration,
            projection.renderContextGeneration,
            static_cast<quint64>(std::max<qsizetype>(0, displayImageByteBudget)),
            static_cast<quint64>(std::max(0, projection.rotationDegrees)),
            bucketKey,
        };
    }

    ImageDocumentRenderContext renderContextForProjection(
        const ImagePresentationRenderProjection &projection)
    {
        return ImageDocumentRenderContext {
            projection.devicePixelRatio,
            projection.maximumTextureSize,
            projection.renderContextGeneration,
        };
    }

    StaticDisplayImagePayload refinedDisplayImagePayload(QtRasterRefinementWork work, QImage image)
    {
        work.currentDisplay.image = displayReadyImage(std::move(image));
        work.currentDisplay.quality = work.quality;
        work.currentDisplay.displayPixelsPerSourcePixel = imagePixelsPerSourcePixel(
            work.currentDisplay.originalSize, work.currentDisplay.image.size());
        work.currentDisplay.previewOrigin = DisplayImagePreviewOrigin::None;
        work.currentDisplay.refinementSource = std::move(work.source);
        return std::move(work.currentDisplay);
    }

    QtRasterRefinementResult runQtRasterRefinement(QtRasterRefinementWork work)
    {
        if (work.source == nullptr || work.rasterSize.isEmpty()) {
            return { work.ticket, std::move(work.demandKey), work.renderContext, {}, false };
        }

        QString errorString;
        QImage image = work.source->decodeRasterDisplayImage(work.rasterSize, &errorString);
        Q_UNUSED(errorString);
        if (image.isNull()) {
            return { work.ticket, std::move(work.demandKey), work.renderContext, {}, false };
        }

        return { work.ticket, work.demandKey, work.renderContext,
            refinedDisplayImagePayload(std::move(work), std::move(image)), true };
    }
}

ImagePageSurfaceController::ImagePageSurfaceController(QObject *context,
    ImagePageSurfaceController::Callbacks callbacks, ImageCacheBudgets cacheBudgets,
    std::shared_ptr<DisplayImageStore> displayImageStore, DisplayedPageRole pageRole)
    : m_callbacks(std::move(callbacks))
    , m_context(context)
    , m_predecodeCacheByteBudget(cacheBudgets.predecodeCacheByteBudget)
    , m_staticTileCacheByteBudget(cacheBudgets.staticTileCacheByteBudget)
    , m_displayImageStore(
          displayImageStore == nullptr ? sharedDisplayImageStore() : std::move(displayImageStore))
    , m_pageRole(pageRole)
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

ImagePageSurfaceController::~ImagePageSurfaceController()
{
    releaseShadowDisplayEntry();
    releaseCurrentDisplayEntry();
}

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

std::optional<StaticDisplayImagePayload> ImagePageSurfaceController::displayImage() const
{
    return m_displayedSurfaceState->displayImage();
}

ImagePresentationPageSlotSnapshot ImagePageSurfaceController::snapshot() const
{
    return ImagePresentationPageSlotSnapshot {
        imageSurface(),
        imageRevision(),
        imageSize(),
        hasImage(),
        m_displaySource,
    };
}

void ImagePageSurfaceController::setImage(const QImage &image, bool predecodeCacheable)
{
    cancelRasterDisplayRefinement();
    m_tileDecodeScheduler->invalidate();
    clearShadowDisplayImage();
    clearDisplaySource();
    applyDisplayedImageChange(m_displayedSurfaceState->setImage(image, predecodeCacheable));
}

void ImagePageSurfaceController::setStaticDisplayImage(StaticDisplayImagePayload displayImage,
    bool predecodeCacheable, const ImageDocumentRenderContext &renderContext)
{
    cancelRasterDisplayRefinement();
    stopAnimation();
    m_tileDecodeScheduler->invalidate();
    clearShadowDisplayImage();
    publishDisplaySource(displayImage);

    const StaticImagePayload staticImage = displayImage.compatibilityStaticImage();
    const bool useFullImageSurface
        = staticImageFitsFullImageSurface(staticImage, renderContext.maximumTextureSize);
    const DisplayedImageSurfaceStateChange change
        = m_displayedSurfaceState->setStaticDisplayImage(std::move(displayImage),
            useFullImageSurface, predecodeCacheable, m_staticTileCacheByteBudget);
    applyDisplayedImageChange(change);
}

void ImagePageSurfaceController::setStaticImage(StaticImagePayload staticImage,
    bool predecodeCacheable, const ImageDocumentRenderContext &renderContext)
{
    cancelRasterDisplayRefinement();
    stopAnimation();
    m_tileDecodeScheduler->invalidate();
    clearShadowDisplayImage();
    clearDisplaySource();
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
        cancelRasterDisplayRefinement();
        updateDisplaySourceVisibility(false);
        discardDecodedTiles();
        return;
    }

    updateDisplaySourceVisibility(true);
    m_tileDecodeScheduler->schedule(imageSurface(), projection.displaySize,
        projection.visibleItemRect,
        ImageDocumentRenderContext { projection.devicePixelRatio, projection.maximumTextureSize,
            projection.renderContextGeneration },
        projection.rotationDegrees);
    scheduleRasterDisplayRefinement(projection);
}

void ImagePageSurfaceController::clearImage()
{
    cancelRasterDisplayRefinement();
    stopAnimation();
    m_tileDecodeScheduler->invalidate();
    clearShadowDisplayImage();
    clearDisplaySource();
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

void ImagePageSurfaceController::publishDisplaySource(const StaticDisplayImagePayload &displayImage)
{
    releaseCurrentDisplayEntry();

    ++m_displaySourceRevision;
    const QSize rasterSize = displayImage.image.size();
    const QString entryId = m_displayImageStore == nullptr
        ? QString()
        : m_displayImageStore->insert(DisplayImageEntry {
              displayImage.image,
              displayImage.originalSize,
              rasterSize,
              displayImage.sourceIdentity,
              m_pageRole,
              displayImage.quality,
              DisplayImageRetentionPriority::Nearby,
              m_displaySourceRevision,
              QStringLiteral("static-display"),
              displayImage.previewOrigin,
          });

    m_displayEntryId = entryId;
    m_displayEntryVisiblePinned = false;
    m_displaySource = ImageDisplaySourceSlot {
        displayImageSourceForId(entryId),
        m_displaySourceRevision,
        displayImage.sourceIdentity,
        displayImage.originalSize,
        rasterSize,
        rasterSize != displayImage.originalSize ? QSize(rasterSize.width(), 0) : QSize(),
        displayImage.quality,
        entryId.isEmpty() ? ImageDisplaySourceStatus::Error : ImageDisplaySourceStatus::Ready,
        false,
        false,
        ImageDisplaySourceRetentionStatus::None,
        false,
    };
}

QString ImagePageSurfaceController::publishShadowDisplayImage(
    StaticDisplayImagePayload displayImage)
{
    releaseShadowDisplayEntry();
    if (m_displayImageStore == nullptr || !displayImage.isValid()) {
        return {};
    }

    const QSize rasterSize = displayImage.image.size();
    const QString entryId = m_displayImageStore->insert(DisplayImageEntry {
        displayImage.image,
        displayImage.originalSize,
        rasterSize,
        displayImage.sourceIdentity,
        m_pageRole,
        displayImage.quality,
        DisplayImageRetentionPriority::Nearby,
        0,
        QStringLiteral("shadow-display"),
        displayImage.previewOrigin,
    });
    m_shadowDisplayEntryId = entryId;
    return entryId;
}

void ImagePageSurfaceController::clearShadowDisplayImage() { releaseShadowDisplayEntry(); }

void ImagePageSurfaceController::clearDisplaySource()
{
    if (m_displaySource.providerUrl.isEmpty() && m_displayEntryId.isEmpty()
        && m_displaySource.status == ImageDisplaySourceStatus::Missing) {
        return;
    }

    releaseCurrentDisplayEntry();
    m_displaySource = {};
}

void ImagePageSurfaceController::cancelRasterDisplayRefinement()
{
    m_rasterDisplayRefinementTicket.invalidate();
    m_rasterDisplayRefinementDemand = std::nullopt;
}

void ImagePageSurfaceController::releaseCurrentDisplayEntry()
{
    if (m_displayImageStore == nullptr || m_displayEntryId.isEmpty()) {
        m_displayEntryId.clear();
        m_displayEntryVisiblePinned = false;
        return;
    }

    const QString entryId = m_displayEntryId;
    if (m_displayEntryVisiblePinned) {
        m_displayImageStore->releasePinLease(entryId, DisplayImagePinKind::Visible);
    }
    m_displayImageStore->release(entryId);
    m_displayEntryId.clear();
    m_displayEntryVisiblePinned = false;
}

void ImagePageSurfaceController::releaseShadowDisplayEntry()
{
    if (m_displayImageStore == nullptr || m_shadowDisplayEntryId.isEmpty()) {
        m_shadowDisplayEntryId.clear();
        return;
    }

    m_displayImageStore->release(m_shadowDisplayEntryId);
    m_shadowDisplayEntryId.clear();
}

void ImagePageSurfaceController::updateDisplaySourceVisibility(bool visible)
{
    if (m_displayImageStore == nullptr || m_displayEntryId.isEmpty()) {
        m_displayEntryVisiblePinned = false;
        return;
    }

    if (visible) {
        if (!m_displayEntryVisiblePinned) {
            m_displayEntryVisiblePinned = m_displayImageStore->acquirePinLease(
                m_displayEntryId, DisplayImagePinKind::Visible);
        }
        m_displayImageStore->updatePriority(
            m_displayEntryId, DisplayImageRetentionPriority::Visible);
        return;
    }

    if (m_displayEntryVisiblePinned) {
        m_displayImageStore->releasePinLease(m_displayEntryId, DisplayImagePinKind::Visible);
        m_displayEntryVisiblePinned = false;
    }
    m_displayImageStore->updatePriority(m_displayEntryId, DisplayImageRetentionPriority::Nearby);
}

void ImagePageSurfaceController::scheduleRasterDisplayRefinement(
    const ImagePresentationRenderProjection &projection)
{
    if (!projection.visible || projection.visibleItemRect.isEmpty()) {
        cancelRasterDisplayRefinement();
        return;
    }

    std::optional<StaticDisplayImagePayload> currentDisplay = displayImage();
    if (!currentDisplay.has_value()) {
        cancelRasterDisplayRefinement();
        return;
    }

    std::shared_ptr<QImageReaderTileSource> source
        = std::dynamic_pointer_cast<QImageReaderTileSource>(currentDisplay->refinementSource);
    if (source == nullptr) {
        cancelRasterDisplayRefinement();
        return;
    }

    const qsizetype displayImageByteBudget = m_displayImageStore == nullptr
        ? m_staticTileCacheByteBudget
        : m_displayImageStore->byteBudget();
    const RasterDisplayBucketDecision decision
        = rasterDisplayBucketDecision(RasterDisplayBucketPolicyInput {
            currentDisplay->originalSize,
            currentDisplay->image.size(),
            currentDisplay->quality,
            projection.displaySize,
            projection.visibleItemRect,
            projection.devicePixelRatio,
            projection.rotationDegrees,
            projection.maximumTextureSize,
            displayImageByteBudget,
        });
    if (decision.status != RasterDisplayBucketStatus::RefinementNeeded
        || decision.bucketKey.rasterSize == currentDisplay->image.size()) {
        cancelRasterDisplayRefinement();
        return;
    }

    RasterDisplayRefinementDemandKey demandKey = rasterDemandKey(*currentDisplay, projection,
        m_displaySourceRevision, displayImageByteBudget, decision.bucketKey);
    if (m_rasterDisplayRefinementDemand.has_value()
        && *m_rasterDisplayRefinementDemand == demandKey) {
        return;
    }

    const quint64 ticket = m_rasterDisplayRefinementTicket.next();
    m_rasterDisplayRefinementDemand = demandKey;
    runAsyncWorker(
        m_context,
        [work = QtRasterRefinementWork {
             ticket,
             std::move(demandKey),
             renderContextForProjection(projection),
             std::move(*currentDisplay),
             std::move(source),
             decision.bucketKey.rasterSize,
             decision.quality,
         }]() mutable { return runQtRasterRefinement(std::move(work)); },
        [this](QtRasterRefinementResult result) mutable {
            if (!m_rasterDisplayRefinementTicket.accepts(result.ticket)
                || !m_rasterDisplayRefinementDemand.has_value()
                || *m_rasterDisplayRefinementDemand != result.demandKey) {
                return;
            }

            m_rasterDisplayRefinementDemand = std::nullopt;
            if (!result.ready) {
                return;
            }

            setStaticDisplayImage(std::move(result.displayImage), isPredecodeCacheable(),
                result.renderContext);
            updateDisplaySourceVisibility(true);
            notify(ImageDocumentChange::DisplaySource);
        });
}

void ImagePageSurfaceController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_callbacks.change, change);
}
}
