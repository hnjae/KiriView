// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "decodedimageresult.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"

#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::DecodedImageResult;
using KiriView::ImageCandidateListContext;
using KiriView::normalizedImageUrl;
using KiriView::predecodeWindowImageUrls;
}

namespace KiriView {
ImagePredecodeCoordinator::ImagePredecodeCoordinator(
    QObject *parent, const ImageAsyncDependencies &dependencies)
    : QObject(parent)
    , m_decodeJob(this, dependencies.imageDecode,
          ImageDecodeJob::Callbacks {
              [this](ImageDecodeRequest request, DecodedImageResult result) {
                  finishPredecodeImageDecode(request, result);
              },
              [this](const ImageDecodeRequest &request, const QString &) {
                  finishPredecodeImageLoadError(request);
              },
          })
    , m_candidateRepository(dependencies.candidateProvider)
{
}

void ImagePredecodeCoordinator::schedule(Context context)
{
    cancel();
    if (context.displayedImageLocation.isEmpty() || !context.displayedImage.isValid()) {
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

    const QUrl currentUrl = candidateContext->currentUrl();
    const ArchiveDocumentLocation archiveDocument = candidateContext->archiveDocument();
    m_listerJob = m_candidateRepository.loadImages(
        this, *candidateContext,
        [this, context, generation, currentUrl, archiveDocument](
            std::vector<ImageNavigationCandidate> candidates) {
            startPredecodeImageLoads(predecodeWindowImageUrls(candidates, currentUrl),
                archiveDocument, context, generation);
        },
        [this, context, generation, archiveDocument](const QString &) {
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
        context.displayedImage);
    m_cache.enqueueMissingWindowLoads(
        context.displayedImageLocation.imageUrl(), archiveDocument, activePredecodeUrl());

    startNextPredecodeImageLoad(generation);
}

void ImagePredecodeCoordinator::startNextPredecodeImageLoad(quint64 generation)
{
    if (!m_generation.accepts(generation)) {
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
    if (!url.isValid() || url.isEmpty() || hasActivePredecodeRequest() || m_cache.hasImage(url)
        || !m_cache.windowContains(url)) {
        return;
    }

    const QUrl normalizedUrl = normalizedImageUrl(url);
    m_activePredecodeRequest = ActivePredecodeRequest { normalizedUrl, archiveDocument };
    m_decodeJob.start(ImageDecodeRequest::fromLocation(
        generation, DisplayedImageLocation::fromUrl(url, archiveDocument), m_firstDisplayContext));
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
            request.imageUrl(), activeRequest->archiveDocument, staticImage->staticImage);
    }

    startNextPredecodeImageLoad(request.id());
}

QUrl ImagePredecodeCoordinator::activePredecodeUrl() const
{
    return m_activePredecodeRequest.has_value() ? m_activePredecodeRequest->url : QUrl();
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
    return m_activePredecodeRequest.has_value() && m_generation.accepts(request.id())
        && normalizedImageUrl(request.imageUrl()) == m_activePredecodeRequest->url;
}

void ImagePredecodeCoordinator::clearActivePredecodeRequest() { m_activePredecodeRequest.reset(); }

void ImagePredecodeCoordinator::cancel()
{
    m_generation.invalidate();
    m_listerJob.cancel();
    m_decodeJob.cancel();
    clearActivePredecodeRequest();
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
