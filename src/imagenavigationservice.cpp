// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagenavigationservice.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <KIO/ListJob>
#include <KIO/UDSEntry>
#include <KJob>
#include <memory>
#include <utility>

namespace {
using KiriView::adjacentContainerNavigationCandidate;
using KiriView::adjacentImageNavigationUrl;
using KiriView::appendArchiveImageNavigationCandidates;
using KiriView::comicBookArchiveRootUrl;
using KiriView::ContainerNavigationCandidate;
using KiriView::containerNavigationCandidates;
using KiriView::ContainerNavigationCandidateType;
using KiriView::ImageNavigationCandidate;
using KiriView::imageNavigationCandidates;
using KiriView::imageNavigationCandidateUrls;
using KiriView::isUrlInsideArchiveRoot;
using KiriView::NavigationDirection;
using KiriView::navigationSourceUrl;
using KiriView::pageNavigationStateForUrls;
using KiriView::parentUrlForContainerNavigation;
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
ImageNavigationService::ImageNavigationService(QObject *parent)
    : QObject(parent)
{
}

void ImageNavigationService::setOpenUrlCallback(OpenUrlCallback callback)
{
    m_openUrl = std::move(callback);
}

void ImageNavigationService::setOpenContainerImageCallback(OpenContainerImageCallback callback)
{
    m_openContainerImage = std::move(callback);
}

void ImageNavigationService::setContainerNavigationErrorCallback(
    ContainerNavigationErrorCallback callback)
{
    m_containerNavigationError = std::move(callback);
}

void ImageNavigationService::setPageNavigationChangedCallback(
    PageNavigationChangedCallback callback)
{
    m_pageNavigationChanged = std::move(callback);
}

int ImageNavigationService::currentPageNumber() const
{
    return m_currentPageIndex < 0 ? 0 : m_currentPageIndex + 1;
}

int ImageNavigationService::imageCount() const
{
    return static_cast<int>(m_pageNavigationUrls.size());
}

std::optional<QUrl> ImageNavigationService::urlAtPage(int pageNumber) const
{
    if (pageNumber <= 0 || pageNumber > imageCount()) {
        return std::nullopt;
    }

    const int pageIndex = pageNumber - 1;
    if (pageIndex == m_currentPageIndex) {
        return std::nullopt;
    }

    return m_pageNavigationUrls.at(static_cast<std::size_t>(pageIndex));
}

void ImageNavigationService::openAdjacentImage(
    const DisplayContext &context, NavigationDirection direction)
{
    if (!context.hasDisplayedImage || context.displayedUrl.isEmpty()) {
        return;
    }

    cancelContainerNavigation();

    if (isUrlInsideArchiveRoot(context.displayedUrl, context.comicBookRootUrl)) {
        openAdjacentComicBookImage(context, direction);
        return;
    }

    const QUrl currentUrl = navigationSourceUrl(context.displayedUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    cancelNavigation();

    auto *lister = createImageCandidateLister(this);
    const quint64 token = m_navigationListerSlot.start(lister, cancelDirLister);

    connect(
        lister, &KCoreDirLister::completed, this, [this, lister, token, direction, currentUrl]() {
            finishNavigation(lister, token, direction, currentUrl);
        });
    connect(lister, &KCoreDirLister::jobError, this,
        [this, lister, token](KIO::Job *) { finishNavigationWithError(lister, token); });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        finishNavigationWithError(lister, token);
    }
}

void ImageNavigationService::openAdjacentComicBookImage(
    const DisplayContext &context, NavigationDirection direction)
{
    const QUrl currentUrl = context.displayedUrl.adjusted(QUrl::NormalizePathSegments);
    const QUrl archiveRootUrl = context.comicBookRootUrl;
    if (!currentUrl.isValid() || archiveRootUrl.isEmpty()) {
        return;
    }

    cancelNavigation();

    auto *job
        = KIO::listRecursive(archiveRootUrl, KIO::HideProgressInfo, recursiveImageListFlags());
    auto candidates = collectArchiveImageCandidates(job, this, archiveRootUrl);
    const quint64 token = m_navigationListSlot.start(job, cancelKJob);

    connect(job, &KJob::result, this,
        [this, job, token, direction, currentUrl, candidates](KJob *finishedJob) {
            if (!m_navigationListSlot.claim(token, job)) {
                return;
            }

            if (finishedJob->error() != KJob::NoError) {
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            const std::optional<QUrl> targetUrl
                = adjacentImageNavigationUrl(*candidates, currentUrl, direction);
            if (!targetUrl.has_value()) {
                return;
            }

            if (m_openUrl) {
                m_openUrl(*targetUrl);
            }
        });

    connect(job, &QObject::destroyed, this, [this, job]() { m_navigationListSlot.clear(job); });
}

void ImageNavigationService::cancelNavigation()
{
    m_navigationListerSlot.cancel();
    m_navigationListSlot.cancel();
}

void ImageNavigationService::finishNavigation(KCoreDirLister *lister, quint64 generation,
    NavigationDirection direction, const QUrl &currentUrl)
{
    if (!m_navigationListerSlot.claim(generation, lister)) {
        return;
    }

    const std::vector<ImageNavigationCandidate> candidates = imageCandidatesFromLister(lister);
    lister->deleteLater();

    const std::optional<QUrl> targetUrl
        = adjacentImageNavigationUrl(candidates, currentUrl, direction);
    if (!targetUrl.has_value()) {
        return;
    }

    if (m_openUrl) {
        m_openUrl(*targetUrl);
    }
}

void ImageNavigationService::finishNavigationWithError(KCoreDirLister *lister, quint64 generation)
{
    if (!m_navigationListerSlot.claim(generation, lister)) {
        return;
    }

    lister->deleteLater();
}

void ImageNavigationService::openAdjacentContainer(
    const QUrl &currentContainerUrl, NavigationDirection direction)
{
    if (currentContainerUrl.isEmpty()) {
        return;
    }

    const QUrl parentUrl = parentUrlForContainerNavigation(currentContainerUrl);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    cancelNavigation();
    cancelContainerNavigation();

    auto *lister = createImageCandidateLister(this);
    const quint64 token = m_containerNavigationListerSlot.start(lister, cancelDirLister);

    connect(lister, &KCoreDirLister::completed, this,
        [this, lister, token, direction, currentContainerUrl]() {
            finishContainerNavigation(lister, token, direction, currentContainerUrl);
        });
    connect(lister, &KCoreDirLister::jobError, this,
        [this, lister, token](KIO::Job *) { finishContainerNavigationWithError(lister, token); });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        finishContainerNavigationWithError(lister, token);
    }
}

void ImageNavigationService::cancelContainerNavigation()
{
    m_containerNavigationListerSlot.cancel();
    m_containerNavigationListSlot.cancel();
}

void ImageNavigationService::finishContainerNavigation(KCoreDirLister *lister, quint64 generation,
    NavigationDirection direction, const QUrl &currentContainerUrl)
{
    if (!m_containerNavigationListerSlot.claim(generation, lister)) {
        return;
    }

    const std::vector<ContainerNavigationCandidate> candidates
        = containerNavigationCandidates(lister->items(KCoreDirLister::AllItems));

    lister->deleteLater();

    const auto target
        = adjacentContainerNavigationCandidate(candidates, currentContainerUrl, direction);
    if (!target.has_value()) {
        return;
    }

    if (target->type == ContainerNavigationCandidateType::Directory) {
        openDirectoryContainer(target->url);
    } else {
        openComicBookContainer(target->url);
    }
}

void ImageNavigationService::finishContainerNavigationWithError(
    KCoreDirLister *lister, quint64 generation)
{
    if (!m_containerNavigationListerSlot.claim(generation, lister)) {
        return;
    }

    lister->deleteLater();
}

void ImageNavigationService::openDirectoryContainer(const QUrl &containerUrl)
{
    auto *lister = createImageCandidateLister(this);
    const quint64 token = m_containerNavigationListerSlot.start(lister, cancelDirLister);

    connect(lister, &KCoreDirLister::completed, this, [this, lister, token, containerUrl]() {
        finishDirectoryContainerNavigation(lister, token, containerUrl);
    });
    connect(lister, &KCoreDirLister::jobError, this,
        [this, lister, token, containerUrl](KIO::Job *job) {
            const QString errorString = job == nullptr ? QString() : job->errorString();
            finishDirectoryContainerNavigationWithError(lister, token, containerUrl, errorString);
        });

    if (!lister->openUrl(containerUrl, KCoreDirLister::Reload)) {
        finishDirectoryContainerNavigationWithError(lister, token, containerUrl, QString());
    }
}

void ImageNavigationService::finishDirectoryContainerNavigation(
    KCoreDirLister *lister, quint64 generation, const QUrl &containerUrl)
{
    if (!m_containerNavigationListerSlot.claim(generation, lister)) {
        return;
    }

    const std::vector<ImageNavigationCandidate> candidates = imageCandidatesFromLister(lister);
    lister->deleteLater();

    if (candidates.empty()) {
        finishContainerNavigationWithEmptyContainer(containerUrl);
        return;
    }

    openImageFromContainerNavigation(candidates.front().url, containerUrl);
}

void ImageNavigationService::finishDirectoryContainerNavigationWithError(KCoreDirLister *lister,
    quint64 generation, const QUrl &containerUrl, const QString &errorString)
{
    if (!m_containerNavigationListerSlot.claim(generation, lister)) {
        return;
    }

    lister->deleteLater();
    finishContainerNavigationLoadWithError(
        containerUrl, ContainerNavigationError::Generic, errorString);
}

void ImageNavigationService::openComicBookContainer(const QUrl &containerUrl)
{
    const std::optional<QUrl> archiveRootUrl = comicBookArchiveRootUrl(containerUrl);
    if (!archiveRootUrl.has_value()) {
        finishContainerNavigationLoadWithError(
            containerUrl, ContainerNavigationError::InvalidComicBookArchive, QString());
        return;
    }

    const QUrl archiveRoot = archiveRootUrl.value();
    auto *job = KIO::listRecursive(archiveRoot, KIO::HideProgressInfo, recursiveImageListFlags());
    auto candidates = collectArchiveImageCandidates(job, this, archiveRoot);
    const quint64 token = m_containerNavigationListSlot.start(job, cancelKJob);

    connect(
        job, &KJob::result, this, [this, job, token, containerUrl, candidates](KJob *finishedJob) {
            if (!m_containerNavigationListSlot.claim(token, job)) {
                return;
            }

            if (finishedJob->error() != KJob::NoError) {
                finishContainerNavigationLoadWithError(
                    containerUrl, ContainerNavigationError::Generic, finishedJob->errorString());
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            if (candidates->empty()) {
                finishContainerNavigationWithEmptyContainer(containerUrl);
                return;
            }

            openImageFromContainerNavigation(candidates->front().url, containerUrl);
        });

    connect(job, &QObject::destroyed, this,
        [this, job]() { m_containerNavigationListSlot.clear(job); });
}

void ImageNavigationService::openImageFromContainerNavigation(
    const QUrl &imageUrl, const QUrl &containerUrl)
{
    if (m_openContainerImage) {
        m_openContainerImage(imageUrl, containerUrl);
    }
}

void ImageNavigationService::finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl)
{
    finishContainerNavigationLoadWithError(
        containerUrl, ContainerNavigationError::EmptyContainer, QString());
}

void ImageNavigationService::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString)
{
    if (m_containerNavigationError) {
        m_containerNavigationError(containerUrl, error, errorString);
    }
}

