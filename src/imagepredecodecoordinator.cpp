// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "decodedimageresult.h"
#include "imageurl.h"
#include "kiriview/src/predecodecachepolicy.cxx.h"

#include <QThread>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::DecodedImageResult;
using KiriView::ImageCandidateListContext;
using KiriView::normalizedImageUrl;

constexpr int predecodeDebounceMsec = 120;
constexpr int predecodeNeutralRefreshMsec = 800;
constexpr int predecodeBiasedNavigationMsec = 450;

std::uint8_t rustFlag(bool value) { return value ? 1 : 0; }

KiriView::RustPredecodeDocumentKind rustDocumentKind(
    const KiriView::ArchiveDocumentLocation &archiveDocument)
{
    if (archiveDocument.isEmpty()) {
        return KiriView::RustPredecodeDocumentKind::Regular;
    }
    if (archiveDocument.isDirectory()) {
        return KiriView::RustPredecodeDocumentKind::DirectoryDocument;
    }

    return KiriView::RustPredecodeDocumentKind::ArchiveDocument;
}

KiriView::RustPredecodeMomentumMode rustMomentumMode(
    KiriView::ImagePredecodeCoordinator::MomentumMode mode)
{
    switch (mode) {
    case KiriView::ImagePredecodeCoordinator::MomentumMode::Neutral:
        return KiriView::RustPredecodeMomentumMode::Neutral;
    case KiriView::ImagePredecodeCoordinator::MomentumMode::NextBiased:
        return KiriView::RustPredecodeMomentumMode::NextBiased;
    case KiriView::ImagePredecodeCoordinator::MomentumMode::PrevBiased:
        return KiriView::RustPredecodeMomentumMode::PrevBiased;
    case KiriView::ImagePredecodeCoordinator::MomentumMode::ScrubbingNext:
        return KiriView::RustPredecodeMomentumMode::ScrubbingNext;
    case KiriView::ImagePredecodeCoordinator::MomentumMode::ScrubbingPrev:
        return KiriView::RustPredecodeMomentumMode::ScrubbingPrev;
    }

    return KiriView::RustPredecodeMomentumMode::Neutral;
}

KiriView::ImagePredecodeCoordinator::MomentumMode momentumMode(
    KiriView::RustPredecodeMomentumMode mode)
{
    switch (mode) {
    case KiriView::RustPredecodeMomentumMode::Neutral:
        return KiriView::ImagePredecodeCoordinator::MomentumMode::Neutral;
    case KiriView::RustPredecodeMomentumMode::NextBiased:
        return KiriView::ImagePredecodeCoordinator::MomentumMode::NextBiased;
    case KiriView::RustPredecodeMomentumMode::PrevBiased:
        return KiriView::ImagePredecodeCoordinator::MomentumMode::PrevBiased;
    case KiriView::RustPredecodeMomentumMode::ScrubbingNext:
        return KiriView::ImagePredecodeCoordinator::MomentumMode::ScrubbingNext;
    case KiriView::RustPredecodeMomentumMode::ScrubbingPrev:
        return KiriView::ImagePredecodeCoordinator::MomentumMode::ScrubbingPrev;
    }

    return KiriView::ImagePredecodeCoordinator::MomentumMode::Neutral;
}

KiriView::RustPredecodePolicyInput rustPredecodePolicyInput(
    const KiriView::ArchiveDocumentLocation &archiveDocument,
    KiriView::ImagePredecodeCoordinator::MomentumMode momentumMode, bool powerSaverEnabled)
{
    KiriView::RustPredecodePolicyInput input {};
    input.document_kind = rustDocumentKind(archiveDocument);
    input.momentum_mode = rustMomentumMode(momentumMode);
    input.power_saver_enabled = powerSaverEnabled;
    input.ideal_thread_count = QThread::idealThreadCount();
    return input;
}

rust::Vec<std::uint8_t> currentCandidateMatches(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &currentUrl)
{
    rust::Vec<std::uint8_t> matches;
    matches.reserve(candidates.size());
    for (const KiriView::ImageNavigationCandidate &candidate : candidates) {
        matches.push_back(rustFlag(KiriView::sameNormalizedUrl(candidate.url, currentUrl)));
    }

    return matches;
}

