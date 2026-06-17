// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagepagesurfacecontroller.h"

#include "async/imagecallback.h"
#include "presentation/animationlogging.h"
#include "presentation/imageanimationplayer.h"
#include "rendering/imagerendering.h"

#include <QDebug>
#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

namespace kiriview {
namespace {
    struct RasterDisplayRefinementWork {
        quint64 ticket = 0;
        RasterDisplayRefinementDemandKey demandKey;
        ImageDocumentRenderContext renderContext;
        StaticDisplayImagePayload currentDisplay;
        std::shared_ptr<ImageTileSource> source;
        QSize rasterSize;
        DisplayImageQuality quality = DisplayImageQuality::Exact;
    };

    struct RasterDisplayRefinementResult {
        quint64 ticket = 0;
        RasterDisplayRefinementDemandKey demandKey;
        ImageDocumentRenderContext renderContext;
        StaticDisplayImagePayload displayImage;
        bool ready = false;
    };

    RasterDisplayRefinementDemandKey rasterDemandKey(const StaticDisplayImagePayload &displayImage,
        const ImagePresentationRenderProjection &projection, quint64 displaySourceRevision,
        qsizetype displayImageByteBudget, quint64 renderRevision,
        const RasterDisplayBucketKey &bucketKey)
    {
        return RasterDisplayRefinementDemandKey {
            displayImage.sourceIdentity,
            displayImage.originalSize,
            projection.pageRole,
            displaySourceRevision,
            projection.imageRevision,
            projection.renderContextGeneration,
            projection.renderContextGeneration,
            static_cast<quint64>(std::max<qsizetype>(0, displayImageByteBudget)),
            static_cast<quint64>(std::max(0, projection.rotationDegrees)),
            renderRevision,
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

    ImageDisplaySourceStatus displaySourceStatusForLoadOutcome(ImageDisplayLoadOutcome outcome)
    {
        switch (outcome) {
        case ImageDisplayLoadOutcome::Loaded:
            return ImageDisplaySourceStatus::Ready;
        case ImageDisplayLoadOutcome::Error:
            return ImageDisplaySourceStatus::Error;
        case ImageDisplayLoadOutcome::Missing:
            return ImageDisplaySourceStatus::Missing;
        }

        return ImageDisplaySourceStatus::Error;
    }

    ImageDisplaySourceSlot displayErrorSourceSlot(const QSize &imageSize, quint64 revision)
    {
        return ImageDisplaySourceSlot {
            QUrl(),
            revision,
            QString(),
            imageSize,
            imageSize,
            QSize(),
            DisplayImageQuality::Exact,
            ImageDisplaySourceStatus::Error,
            false,
            false,
            ImageDisplaySourceRetentionStatus::None,
            false,
        };
    }

    ImagePresentationPageSlotSource pageSlotSource(
        bool hasImage, const ImageDisplaySourceSlot &displaySource)
    {
        if (!hasImage) {
            return ImagePresentationPageSlotSource::empty();
        }
        if (displaySource.status == ImageDisplaySourceStatus::Error
            && displaySource.providerUrl.isEmpty()) {
            return ImagePresentationPageSlotSource::displayError(displaySource);
        }

        return ImagePresentationPageSlotSource::providerReady(displaySource);
    }

    StaticDisplayImagePayload refinedDisplayImagePayload(
        RasterDisplayRefinementWork work, QImage image)
    {
        work.currentDisplay.image = displayReadyImage(std::move(image));
        work.currentDisplay.quality = work.quality;
        work.currentDisplay.displayPixelsPerSourcePixel = imagePixelsPerSourcePixel(
            work.currentDisplay.originalSize, work.currentDisplay.image.size());
        work.currentDisplay.previewOrigin = DisplayImagePreviewOrigin::None;
        work.currentDisplay.refinementSource = std::move(work.source);
        return std::move(work.currentDisplay);
    }

    RasterDisplayRefinementResult runRasterDisplayRefinement(RasterDisplayRefinementWork work)
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
    std::shared_ptr<DisplayImageStore> displayImageStore, DisplayedPageRole pageRole,
    ImageWorkerScheduler workerScheduler)
    : m_callbacks(std::move(callbacks))
    , m_context(context)
    , m_predecodeCacheByteBudget(cacheBudgets.predecodeCacheByteBudget)
    , m_displayImageByteBudget(cacheBudgets.displayImageCacheByteBudget)
    , m_displayImageStore(
          displayImageStore == nullptr ? sharedDisplayImageStore() : std::move(displayImageStore))
    , m_pageRole(pageRole)
    , m_workerScheduler(std::move(workerScheduler))
{
    m_animationPlayer = std::make_unique<ImageAnimationPlayer>(
        context,
        [this](const QImage &image) { setAnimationFrame(image, m_animationFrameSourceIdentity); },
        [this](
            const QString &errorString) { invokeIfSet(m_callbacks.animationError, errorString); },
        [this]() { releaseRetainedAnimationFrameEntry(); });
}

ImagePageSurfaceController::~ImagePageSurfaceController()
{
    releaseShadowDisplayEntry();
    releaseRetainedAnimationFrameEntry();
    releaseCurrentDisplayEntry();
}

QSize ImagePageSurfaceController::imageSize() const { return m_imageSize; }

quint64 ImagePageSurfaceController::imageRevision() const { return m_imageRevision; }

bool ImagePageSurfaceController::hasImage() const { return m_hasImage; }

bool ImagePageSurfaceController::isPredecodeCacheable() const { return m_predecodeCacheable; }

qsizetype ImagePageSurfaceController::predecodeCacheByteBudget() const
{
    return m_predecodeCacheByteBudget;
}

std::optional<StaticDisplayImagePayload> ImagePageSurfaceController::displayImage() const
{
    return m_displayImage;
}

ImagePresentationPageSlotSnapshot ImagePageSurfaceController::snapshot() const
{
    return ImagePresentationPageSlotSnapshot {
        imageRevision(),
        imageSize(),
        pageSlotSource(hasImage(), m_displaySource),
    };
}

void ImagePageSurfaceController::setImage(const QImage &image, bool predecodeCacheable)
{
    cancelRasterDisplayRefinement();
    clearShadowDisplayImage();
    clearDisplaySource();
    m_animationFrameSourceIdentity.clear();
    if (!image.isNull()) {
        ++m_displaySourceRevision;
        m_displaySource = displayErrorSourceSlot(image.size(), m_displaySourceRevision);
    }
    acceptImageState(image.size(), predecodeCacheable, std::nullopt);
}

void ImagePageSurfaceController::setAnimationFrame(
    const QImage &image, const QString &sourceIdentity)
{
    cancelRasterDisplayRefinement();
    clearShadowDisplayImage();
    m_animationFrameSourceIdentity = sourceIdentity;

    const QImage displayImage = displayReadyImage(image);
    publishAnimationFrameDisplaySource(displayImage, sourceIdentity);
    acceptImageState(displayImage.size(), false, std::nullopt);
    notify(ImageDocumentChange::DisplaySource);
}

void ImagePageSurfaceController::setStaticDisplayImage(StaticDisplayImagePayload displayImage,
    bool predecodeCacheable, const ImageDocumentRenderContext &renderContext)
{
    cancelRasterDisplayRefinement();
    stopAnimation();
    clearShadowDisplayImage();
    m_animationFrameSourceIdentity.clear();
    publishDisplaySource(displayImage);

    Q_UNUSED(renderContext);
    StaticDisplayImagePayload storedDisplay = std::move(displayImage);
    const QSize imageSize = storedDisplay.originalSize;
    acceptImageState(imageSize, predecodeCacheable, std::move(storedDisplay));
}

void ImagePageSurfaceController::updateDisplayProjection(
    const ImagePresentationRenderProjection &projection)
{
    if (!projection.visible || projection.visibleItemRect.isEmpty()) {
        cancelRasterDisplayRefinement();
        updateDisplaySourceVisibility(false);
        return;
    }

    updateDisplaySourceVisibility(true);
    scheduleRasterDisplayRefinement(projection);
}

void ImagePageSurfaceController::clearImage()
{
    cancelRasterDisplayRefinement();
    stopAnimation();
    clearShadowDisplayImage();
    clearDisplaySource();
    m_animationFrameSourceIdentity.clear();
    m_imageSize = {};
    m_hasImage = false;
    m_predecodeCacheable = false;
    m_displayImage = std::nullopt;
    ++m_imageRevision;
}

void ImagePageSurfaceController::startAnimation(ImageAnimationPlaybackRequest request)
{
    m_animationPlayer->start(std::move(request));
}

void ImagePageSurfaceController::stopAnimation() { m_animationPlayer->stop(); }

void ImagePageSurfaceController::acceptImageState(
    QSize imageSize, bool predecodeCacheable, std::optional<StaticDisplayImagePayload> displayImage)
{
    m_imageSize = imageSize;
    m_hasImage = !imageSize.isEmpty();
    m_predecodeCacheable = predecodeCacheable;
    m_displayImage = std::move(displayImage);
    ++m_imageRevision;
}

void ImagePageSurfaceController::publishDisplaySource(const StaticDisplayImagePayload &displayImage)
{
    releaseRetainedAnimationFrameEntry();
    clearAnimationFrameLoadContract();
    releaseCurrentDisplayEntry();
    m_animationFrameSourceIdentity.clear();

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
    const QUrl providerUrl = displayImageSourceForId(entryId);
    const bool loadAcknowledgmentRequired = m_displayImageStore != nullptr && !entryId.isEmpty()
        && m_displayImageStore->acquirePinLease(entryId, DisplayImagePinKind::PendingLoad);

    m_displayEntryId = entryId;
    m_displayEntryVisiblePinned = false;
    m_pendingStillImageEntryId = loadAcknowledgmentRequired ? entryId : QString();
    m_pendingStillImageProviderUrl = loadAcknowledgmentRequired ? providerUrl : QUrl();
    m_pendingStillImageRevision = loadAcknowledgmentRequired ? m_displaySourceRevision : 0;
    m_pendingStillImageSourceIdentity
        = loadAcknowledgmentRequired ? displayImage.sourceIdentity : QString();
    m_stillImageDisplayLoadPending = loadAcknowledgmentRequired;
    m_displaySource = ImageDisplaySourceSlot {
        providerUrl,
        m_displaySourceRevision,
        displayImage.sourceIdentity,
        displayImage.originalSize,
        rasterSize,
        rasterSize != displayImage.originalSize ? QSize(rasterSize.width(), 0) : QSize(),
        displayImage.quality,
        entryId.isEmpty() ? ImageDisplaySourceStatus::Error : ImageDisplaySourceStatus::Ready,
        false,
        loadAcknowledgmentRequired,
        ImageDisplaySourceRetentionStatus::None,
        false,
    };
}

void ImagePageSurfaceController::publishAnimationFrameDisplaySource(
    const QImage &image, const QString &sourceIdentity)
{
    retainCurrentAnimationFrameEntryForLoad();

    ++m_displaySourceRevision;
    const QSize rasterSize = image.size();
    const QString entryId = m_displayImageStore == nullptr
        ? QString()
        : m_displayImageStore->insert(DisplayImageEntry {
              image,
              rasterSize,
              rasterSize,
              sourceIdentity,
              m_pageRole,
              DisplayImageQuality::Exact,
              DisplayImageRetentionPriority::Nearby,
              m_displaySourceRevision,
              QStringLiteral("animation-frame"),
              DisplayImagePreviewOrigin::None,
          });

    m_displayEntryId = entryId;
    m_displayEntryVisiblePinned = false;
    m_currentDisplayEntryIsAnimationFrame = true;
    const QUrl providerUrl = displayImageSourceForId(entryId);
    const bool loadAcknowledgmentRequired = !entryId.isEmpty();
    m_animationFrameDisplayLoadPending = loadAcknowledgmentRequired;
    m_pendingAnimationFrameProviderUrl = providerUrl;
    m_pendingAnimationFrameRevision = m_displaySourceRevision;
    m_pendingAnimationFrameSourceIdentity = sourceIdentity;
    m_displaySource = ImageDisplaySourceSlot {
        providerUrl,
        m_displaySourceRevision,
        sourceIdentity,
        rasterSize,
        rasterSize,
        {},
        DisplayImageQuality::Exact,
        entryId.isEmpty() ? ImageDisplaySourceStatus::Error : ImageDisplaySourceStatus::Ready,
        false,
        loadAcknowledgmentRequired,
        ImageDisplaySourceRetentionStatus::None,
        false,
    };
    qCDebug(kiriviewAnimationLog) << "animation frame provider source published" << "providerUrl"
                                  << providerUrl << "revision" << m_displaySourceRevision
                                  << "entryId" << entryId << "rasterSize" << rasterSize
                                  << "sourceIdentity" << sourceIdentity
                                  << "loadAcknowledgmentRequired" << loadAcknowledgmentRequired;
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
        && m_retainedAnimationFrameEntryId.isEmpty()
        && m_displaySource.status == ImageDisplaySourceStatus::Missing
        && !m_stillImageDisplayLoadPending && !m_animationFrameDisplayLoadPending) {
        return;
    }

    clearStillImageLoadContract();
    releaseRetainedAnimationFrameEntry();
    clearAnimationFrameLoadContract();
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
    clearStillImageLoadContract();

    if (m_displayImageStore == nullptr || m_displayEntryId.isEmpty()) {
        m_displayEntryId.clear();
        m_displayEntryVisiblePinned = false;
        m_currentDisplayEntryIsAnimationFrame = false;
        return;
    }

    const QString entryId = m_displayEntryId;
    if (m_displayEntryVisiblePinned) {
        m_displayImageStore->releasePinLease(entryId, DisplayImagePinKind::Visible);
    }
    m_displayImageStore->release(entryId);
    m_displayEntryId.clear();
    m_displayEntryVisiblePinned = false;
    m_currentDisplayEntryIsAnimationFrame = false;
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

void ImagePageSurfaceController::retainCurrentAnimationFrameEntryForLoad()
{
    releaseRetainedAnimationFrameEntry();
    clearAnimationFrameLoadContract();

    if (!m_currentDisplayEntryIsAnimationFrame || m_displayEntryId.isEmpty()
        || m_displayImageStore == nullptr) {
        qCDebug(kiriviewAnimationLog)
            << "animation frame retention skipped" << "currentIsAnimationFrame"
            << m_currentDisplayEntryIsAnimationFrame << "entryId" << m_displayEntryId << "hasStore"
            << (m_displayImageStore != nullptr);
        releaseCurrentDisplayEntry();
        return;
    }

    const QString entryId = m_displayEntryId;
    if (m_displayEntryVisiblePinned) {
        qCDebug(kiriviewAnimationLog)
            << "animation frame visible pin released for retention" << "entryId" << entryId;
        m_displayImageStore->releasePinLease(entryId, DisplayImagePinKind::Visible);
    }

    const bool retained
        = m_displayImageStore->acquirePinLease(entryId, DisplayImagePinKind::FrameRetention);
    m_displayImageStore->release(entryId);
    m_displayEntryId.clear();
    m_displayEntryVisiblePinned = false;
    m_currentDisplayEntryIsAnimationFrame = false;
    if (retained) {
        m_retainedAnimationFrameEntryId = entryId;
    }
    qCDebug(kiriviewAnimationLog) << "animation frame retention acquired" << "entryId" << entryId
                                  << "retained" << retained;
}

void ImagePageSurfaceController::releaseRetainedAnimationFrameEntry()
{
    if (m_displayImageStore == nullptr || m_retainedAnimationFrameEntryId.isEmpty()) {
        qCDebug(kiriviewAnimationLog)
            << "animation frame retention release skipped" << "entryId"
            << m_retainedAnimationFrameEntryId << "hasStore" << (m_displayImageStore != nullptr);
        m_retainedAnimationFrameEntryId.clear();
        return;
    }

    qCDebug(kiriviewAnimationLog) << "animation frame retention released" << "entryId"
                                  << m_retainedAnimationFrameEntryId;
    m_displayImageStore->releasePinLease(
        m_retainedAnimationFrameEntryId, DisplayImagePinKind::FrameRetention);
    m_retainedAnimationFrameEntryId.clear();
}

void ImagePageSurfaceController::clearStillImageLoadContract()
{
    if (m_displayImageStore != nullptr && m_stillImageDisplayLoadPending
        && !m_pendingStillImageEntryId.isEmpty()) {
        m_displayImageStore->releasePinLease(
            m_pendingStillImageEntryId, DisplayImagePinKind::PendingLoad);
    }

    m_stillImageDisplayLoadPending = false;
    m_pendingStillImageEntryId.clear();
    m_pendingStillImageProviderUrl = QUrl();
    m_pendingStillImageRevision = 0;
    m_pendingStillImageSourceIdentity.clear();
}

void ImagePageSurfaceController::clearAnimationFrameLoadContract()
{
    m_animationFrameDisplayLoadPending = false;
    m_pendingAnimationFrameProviderUrl = QUrl();
    m_pendingAnimationFrameRevision = 0;
    m_pendingAnimationFrameSourceIdentity.clear();
}

bool ImagePageSurfaceController::acknowledgeDisplayImageLoad(const QUrl &providerUrl,
    quint64 revision, const QString &sourceIdentity, ImageDisplayLoadOutcome outcome)
{
    if (m_currentDisplayEntryIsAnimationFrame) {
        return acknowledgeAnimationFrameDisplayLoad(providerUrl, revision, sourceIdentity, outcome);
    }

    return acknowledgeStillImageDisplayLoad(providerUrl, revision, sourceIdentity, outcome);
}

bool ImagePageSurfaceController::acknowledgeStillImageDisplayLoad(const QUrl &providerUrl,
    quint64 revision, const QString &sourceIdentity, ImageDisplayLoadOutcome outcome)
{
    if (m_currentDisplayEntryIsAnimationFrame || !m_stillImageDisplayLoadPending
        || providerUrl != m_pendingStillImageProviderUrl || revision != m_pendingStillImageRevision
        || sourceIdentity != m_pendingStillImageSourceIdentity) {
        return false;
    }

    clearStillImageLoadContract();
    m_displaySource.status = displaySourceStatusForLoadOutcome(outcome);
    m_displaySource.loadAcknowledgmentRequired = false;
    notify(ImageDocumentChange::DisplaySource);
    return true;
}

bool ImagePageSurfaceController::acknowledgeAnimationFrameDisplayLoad(const QUrl &providerUrl,
    quint64 revision, const QString &sourceIdentity, ImageDisplayLoadOutcome outcome)
{
    if (!m_currentDisplayEntryIsAnimationFrame || !m_animationFrameDisplayLoadPending
        || providerUrl != m_pendingAnimationFrameProviderUrl
        || revision != m_pendingAnimationFrameRevision
        || sourceIdentity != m_pendingAnimationFrameSourceIdentity) {
        return false;
    }

    clearAnimationFrameLoadContract();
    releaseRetainedAnimationFrameEntry();
    m_displaySource.status = displaySourceStatusForLoadOutcome(outcome);
    m_displaySource.loadAcknowledgmentRequired = false;
    notify(ImageDocumentChange::DisplaySource);
    return true;
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

    std::shared_ptr<ImageTileSource> source = currentDisplay->refinementSource;
    if (source == nullptr || !source->supportsRasterDisplayRefinement()) {
        cancelRasterDisplayRefinement();
        return;
    }

    const qsizetype displayImageByteBudget = m_displayImageByteBudget;
    const RasterDisplayBucketPolicyInput policyInput {
        currentDisplay->originalSize,
        currentDisplay->image.size(),
        currentDisplay->quality,
        projection.displaySize,
        projection.visibleItemRect,
        projection.devicePixelRatio,
        projection.rotationDegrees,
        projection.maximumTextureSize,
        displayImageByteBudget,
    };
    const RasterDisplayBucketDecision decision = source->isResolutionIndependent()
        ? svgDisplayBucketDecision(policyInput)
        : rasterDisplayBucketDecision(policyInput);
    if (decision.status != RasterDisplayBucketStatus::RefinementNeeded
        || decision.bucketKey.rasterSize == currentDisplay->image.size()) {
        cancelRasterDisplayRefinement();
        return;
    }

    RasterDisplayRefinementDemandKey demandKey = rasterDemandKey(*currentDisplay, projection,
        m_displaySourceRevision, displayImageByteBudget, 0, decision.bucketKey);
    if (m_rasterDisplayRefinementDemand.has_value()) {
        RasterDisplayRefinementDemandKey pendingDemand = *m_rasterDisplayRefinementDemand;
        pendingDemand.renderRevision = 0;
        if (pendingDemand == demandKey) {
            return;
        }
    }

    const quint64 ticket = m_rasterDisplayRefinementTicket.next();
    demandKey.renderRevision = ticket;
    m_rasterDisplayRefinementDemand = demandKey;
    m_workerScheduler.run(
        m_context,
        [work = RasterDisplayRefinementWork {
             ticket,
             std::move(demandKey),
             renderContextForProjection(projection),
             std::move(*currentDisplay),
             std::move(source),
             decision.bucketKey.rasterSize,
             decision.quality,
         }]() mutable { return runRasterDisplayRefinement(std::move(work)); },
        [this](RasterDisplayRefinementResult result) mutable {
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
