// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "decodedimageresult.h"
#include "imagedecodejob.h"
#include "imageurl.h"

#include <QThread>
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::DecodedImageResult;
using KiriView::ImageCandidateListContext;

KiriView::PredecodePolicyInput predecodePolicyInput(
    const KiriView::ArchiveDocumentLocation &archiveDocument,
    KiriView::PredecodeMomentumMode momentumMode, bool powerSaverEnabled)
{
    return KiriView::predecodePolicyInputForArchiveDocument(
        archiveDocument, momentumMode, powerSaverEnabled, QThread::idealThreadCount());
}

std::optional<std::size_t> currentCandidateIndex(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    const auto currentCandidate = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentUrl](const KiriView::ImageNavigationCandidate &candidate) {
            return KiriView::sameNormalizedUrl(candidate.url, currentUrl);
        });
    if (currentCandidate == candidates.cend()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(candidates.cbegin(), currentCandidate));
}

std::vector<QUrl> predecodeWindowImageUrls(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates,
    const std::vector<std::size_t> &indices)
{
    std::vector<QUrl> urls;
    urls.reserve(indices.size());
    for (std::size_t index : indices) {
        if (index < candidates.size()) {
            urls.push_back(candidates.at(index).url);
        }
    }
    return urls;
}

std::vector<QUrl> displayedPredecodeImageUrls(
    const KiriView::ImagePredecodeCoordinator::Context &context)
{
    std::vector<QUrl> urls;
    if (context.primaryImage.hasLocation()) {
        urls.push_back(context.primaryImage.location.imageUrl());
    }
    if (context.secondaryImage.has_value() && context.secondaryImage->hasLocation()) {
        urls.push_back(context.secondaryImage->location.imageUrl());
    }

    return urls;
}

void cacheDisplayedPredecodeImage(
    KiriView::PredecodeCache &cache, const KiriView::DisplayedPredecodeImage &image)
{
    if (!image.isCacheable()) {
        return;
    }

    cache.cacheDisplayedImage(
        true, image.location.imageUrl(), image.location.archiveDocument(), *image.staticImage);
}

void cacheDisplayedPredecodeImages(
    KiriView::PredecodeCache &cache, const KiriView::ImagePredecodeCoordinator::Context &context)
{
    cacheDisplayedPredecodeImage(cache, context.primaryImage);
    if (context.secondaryImage.has_value()) {
        cacheDisplayedPredecodeImage(cache, *context.secondaryImage);
    }
}
}

namespace KiriView {
ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent)
    : ImagePredecodeCoordinator(
          parent, ImageNavigationCandidateProvider {}, ImageDecodeDependencies {})
{
}

ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent,
    ImageNavigationCandidateProvider candidateProvider, ImageDecodeDependencies decodeDependencies)
    : QObject(parent)
    , m_decodeDependencies(imageDecodeDependenciesWithDefaults(std::move(decodeDependencies)))
    , m_candidateRepository(std::move(candidateProvider))
{
    m_monotonicClock.start();
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(predecodeDebounceMsec());
    m_neutralTimer.setSingleShot(true);
    m_neutralTimer.setInterval(predecodeNeutralRefreshMsec());

    QObject::connect(
        &m_debounceTimer, &QTimer::timeout, this, [this]() { startDebouncedPredecode(); });
    QObject::connect(
        &m_neutralTimer, &QTimer::timeout, this, [this]() { scheduleSettledNeutralPredecode(); });
}

void ImagePredecodeCoordinator::schedule(Context context)
{
    cancelBackgroundWork();
    if (!context.primaryImage.hasLocation() || !context.primaryImage.hasStaticImage()) {
        return;
    }

    const quint64 generation = m_generation.next();
    updateNavigationMomentum(context.pageIndex, currentMonotonicMsec());
    m_firstDisplayContext = context.firstDisplayContext;
    cacheDisplayedImages(context);
    m_displayedContext = context;
    if (m_powerSaverEnabled) {
        m_cache.setWindowUrls({});
        return;
    }

    m_pendingContext = std::move(context);
    m_pendingGeneration = generation;
    m_debounceTimer.start();
    m_neutralTimer.start();
}