std::vector<QUrl> predecodeWindowImageUrls(
    const std::vector<KiriView::ImageNavigationCandidate> &candidates, const QUrl &currentUrl,
    KiriView::RustPredecodePolicyInput policyInput)
{
    std::vector<QUrl> urls;
    const rust::Vec<std::size_t> indices = KiriView::rustPredecodeTargetImageIndices(
        currentCandidateMatches(candidates, currentUrl), policyInput);
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
    , m_decodeJob(this, std::move(decodeDependencies),
          ImageDecodeJob::Callbacks {
              [this](ImageDecodeRequest request, DecodedImageResult result) {
                  finishPredecodeImageDecode(request, result);
              },
              [this](const ImageDecodeRequest &request, const QString &) {
                  finishPredecodeImageLoadError(request);
              },
          })
    , m_candidateRepository(std::move(candidateProvider))
{
    m_monotonicClock.start();
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(predecodeDebounceMsec);
    m_neutralTimer.setSingleShot(true);
    m_neutralTimer.setInterval(predecodeNeutralRefreshMsec);

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
    m_pendingContext = std::move(context);
    m_pendingGeneration = generation;
    m_debounceTimer.start();
    m_neutralTimer.start();
}

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
    if (!m_pendingContext.has_value() || m_momentumMode == MomentumMode::Neutral) {
        return;
    }

    const Context context = *m_pendingContext;
    m_debounceTimer.stop();
    m_listerJob.cancel();
    m_decodeJob.cancel();
    clearActivePredecodeRequest();
    m_cache.clearQueuedLoads();
    m_momentumMode = MomentumMode::Neutral;
    m_firstDisplayContext = context.firstDisplayContext;
    m_pendingGeneration = m_generation.next();
    m_pendingContext = context;
    scheduleAdjacentImagePredecode(context, m_pendingGeneration);
}

void ImagePredecodeCoordinator::scheduleAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const RustPredecodePolicyInput policyInput = rustPredecodePolicyInput(
        context.primaryImage.location.archiveDocument(), m_momentumMode, false);
    const std::size_t parallelLimit = rustPredecodeParallelLimit(policyInput);
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
        [this, context, generation, currentUrl, archiveDocument, policyInput, parallelLimit](
            std::vector<ImageNavigationCandidate> candidates) {
            startPredecodeImageLoads(predecodeWindowImageUrls(candidates, currentUrl, policyInput),
                archiveDocument, context, generation, parallelLimit);
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
    m_cache.enqueueMissingWindowLoads(
        context.primaryImage.location.imageUrl(), archiveDocument, activePredecodeUrl());

    startNextPredecodeImageLoad(generation);
}

void ImagePredecodeCoordinator::startNextPredecodeImageLoad(quint64 generation)
{
    if (!m_generation.accepts(generation) || m_parallelLimit == 0) {
        return;
    }

    const std::optional<KiriView::PredecodeRequest> request
        = m_cache.takeNextRequest(activePredecodeUrl());
    if (!request.has_value()) {
        return;
    }

    startPredecodeImageLoad(request->url, request->archiveDocument, generation);
}

void ImagePredecodeCoordinator::startPredecodeImageLoad(
    const QUrl &url, const ArchiveDocumentLocation &archiveDocument, quint64 generation)
{
    const bool urlAvailable = url.isValid() && !url.isEmpty();
    const bool cached = urlAvailable && m_cache.hasImage(url);
    const bool inWindow = urlAvailable && m_cache.windowContains(url);
    if (!urlAvailable || hasActivePredecodeRequest() || cached || !inWindow) {
        return;
    }

    const QUrl normalizedUrl = normalizedImageUrl(url);
    ImageDecodeRequest request = ImageDecodeRequest::fromLocation(
        generation, DisplayedImageLocation::fromUrl(url, archiveDocument), m_firstDisplayContext);
    m_activePredecodeRequest = ActivePredecodeRequest { request, normalizedUrl };
    m_decodeJob.start(std::move(request));
}

void ImagePredecodeCoordinator::finishPredecodeImageLoadError(const ImageDecodeRequest &request)
{
    if (!takeActivePredecodeRequest(request).has_value()) {
        return;
    }

    startNextPredecodeImageLoad(request.id());
}

