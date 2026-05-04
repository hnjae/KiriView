// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "decodedimageresult.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::DecodedImageResult;
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::normalizedImageUrl;
using KiriView::predecodeWindowImageUrls;
}

namespace KiriView {
ImagePredecodeCoordinator::ImagePredecodeCoordinator(
    QObject *parent, const ImageAsyncDependencies &dependencies)
    : QObject(parent)
    , m_decodeJob(this, dependencies.imageDataLoader, dependencies.imageDataDecoder)
    , m_candidateRepository(dependencies.candidateProvider)
{
    m_decodeJob.setDecodedCallback(
        [this](ImageDecodeRequest request, std::shared_ptr<DecodedImageResult> result) {
            finishPredecodeImageDecode(request, *result);
        });
    m_decodeJob.setLoadErrorCallback([this](const ImageDecodeRequest &request, const QString &) {
        finishPredecodeImageLoadError(request);
    });
}

void ImagePredecodeCoordinator::schedule(Context context)
{
    cancel();
    if (context.displayedImageLocation.isEmpty() || context.displayedImageSource == nullptr
        || context.displayedImagePreview.isNull()) {
        return;
    }

    const quint64 generation = m_generation.next();
    m_firstDisplayContext = context.firstDisplayContext;
    scheduleAdjacentImagePredecode(context, generation);
}

void ImagePredecodeCoordinator::scheduleAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const std::optional<ImageCandidateListContext> candidateContext
        = imageCandidateListContextForDisplayedImage(context.displayedImageLocation);
    if (!candidateContext.has_value()) {
        startPredecodeImageLoads(
            {}, context.displayedImageLocation.archiveDocument(), context, generation);
        return;
    }

    m_listerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, context, generation, candidateContext](
            std::vector<ImageNavigationCandidate> candidates) {
            startPredecodeImageLoads(
                predecodeWindowImageUrls(candidates, candidateContext->currentUrl),
                candidateContext->archiveDocument, context, generation);
        },
        [this, context, generation, archiveDocument = candidateContext->archiveDocument](
            const QString &) {
            startPredecodeImageLoads({}, archiveDocument, context, generation);
        });
}

void ImagePredecodeCoordinator::startPredecodeImageLoads(const std::vector<QUrl> &urls,
    const ArchiveDocumentLocation &archiveDocument, const Context &context, quint64 generation)
{
    if (!m_generation.accepts(generation)) {
        return;
    }

    m_cache.setWindowUrls(urls);
    m_cache.cacheDisplayedImage(context.displayedImageIsCacheable,
        context.displayedImageLocation.imageUrl(), context.displayedImageLocation.archiveDocument(),
        context.displayedImageSource, context.displayedImagePreview,
        context.displayedImageDisplayHints);
    m_cache.enqueueMissingWindowLoads(
        context.displayedImageLocation.imageUrl(), archiveDocument, m_activePredecodeUrl);

    startNextPredecodeImageLoad(generation);
}

void ImagePredecodeCoordinator::startNextPredecodeImageLoad(quint64 generation)
{
    if (!m_generation.accepts(generation)) {
        return;
    }

    const std::optional<KiriView::PredecodeRequest> request
        = m_cache.takeNextRequest(m_activePredecodeUrl);
    if (!request.has_value()) {
        return;
    }

    startPredecodeImageLoad(request->url, request->archiveDocument, generation);
}

void ImagePredecodeCoordinator::startPredecodeImageLoad(
    const QUrl &url, const ArchiveDocumentLocation &archiveDocument, quint64 generation)
{
    if (!url.isValid() || url.isEmpty() || !m_activePredecodeUrl.isEmpty() || m_cache.hasImage(url)
        || !m_cache.windowContains(url)) {
        return;
    }

    const QUrl normalizedUrl = normalizedImageUrl(url);
    m_activePredecodeUrl = normalizedUrl;
    m_activePredecodeArchiveDocument = archiveDocument;
    m_decodeJob.start(
        ImageDecodeRequest { generation, url, archiveDocument, m_firstDisplayContext });
}

void ImagePredecodeCoordinator::finishPredecodeImageLoadError(const ImageDecodeRequest &request)
{
    if (!m_generation.accepts(request.id)
        || normalizedImageUrl(request.imageUrl) != m_activePredecodeUrl) {
        return;
    }

    m_activePredecodeUrl = QUrl();
    m_activePredecodeArchiveDocument = ArchiveDocumentLocation::none();
    startNextPredecodeImageLoad(request.id);
}

void ImagePredecodeCoordinator::finishPredecodeImageDecode(
    ImageDecodeRequest request, const DecodedImageResult &result)
{
    if (!m_generation.accepts(request.id)
        || normalizedImageUrl(request.imageUrl) != m_activePredecodeUrl) {
        return;
    }

    const auto *staticImage = std::get_if<StaticDecodedImage>(&result);
    if (staticImage != nullptr
        && decodedImageResultIsPredecodeCacheable(result, KiriView::PredecodeCache::byteBudget())) {
        m_cache.cacheImage(request.imageUrl, m_activePredecodeArchiveDocument, staticImage->source,
            staticImage->preview, staticImage->displayHints);
    }

    m_activePredecodeUrl = QUrl();
    m_activePredecodeArchiveDocument = ArchiveDocumentLocation::none();
    startNextPredecodeImageLoad(request.id);
}

void ImagePredecodeCoordinator::cancel()
{
    m_generation.invalidate();
    m_listerJob.cancel();
    m_decodeJob.cancel();
    m_activePredecodeUrl = QUrl();
    m_activePredecodeArchiveDocument = ArchiveDocumentLocation::none();
    m_firstDisplayContext = {};
    m_cache.clearQueuedLoads();
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
