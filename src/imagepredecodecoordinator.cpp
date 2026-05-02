// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "imageiojobs.h"
#include "kiriimagedecoder.h"
#include "kiriimagenavigation.h"

#include <QByteArray>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <Qt>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using KiriView::DecodedImageResult;
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::decodeImageData;
using KiriView::isUrlInsideArchiveRoot;
using KiriView::navigationSourceUrl;
using KiriView::normalizedImageUrl;
using KiriView::predecodeWindowImageUrls;
}

namespace KiriView {
ImagePredecodeCoordinator::ImagePredecodeCoordinator(QObject *parent)
    : QObject(parent)
{
}

void ImagePredecodeCoordinator::schedule(Context context)
{
    cancel();
    if (context.displayedUrl.isEmpty() || context.displayedImage.isNull()) {
        return;
    }

    const quint64 generation = m_generation;
    if (isUrlInsideArchiveRoot(context.displayedUrl, context.comicBookRootUrl)) {
        scheduleComicBookAdjacentImagePredecode(context, generation);
        return;
    }

    scheduleFileAdjacentImagePredecode(context, generation);
}

void ImagePredecodeCoordinator::scheduleFileAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const QUrl currentUrl = navigationSourceUrl(context.displayedUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        startPredecodeImageLoads({}, QUrl(), context, generation);
        return;
    }

    startDirectoryImageCandidateList(
        this, &m_listerSlot, parentUrl,
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
    const QUrl currentUrl = context.displayedUrl.adjusted(QUrl::NormalizePathSegments);
    const QUrl archiveRootUrl = context.comicBookRootUrl;
    if (!currentUrl.isValid() || archiveRootUrl.isEmpty()) {
        startPredecodeImageLoads({}, archiveRootUrl, context, generation);
        return;
    }

    startArchiveImageCandidateList(
        this, &m_listJobSlot, archiveRootUrl,
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
    m_cache.cacheDisplayedImage(context.displayedImageIsCacheable, context.displayedUrl,
        context.comicBookRootUrl, context.displayedImage);
    m_cache.enqueueMissingWindowLoads(context.displayedUrl, comicBookRootUrl, m_activePredecodeUrl);

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
    startStoredImageDataLoad(
        this, &m_imageLoadSlot, url,
        [this, generation, url, comicBookRootUrl, normalizedUrl](QByteArray data) {
            if (generation != m_generation || normalizedUrl != m_activePredecodeUrl) {
                return;
            }

            startPredecodeImageDecode(std::move(data), url, comicBookRootUrl, generation);
        },
        [this, generation, normalizedUrl](const QString &) {
            if (generation != m_generation || normalizedUrl != m_activePredecodeUrl) {
                return;
            }

            m_activePredecodeUrl = QUrl();
            startNextPredecodeImageLoad(generation);
        });
}

void ImagePredecodeCoordinator::startPredecodeImageDecode(
    QByteArray data, const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation)
{
    const QPointer<ImagePredecodeCoordinator> coordinator(this);
    QThreadPool::globalInstance()->start(QRunnable::create(
        [coordinator, data = std::move(data), url, comicBookRootUrl, generation]() mutable {
            auto result = std::make_shared<DecodedImageResult>(decodeImageData(data));
            if (coordinator == nullptr) {
                return;
            }

            QMetaObject::invokeMethod(
                coordinator.data(),
                [coordinator, generation, url, comicBookRootUrl, result]() {
                    if (coordinator == nullptr || generation != coordinator->m_generation) {
                        return;
                    }

                    const QUrl normalizedUrl = normalizedImageUrl(url);
                    if (normalizedUrl != coordinator->m_activePredecodeUrl) {
                        return;
                    }

                    if (decodedImageResultIsPredecodeCacheable(
                            *result, KiriView::PredecodeCache::byteBudget())) {
                        coordinator->m_cache.cacheImage(url, comicBookRootUrl, result->image);
                    }

                    coordinator->m_activePredecodeUrl = QUrl();
                    coordinator->startNextPredecodeImageLoad(generation);
                },
                Qt::QueuedConnection);
        }));
}

void ImagePredecodeCoordinator::cancel()
{
    ++m_generation;
    m_listerSlot.cancel();
    m_listJobSlot.cancel();
    m_imageLoadSlot.cancel();
    m_activePredecodeUrl = QUrl();
    m_cache.clearQueuedLoads();
}

void ImagePredecodeCoordinator::clear()
{
    cancel();
    m_cache.clear();
}

bool ImagePredecodeCoordinator::tryTake(
    const QUrl &url, QImage *image, QUrl *comicBookRootUrl) const
{
    return m_cache.findImage(url, image, comicBookRootUrl);
}
}