void ImagePredecodeCoordinator::finishPredecodeImageDecode(
    ImageDecodeRequest request, const DecodedImageResult &result)
{
    std::optional<ActivePredecodeRequest> activeRequest = takeActivePredecodeRequest(request);
    if (!activeRequest.has_value()) {
        return;
    }

    const auto *staticImage = decodedImageResultImageAs<StaticDecodedImage>(result);
    if (staticImage != nullptr) {
        m_cache.cacheImage(
            request.imageUrl(), activeRequest->request.archiveDocument(), staticImage->staticImage);
    }

    startNextPredecodeImageLoad(request.id());
}

QUrl ImagePredecodeCoordinator::activePredecodeUrl() const
{
    return m_activePredecodeRequest.has_value() ? m_activePredecodeRequest->normalizedUrl : QUrl();
}

bool ImagePredecodeCoordinator::hasActivePredecodeRequest() const
{
    return m_activePredecodeRequest.has_value();
}

std::optional<ImagePredecodeCoordinator::ActivePredecodeRequest>
ImagePredecodeCoordinator::takeActivePredecodeRequest(const ImageDecodeRequest &request)
{
    if (!predecodeRequestIsActive(request)) {
        return std::nullopt;
    }

    std::optional<ActivePredecodeRequest> activeRequest = std::move(m_activePredecodeRequest);
    clearActivePredecodeRequest();
    return activeRequest;
}

bool ImagePredecodeCoordinator::predecodeRequestIsActive(const ImageDecodeRequest &request) const
{
    return m_activePredecodeRequest.has_value()
        && m_activePredecodeRequest->request.matches(request);
}

void ImagePredecodeCoordinator::clearActivePredecodeRequest() { m_activePredecodeRequest.reset(); }

void ImagePredecodeCoordinator::cancelBackgroundWork()
{
    m_generation.invalidate();
    m_debounceTimer.stop();
    m_neutralTimer.stop();
    m_listerJob.cancel();
    m_decodeJob.cancel();
    clearActivePredecodeRequest();
    m_pendingContext.reset();
    m_pendingGeneration = 0;
    m_parallelLimit = 1;
    m_firstDisplayContext = {};
    m_cache.clearQueuedLoads();
}

void ImagePredecodeCoordinator::resetNavigationMomentum()
{
    m_lastPageIndex = -1;
    m_lastNavigationMsec = -1;
    m_sameDirectionMoveCount = 0;
    m_lastMomentumDirection = MomentumDirection::None;
    m_momentumMode = MomentumMode::Neutral;
}

void ImagePredecodeCoordinator::updateNavigationMomentum(int pageIndex, qint64 monotonicMsec)
{
    m_momentumMode = MomentumMode::Neutral;
    if (pageIndex < 0) {
        return;
    }
    if (m_lastPageIndex < 0 || m_lastNavigationMsec < 0) {
        m_lastPageIndex = pageIndex;
        m_lastNavigationMsec = monotonicMsec;
        return;
    }
    if (pageIndex == m_lastPageIndex) {
        return;
    }

    const qint64 elapsedMsec = monotonicMsec - m_lastNavigationMsec;
    const MomentumDirection direction
        = pageIndex > m_lastPageIndex ? MomentumDirection::Next : MomentumDirection::Previous;
    if (direction == m_lastMomentumDirection && elapsedMsec <= predecodeBiasedNavigationMsec) {
        ++m_sameDirectionMoveCount;
    } else {
        m_sameDirectionMoveCount = 1;
    }

    m_momentumMode = momentumMode(rustPredecodeMomentumMode(RustPredecodeMomentumInput {
        m_lastPageIndex,
        pageIndex,
        elapsedMsec,
        m_sameDirectionMoveCount,
    }));
    m_lastMomentumDirection = direction;
    m_lastPageIndex = pageIndex;
    m_lastNavigationMsec = monotonicMsec;
}

qint64 ImagePredecodeCoordinator::currentMonotonicMsec() const
{
    return m_monotonicClock.isValid() ? m_monotonicClock.elapsed() : 0;
}

void ImagePredecodeCoordinator::cancel()
{
    cancelBackgroundWork();
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
