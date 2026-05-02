// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "imagecontainer.h"
#include "imageiojobs.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"
#include "kiriimagedecoder.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::DecodedImageResult;
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::decodeImageData;
using KiriView::ImageDecodeJob;
using KiriView::ImageIoJob;
using KiriView::isUrlInsideArchiveRoot;
using KiriView::navigationSourceUrl;
using KiriView::normalizedImageUrl;
using KiriView::predecodeWindowImageUrls;

ImageIoJob startImageDataLoad(QObject *receiver, QUrl imageUrl,
    ImageDecodeJob::DataCallback callback, ImageDecodeJob::ErrorCallback errorCallback)
{
    return KiriView::startStoredImageDataLoad(
        receiver, std::move(imageUrl), std::move(callback), std::move(errorCallback));
}

DecodedImageResult decodeImageBytes(const QByteArray &data) { return decodeImageData(data); }
}

namespace KiriView {
ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent)
    : QObject(parent)
    , m_decodeJob(this, startImageDataLoad, decodeImageBytes)
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
    if (context.displayedImageLocation.isEmpty() || context.displayedImage.isNull()) {
        return;
    }

    const quint64 generation = m_generation;
    if (isUrlInsideArchiveRoot(context.displayedImageLocation.imageUrl,
            context.displayedImageLocation.comicBookRootUrl)) {
        scheduleComicBookAdjacentImagePredecode(context, generation);
        return;
    }

    scheduleFileAdjacentImagePredecode(context, generation);
}

void ImagePredecodeCoordinator::scheduleFileAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const QUrl currentUrl = navigationSourceUrl(context.displayedImageLocation.imageUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        startPredecodeImageLoads({}, QUrl(), context, generation);
        return;
    }

    m_listerJob = startDirectoryImageCandidateList(
        this, parentUrl,
        [this, context, currentUrl, generation](std::vector<ImageNavigationCandidate> candidates) {
            startPredecodeImageLoads(
                predecodeWindowImageUrls(candidates, currentUrl), QUrl(), context, generation);
        },
        [this, context, generation](
            const QString &) { startPredecodeImageLoads({}, QUrl(), context, generation); });
}

void ImagePredecodeCoordinator::scheduleComicBookAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const QUrl currentUrl
        = context.displayedImageLocation.imageUrl.adjusted(QUrl::NormalizePathSegments);
    const QUrl archiveRootUrl = context.displayedImageLocation.comicBookRootUrl;
    if (!currentUrl.isValid() || archiveRootUrl.isEmpty()) {
        startPredecodeImageLoads({}, archiveRootUrl, context, generation);
        return;
    }

    m_listJob = startArchiveImageCandidateList(
        this, archiveRootUrl,
        [this, context, generation, currentUrl, archiveRootUrl](
            std::vector<ImageNavigationCandidate> candidates) {
            startPredecodeImageLoads(predecodeWindowImageUrls(candidates, currentUrl),
                archiveRootUrl, context, generation);
        },
        [this, context, generation, archiveRootUrl](const QString &) {
            startPredecodeImageLoads({}, archiveRootUrl, context, generation);
        });
}

void ImagePredecodeCoordinator::startPredecodeImageLoads(const std::vector<QUrl> &urls,
    const QUrl &comicBookRootUrl, const Context &context, quint64 generation)
{
    if (generation != m_generation) {
        return;
    }

    m_cache.setWindowUrls(urls);
    m_cache.cacheDisplayedImage(context.displayedImageIsCacheable,
        context.displayedImageLocation.imageUrl, context.displayedImageLocation.comicBookRootUrl,
        context.displayedImage);
    m_cache.enqueueMissingWindowLoads(
        context.displayedImageLocation.imageUrl, comicBookRootUrl, m_activePredecodeUrl);

    startNextPredecodeImageLoad(generation);
}

void ImagePredecodeCoordinator::startNextPredecodeImageLoad(quint64 generation)
{
    if (generation != m_generation) {
        return;
    }

    const std::optional<KiriView::PredecodeRequest> request
        = m_cache.takeNextRequest(m_activePredecodeUrl);
    if (!request.has_value()) {
        return;
    }

    startPredecodeImageLoad(request->url, request->comicBookRootUrl, generation);
}

void ImagePredecodeCoordinator::startPredecodeImageLoad(
    const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation)
{
    if (!url.isValid() || url.isEmpty() || !m_activePredecodeUrl.isEmpty() || m_cache.hasImage(url)
        || !m_cache.windowContains(url)) {
        return;
    }

    const QUrl normalizedUrl = normalizedImageUrl(url);
    m_activePredecodeUrl = normalizedUrl;
    m_activePredecodeComicBookRootUrl = comicBookRootUrl;
    m_decodeJob.start(ImageDecodeRequest { generation, url });
}

void ImagePredecodeCoordinator::finishPredecodeImageLoadError(const ImageDecodeRequest &request)
{
    if (request.id != m_generation
        || normalizedImageUrl(request.imageUrl) != m_activePredecodeUrl) {
        return;
    }

    m_activePredecodeUrl = QUrl();
    m_activePredecodeComicBookRootUrl = QUrl();
    startNextPredecodeImageLoad(request.id);
}

void ImagePredecodeCoordinator::finishPredecodeImageDecode(
    ImageDecodeRequest request, const DecodedImageResult &result)
{
    if (request.id != m_generation
        || normalizedImageUrl(request.imageUrl) != m_activePredecodeUrl) {
        return;
    }

    if (decodedImageResultIsPredecodeCacheable(result, KiriView::PredecodeCache::byteBudget())) {
        m_cache.cacheImage(request.imageUrl, m_activePredecodeComicBookRootUrl, result.image);
    }

    m_activePredecodeUrl = QUrl();
    m_activePredecodeComicBookRootUrl = QUrl();
    startNextPredecodeImageLoad(request.id);
}

void ImagePredecodeCoordinator::cancel()
{
    ++m_generation;
    m_listerJob.cancel();
    m_listJob.cancel();
    m_decodeJob.cancel();
    m_activePredecodeUrl = QUrl();
    m_activePredecodeComicBookRootUrl = QUrl();
    m_cache.clearQueuedLoads();
}

void ImagePredecodeCoordinator::clear()
{
    cancel();
    m_cache.clear();
}

std::optional<PredecodedImage> ImagePredecodeCoordinator::tryTake(const QUrl &url) const
{
    QImage image;
    QUrl comicBookRootUrl;
    if (!m_cache.findImage(url, &image, &comicBookRootUrl)) {
        return std::nullopt;
    }

    return PredecodedImage { image, DisplayedImageLocation { url, comicBookRootUrl } };
}
}