void ImagePredecodeCoordinator::setPowerSaverEnabled(bool enabled)
{
    if (m_powerSaverEnabled == enabled) {
        return;
    }

    m_powerSaverEnabled = enabled;
    if (enabled) {
        cancelBackgroundWork();
        m_cache.setWindowUrls({});
        return;
    }

    if (m_displayedContext.has_value()) {
        schedule(*m_displayedContext);
    }
}

bool ImagePredecodeCoordinator::powerSaverEnabled() const { return m_powerSaverEnabled; }

void ImagePredecodeCoordinator::cacheDisplayedImages(const Context &context)
{
    m_cache.setDisplayedUrls(displayedPredecodeImageUrls(context));
    cacheDisplayedPredecodeImages(m_cache, context);
}

void ImagePredecodeCoordinator::startDebouncedPredecode()
{
    if (!m_pendingContext.has_value() || !m_generation.accepts(m_pendingGeneration)) {
        return;
    }

    scheduleAdjacentImagePredecode(*m_pendingContext, m_pendingGeneration);
}

void ImagePredecodeCoordinator::scheduleSettledNeutralPredecode()
{
    if (!m_pendingContext.has_value() || m_momentumState.mode == PredecodeMomentumMode::Neutral) {
        return;
    }

    const Context context = *m_pendingContext;
    m_debounceTimer.stop();
    m_listerJob.cancel();
    m_activePredecodeRequests.cancel();
    m_cache.clearQueuedLoads();
    m_momentumState.mode = PredecodeMomentumMode::Neutral;
    m_firstDisplayContext = context.firstDisplayContext;
    m_pendingGeneration = m_generation.next();
    m_pendingContext = context;
    scheduleAdjacentImagePredecode(context, m_pendingGeneration);
}

void ImagePredecodeCoordinator::scheduleAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const PredecodePolicyInput policyInput = predecodePolicyInput(
        context.primaryImage.location.archiveDocument(), m_momentumState.mode, m_powerSaverEnabled);
    const PredecodeSchedulePlan initialPlan = predecodeSchedulePlan(0, std::nullopt, policyInput);
    const std::size_t parallelLimit = initialPlan.parallelLimit;
    if (parallelLimit == 0) {
        startPredecodeImageLoads({}, context.primaryImage.location.archiveDocument(), context,
            generation, parallelLimit);
        return;
    }

    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateListContextForDisplayedImage(context.primaryImage.location);
    if (!candidateContext.has_value()) {
        startPredecodeImageLoads({}, context.primaryImage.location.archiveDocument(), context,
            generation, parallelLimit);
        return;
    }

    const QUrl currentUrl = candidateContext->currentUrl();
    const ArchiveDocumentLocation archiveDocument = candidateContext->archiveDocument();
    m_listerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, context, generation, currentUrl, archiveDocument, policyInput](
            std::vector<ImageNavigationCandidate> candidates) {
            const PredecodeSchedulePlan plan = predecodeSchedulePlan(
                candidates.size(), currentCandidateIndex(candidates, currentUrl), policyInput);
            startPredecodeImageLoads(predecodeWindowImageUrls(candidates, plan.targetIndices),
                archiveDocument, context, generation, plan.parallelLimit);
        },
        [this, context, generation, archiveDocument, parallelLimit](const QString &) {
            startPredecodeImageLoads({}, archiveDocument, context, generation, parallelLimit);
        });
}

void ImagePredecodeCoordinator::startPredecodeImageLoads(const std::vector<QUrl> &urls,
    const ArchiveDocumentLocation &archiveDocument, const Context &context, quint64 generation,
    std::size_t parallelLimit)
{
    if (!m_generation.accepts(generation)) {
        return;
    }

    m_parallelLimit = parallelLimit;
    m_cache.setDisplayedUrls(displayedPredecodeImageUrls(context));
    m_cache.setWindowUrls(urls);
    cacheDisplayedPredecodeImages(m_cache, context);
    m_cache.enqueueMissingWindowLoads(context.primaryImage.location.imageUrl(), archiveDocument,
        m_activePredecodeRequests.urls());

    startNextPredecodeImageLoads(generation);
}

