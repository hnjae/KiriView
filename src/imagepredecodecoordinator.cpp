// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagepredecodecoordinator.h"

#include "kiriimagedecoder.h"
#include "kiriimagenavigation.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <KIO/ListJob>
#include <KIO/StoredTransferJob>
#include <KIO/UDSEntry>
#include <KJob>
#include <QByteArray>
#include <QMetaObject>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <Qt>
#include <memory>
#include <optional>
#include <utility>

namespace {
using KiriView::appendArchiveImageNavigationCandidates;
using KiriView::DecodedImageResult;
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::decodeImageData;
using KiriView::ImageNavigationCandidate;
using KiriView::imageNavigationCandidates;
using KiriView::isUrlInsideArchiveRoot;
using KiriView::navigationSourceUrl;
using KiriView::normalizedImageUrl;
using KiriView::predecodeWindowImageUrls;
using KiriView::sortImageNavigationCandidates;

KIO::ListJob::ListFlags recursiveImageListFlags()
{
    return KIO::ListJob::ListFlags(KIO::ListJob::ListFlag::IncludeHidden);
}

void cancelKJob(QObject *object)
{
    auto *job = qobject_cast<KJob *>(object);
    if (job == nullptr) {
        return;
    }

    job->kill(KJob::Quietly);
}

void cancelDirLister(QObject *object)
{
    auto *lister = qobject_cast<KCoreDirLister *>(object);
    if (lister == nullptr) {
        return;
    }

    lister->stop();
    lister->deleteLater();
}

KCoreDirLister *createImageCandidateLister(QObject *parent)
{
    auto *lister = new KCoreDirLister(parent);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    return lister;
}

std::vector<ImageNavigationCandidate> imageCandidatesFromLister(KCoreDirLister *lister)
{
    return imageNavigationCandidates(lister->items(KCoreDirLister::AllItems));
}

std::shared_ptr<std::vector<ImageNavigationCandidate>> collectArchiveImageCandidates(
    KIO::ListJob *job, QObject *receiver, const QUrl &archiveRootUrl)
{
    auto candidates = std::make_shared<std::vector<ImageNavigationCandidate>>();
    QObject::connect(job, &KIO::ListJob::entries, receiver,
        [archiveRootUrl, candidates](KIO::Job *entriesJob, const KIO::UDSEntryList &entries) {
            auto *listJob = qobject_cast<KIO::ListJob *>(entriesJob);
            const QUrl directoryUrl = listJob == nullptr ? archiveRootUrl : listJob->url();
            appendArchiveImageNavigationCandidates(
                candidates.get(), entries, directoryUrl, archiveRootUrl);
        });
    return candidates;
}
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

    auto *lister = createImageCandidateLister(this);
    const quint64 token = m_listerSlot.start(lister, cancelDirLister);

    connect(lister, &KCoreDirLister::completed, this,
        [this, lister, token, context, currentUrl, generation]() {
            if (!m_listerSlot.claim(token, lister)) {
                return;
            }

            const std::vector<ImageNavigationCandidate> candidates
                = imageCandidatesFromLister(lister);
            lister->deleteLater();
            startPredecodeImageLoads(
                predecodeWindowImageUrls(candidates, currentUrl), QUrl(), context, generation);
        });
    connect(lister, &KCoreDirLister::jobError, this,
        [this, lister, token, context, generation](KIO::Job *) {
            if (!m_listerSlot.claim(token, lister)) {
                return;
            }

            lister->deleteLater();
            startPredecodeImageLoads({}, QUrl(), context, generation);
        });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        if (m_listerSlot.claim(token, lister)) {
            lister->deleteLater();
            startPredecodeImageLoads({}, QUrl(), context, generation);
        }
    }
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

    auto *job
        = KIO::listRecursive(archiveRootUrl, KIO::HideProgressInfo, recursiveImageListFlags());
    auto candidates = collectArchiveImageCandidates(job, this, archiveRootUrl);
    const quint64 token = m_listJobSlot.start(job, cancelKJob);

    connect(job, &KJob::result, this,
        [this, job, token, context, generation, currentUrl, archiveRootUrl, candidates](
            KJob *finishedJob) {
            if (!m_listJobSlot.claim(token, job)) {
                return;
            }

            if (finishedJob->error() != KJob::NoError) {
                startPredecodeImageLoads({}, archiveRootUrl, context, generation);
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            startPredecodeImageLoads(predecodeWindowImageUrls(*candidates, currentUrl),
                archiveRootUrl, context, generation);
        });

    connect(job, &QObject::destroyed, this, [this, job]() { m_listJobSlot.clear(job); });
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

    auto *job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    const QUrl normalizedUrl = normalizedImageUrl(url);
    const quint64 token = m_imageLoadSlot.start(job, cancelKJob);
    m_activePredecodeUrl = normalizedUrl;

    connect(job, &KJob::result, this,
        [this, job, token, generation, url, comicBookRootUrl, normalizedUrl](KJob *finishedJob) {
            if (!m_imageLoadSlot.claim(token, job) || generation != m_generation
                || normalizedUrl != m_activePredecodeUrl) {
                return;
            }

            if (finishedJob->error() != KJob::NoError) {
                m_activePredecodeUrl = QUrl();
                startNextPredecodeImageLoad(generation);
                return;
            }

            startPredecodeImageDecode(job->data(), url, comicBookRootUrl, generation);
        });

    connect(job, &QObject::destroyed, this, [this, job, token, generation, normalizedUrl]() {
        if (!m_imageLoadSlot.accepts(token, job)) {
            return;
        }

        m_imageLoadSlot.clear(job);
        if (generation == m_generation && normalizedUrl == m_activePredecodeUrl) {
            m_activePredecodeUrl = QUrl();
            startNextPredecodeImageLoad(generation);
        }
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