void ImageNavigationService::updatePageNavigation(const DisplayContext &context)
{
    cancelPageNavigationUpdate();

    if (!context.hasDisplayedImage || context.displayedUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    const bool isComicBookPage
        = isUrlInsideArchiveRoot(context.displayedUrl, context.comicBookRootUrl);
    const QUrl currentUrl = isComicBookPage
        ? context.displayedUrl.adjusted(QUrl::NormalizePathSegments)
        : navigationSourceUrl(context.displayedUrl);
    if (!currentUrl.isValid() || currentUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    setFallbackPageNavigationUrl(currentUrl);

    if (isComicBookPage) {
        updateComicBookPageNavigation(currentUrl, context.comicBookRootUrl);
        return;
    }

    updateFilePageNavigation(currentUrl);
}

void ImageNavigationService::updateFilePageNavigation(const QUrl &currentUrl)
{
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    auto *lister = createImageCandidateLister(this);
    const quint64 token = m_pageNavigationListerSlot.start(lister, cancelDirLister);

    connect(lister, &KCoreDirLister::completed, this, [this, lister, token, currentUrl]() {
        if (!m_pageNavigationListerSlot.claim(token, lister)) {
            return;
        }

        const std::vector<ImageNavigationCandidate> candidates = imageCandidatesFromLister(lister);
        lister->deleteLater();

        setPageNavigationUrls(imageNavigationCandidateUrls(candidates), currentUrl);
    });
    connect(lister, &KCoreDirLister::jobError, this,
        [this, lister, token](KIO::Job *) { finishPageNavigationUpdateWithError(lister, token); });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        finishPageNavigationUpdateWithError(lister, token);
    }
}

void ImageNavigationService::updateComicBookPageNavigation(
    const QUrl &currentUrl, const QUrl &archiveRootUrl)
{
    if (archiveRootUrl.isEmpty()) {
        return;
    }

    auto *job
        = KIO::listRecursive(archiveRootUrl, KIO::HideProgressInfo, recursiveImageListFlags());
    auto candidates = collectArchiveImageCandidates(job, this, archiveRootUrl);
    const quint64 token = m_pageNavigationListSlot.start(job, cancelKJob);

    connect(
        job, &KJob::result, this, [this, job, token, currentUrl, candidates](KJob *finishedJob) {
            if (!m_pageNavigationListSlot.claim(token, job)) {
                return;
            }

            if (finishedJob->error() != KJob::NoError) {
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            setPageNavigationUrls(imageNavigationCandidateUrls(*candidates), currentUrl);
        });

    connect(job, &QObject::destroyed, this, [this, job]() { m_pageNavigationListSlot.clear(job); });
}

void ImageNavigationService::cancelPageNavigationUpdate()
{
    m_pageNavigationListerSlot.cancel();
    m_pageNavigationListSlot.cancel();
}

void ImageNavigationService::finishPageNavigationUpdateWithError(
    KCoreDirLister *lister, quint64 generation)
{
    if (!m_pageNavigationListerSlot.claim(generation, lister)) {
        return;
    }

    lister->deleteLater();
}

void ImageNavigationService::setPageNavigationUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    auto state = pageNavigationStateForUrls(std::move(urls), currentUrl);

    if (m_pageNavigationUrls == state.urls && m_currentPageIndex == state.currentIndex) {
        return;
    }

    m_pageNavigationUrls = std::move(state.urls);
    m_currentPageIndex = state.currentIndex;
    if (m_pageNavigationChanged) {
        m_pageNavigationChanged();
    }
}

void ImageNavigationService::setFallbackPageNavigationUrl(const QUrl &currentUrl)
{
    if (!currentUrl.isValid() || currentUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    setPageNavigationUrls({ currentUrl.adjusted(QUrl::NormalizePathSegments) }, currentUrl);
}

void ImageNavigationService::clearPageNavigation()
{
    if (m_pageNavigationUrls.empty() && m_currentPageIndex == -1) {
        return;
    }

    m_pageNavigationUrls.clear();
    m_currentPageIndex = -1;
    if (m_pageNavigationChanged) {
        m_pageNavigationChanged();
    }
}
}