void ImagePredecodeCoordinator::startNextPredecodeImageLoads(quint64 generation)
{
    if (!m_generation.accepts(generation) || m_parallelLimit == 0) {
        return;
    }

    while (m_activePredecodeRequests.size() < m_parallelLimit) {
        const std::optional<KiriView::PredecodeRequest> request
            = m_cache.takeNextRequest(m_activePredecodeRequests.urls());
        if (!request.has_value()) {
            return;
        }
        if (!startPredecodeImageLoad(request->url, request->archiveDocument, generation)) {
            return;
        }
    }
}

bool ImagePredecodeCoordinator::startPredecodeImageLoad(
    const QUrl &url, const ArchiveDocumentLocation &archiveDocument, quint64 generation)
{
    const bool urlAvailable = url.isValid() && !url.isEmpty();
    const bool cached = urlAvailable && m_cache.hasImage(url);
    const bool inWindow = urlAvailable && m_cache.windowContains(url);
    if (!urlAvailable || m_activePredecodeRequests.size() >= m_parallelLimit || cached || !inWindow
        || m_activePredecodeRequests.containsUrl(url)) {
        return false;
    }

    ImageDecodeRequest request = ImageDecodeRequest::fromLocation(
        generation, DisplayedImageLocation::fromUrl(url, archiveDocument), m_firstDisplayContext);
    auto *decodeJob = new ImageDecodeJob(this, m_decodeDependencies,
        ImageDecodeJob::Callbacks {
            [this](ImageDecodeRequest request, DecodedImageResult result) {
                finishPredecodeImageDecode(request, result);
            },
            [this](const ImageDecodeRequest &request, const QString &) {
                finishPredecodeImageLoadError(request);
            },
        });
    m_activePredecodeRequests.add(request, decodeJob);
    decodeJob->start(std::move(request));
    return true;
}

void ImagePredecodeCoordinator::finishPredecodeImageLoadError(const ImageDecodeRequest &request)
{
    if (!m_activePredecodeRequests.finish(request).has_value()) {
        return;
    }

    startNextPredecodeImageLoads(request.id());
}

void ImagePredecodeCoordinator::finishPredecodeImageDecode(
    ImageDecodeRequest request, const DecodedImageResult &result)
{
    std::optional<ImageDecodeRequest> activeRequest = m_activePredecodeRequests.finish(request);
    if (!activeRequest.has_value()) {
        return;
    }

    const auto *staticImage = decodedImageResultImageAs<StaticDecodedImage>(result);
    if (staticImage != nullptr) {
        m_cache.cacheImage(
            request.imageUrl(), activeRequest->archiveDocument(), staticImage->staticImage);
    }

    startNextPredecodeImageLoads(request.id());
}

void ImagePredecodeCoordinator::cancelBackgroundWork()
{
    m_generation.invalidate();
    m_debounceTimer.stop();
    m_neutralTimer.stop();
    m_listerJob.cancel();
    m_activePredecodeRequests.cancel();
    m_pendingContext.reset();
    m_pendingGeneration = 0;
    m_parallelLimit = 1;
    m_firstDisplayContext = {};
    m_cache.clearQueuedLoads();
}

void ImagePredecodeCoordinator::resetNavigationMomentum() { m_momentumState = {}; }

void ImagePredecodeCoordinator::updateNavigationMomentum(int pageIndex, qint64 monotonicMsec)
{
    m_momentumState = predecodeUpdatedMomentumState(m_momentumState, pageIndex, monotonicMsec);
}

qint64 ImagePredecodeCoordinator::currentMonotonicMsec() const
{
    return m_monotonicClock.isValid() ? m_monotonicClock.elapsed() : 0;
}

void ImagePredecodeCoordinator::cancel()
{
    cancelBackgroundWork();
    m_displayedContext.reset();
    resetNavigationMomentum();
}

void ImagePredecodeCoordinator::clear()
{
    cancel();
    m_cache.clear();
}

std::optional<PredecodedImage> ImagePredecodeCoordinator::tryTake(const QUrl &url) const
{
    return m_cache.findImage(url);
}
}
