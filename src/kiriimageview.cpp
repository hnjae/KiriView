// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimageview.h"

#include "kiriimagedecoder.h"
#include "kiriimagenavigation.h"
#include "kiriimagerendernode.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <KIO/ListJob>
#include <KIO/StoredTransferJob>
#include <KIO/UDSEntry>
#include <KJob>
#include <QBuffer>
#include <QByteArray>
#include <QIODevice>
#include <QImageReader>
#include <QMetaObject>
#include <QPointer>
#include <QQuickWindow>
#include <QRunnable>
#include <QThreadPool>
#include <Qt>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <memory>
#include <optional>
#include <rhi/qrhi.h>
#include <utility>

namespace {
constexpr int defaultAnimationFrameDelay = 100;
constexpr int minimumAnimationFrameDelay = 10;
constexpr qreal minimumManualZoomPercent = 10.0;
constexpr qreal maximumManualZoomPercent = 800.0;
constexpr qsizetype predecodeCacheByteBudget = 512 * 1024 * 1024;

using KiriView::appendArchiveImageNavigationCandidates;
using KiriView::comicBookArchiveRootUrl;
using KiriView::ContainerNavigationCandidate;
using KiriView::containerNavigationCandidates;
using KiriView::ContainerNavigationCandidateType;
using KiriView::containerNavigationUrlForImage;
using KiriView::containingComicBookArchiveRootUrl;
using KiriView::DecodedImageResult;
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::decodeImageData;
using KiriView::displayReadyImage;
using KiriView::imageByteCost;
using KiriView::ImageNavigationCandidate;
using KiriView::imageNavigationCandidates;
using KiriView::imageNavigationCandidateUrls;
using KiriView::isUrlInsideArchiveRoot;
using KiriView::navigationSourceUrl;
using KiriView::normalizedImageUrl;
using KiriView::parentUrlForContainerNavigation;
using KiriView::predecodeWindowImageUrls;
using KiriView::renderSvgImage;
using KiriView::sameContainerNavigationUrl;
using KiriView::sortImageNavigationCandidates;
using KiriView::svgRasterSize;
using KiriView::windowTitleFileNameForDisplayedUrl;

int normalizedAnimationFrameDelay(int delay)
{
    if (delay < 0) {
        return defaultAnimationFrameDelay;
    }

    return std::max(delay, minimumAnimationFrameDelay);
}

bool approximatelyEqual(qreal left, qreal right) { return std::abs(left - right) < 0.001; }

bool approximatelyEqual(const QSizeF &left, const QSizeF &right)
{
    return approximatelyEqual(left.width(), right.width())
        && approximatelyEqual(left.height(), right.height());
}
}

KiriImageView::KiriImageView(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    m_animationTimer.setSingleShot(true);
    connect(&m_animationTimer, &QTimer::timeout, this, &KiriImageView::advanceAnimationFrame);
}

KiriImageView::~KiriImageView()
{
    stopAnimation();
    cancelPredecode();
    cancelPageNavigationUpdate();
    cancelContainerNavigation();
    cancelNavigation();
    cancelLoad();
}

QUrl KiriImageView::sourceUrl() const { return m_sourceUrl; }

void KiriImageView::setSourceUrl(const QUrl &sourceUrl) { setSourceUrlForLoad(sourceUrl, QUrl()); }

void KiriImageView::setSourceUrlForLoad(const QUrl &sourceUrl, const QUrl &containerNavigationUrl)
{
    if (m_sourceUrl == sourceUrl) {
        m_loadingContainerNavigationUrl = QUrl();
        if (!containerNavigationUrl.isEmpty()) {
            setContainerNavigationUrl(containerNavigationUrl);
        }
        return;
    }

    cancelNavigation();
    cancelContainerNavigation();
    m_loadingContainerNavigationUrl = containerNavigationUrl;
    m_sourceUrl = sourceUrl;
    Q_EMIT sourceUrlChanged();
    startLoad();
}

KiriImageView::Status KiriImageView::status() const { return m_status; }

bool KiriImageView::loading() const { return m_loading; }

QString KiriImageView::errorString() const { return m_errorString; }

QString KiriImageView::windowTitleFileName() const { return m_windowTitleFileName; }

QSize KiriImageView::imageSize() const { return m_imageSize; }

QSizeF KiriImageView::viewportSize() const { return m_viewportSize; }

void KiriImageView::setViewportSize(const QSizeF &viewportSize)
{
    const QSizeF normalizedViewportSize(
        std::max<qreal>(0.0, viewportSize.width()), std::max<qreal>(0.0, viewportSize.height()));
    if (approximatelyEqual(m_viewportSize, normalizedViewportSize)) {
        return;
    }

    m_viewportSize = normalizedViewportSize;
    Q_EMIT viewportSizeChanged();
    updateZoomState();
}

QSizeF KiriImageView::displaySize() const { return m_displaySize; }

qreal KiriImageView::zoomPercent() const { return m_zoomPercent; }

void KiriImageView::setZoomPercent(qreal zoomPercent)
{
    if (!std::isfinite(zoomPercent)) {
        return;
    }

    const qreal clampedZoomPercent
        = std::clamp(zoomPercent, minimumManualZoomPercent, maximumManualZoomPercent);
    const bool zoomChanged = !approximatelyEqual(m_zoomPercent, clampedZoomPercent);
    const bool modeChanged = m_zoomMode != ZoomMode::Manual;

    if (!zoomChanged && !modeChanged) {
        return;
    }

    m_zoomMode = ZoomMode::Manual;
    m_zoomPercent = clampedZoomPercent;

    if (modeChanged) {
        Q_EMIT zoomModeChanged();
    }
    if (zoomChanged) {
        Q_EMIT zoomPercentChanged();
    }

    setDisplaySize(displaySizeForZoomPercent(m_zoomPercent));
}

KiriImageView::ZoomMode KiriImageView::zoomMode() const { return m_zoomMode; }

int KiriImageView::currentPageNumber() const
{
    return m_currentPageIndex < 0 ? 0 : m_currentPageIndex + 1;
}

int KiriImageView::imageCount() const { return static_cast<int>(m_pageNavigationUrls.size()); }

bool KiriImageView::containerNavigationAvailable() const
{
    return !m_containerNavigationUrl.isEmpty();
}

void KiriImageView::openPreviousImage() { openAdjacentImage(NavigationDirection::Previous); }

void KiriImageView::openNextImage() { openAdjacentImage(NavigationDirection::Next); }

void KiriImageView::openPreviousContainer()
{
    openAdjacentContainer(NavigationDirection::Previous);
}

void KiriImageView::openNextContainer() { openAdjacentContainer(NavigationDirection::Next); }

void KiriImageView::openImageAtPage(int pageNumber)
{
    if (pageNumber <= 0 || pageNumber > imageCount()) {
        return;
    }

    const int pageIndex = pageNumber - 1;
    if (pageIndex == m_currentPageIndex) {
        return;
    }

    setSourceUrl(m_pageNavigationUrls.at(static_cast<std::size_t>(pageIndex)));
}

void KiriImageView::resetZoom()
{
    setZoomMode(ZoomMode::Fit);
    updateZoomState();
}

void KiriImageView::setFitMode(ZoomMode zoomMode)
{
    if (zoomMode == ZoomMode::Manual) {
        return;
    }

    setZoomMode(zoomMode);
    updateZoomState();
}

QSGNode *KiriImageView::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    if (m_image.isNull()) {
        delete oldNode;
        return nullptr;
    }

    const QSizeF boundsSize(width(), height());
    if (boundsSize.isEmpty()) {
        delete oldNode;
        return nullptr;
    }

    auto *node = static_cast<KiriView::KiriImageRenderNode *>(oldNode);
    if (node == nullptr) {
        node = new KiriView::KiriImageRenderNode;
    }

    node->setRhi(window() == nullptr ? nullptr : window()->rhi());
    node->setImage(m_image, m_imageRevision);
    node->setTargetRect(KiriView::imageTargetRect(m_image.size(), boundsSize));
    node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    return node;
}

void KiriImageView::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);

    if (change == ItemSceneChange || change == ItemDevicePixelRatioHasChanged) {
        updateZoomState();
    }
}

void KiriImageView::startLoad()
{
    cancelLoad();
    cancelPredecode();
    const quint64 generation = ++m_loadGeneration;
    setErrorString(QString());

    if (m_sourceUrl.isEmpty()) {
        clearImage();
        setZoomMode(ZoomMode::Fit);
        updateZoomState();
        setLoading(false);
        m_comicBookRootUrl = QUrl();
        m_loadingContainerNavigationUrl = QUrl();
        setContainerNavigationUrl(QUrl());
        setStatus(Status::Null);
        return;
    }

    if (!hasDisplayedImage() && m_loadingContainerNavigationUrl.isEmpty()) {
        setContainerNavigationUrl(QUrl());
    }

    setLoading(true);
    if (!hasDisplayedImage()) {
        clearImage();
        setZoomMode(ZoomMode::Fit);
        updateZoomState();
        setStatus(Status::Loading);
    } else {
        setStatus(Status::Ready);
    }

    const std::optional<QUrl> selectedArchiveRootUrl = comicBookArchiveRootUrl(m_sourceUrl);
    if (selectedArchiveRootUrl.has_value()) {
        m_comicBookRootUrl = selectedArchiveRootUrl.value();
        startComicBookLoad(m_comicBookRootUrl, generation);
        return;
    }

    if (!isUrlInsideArchiveRoot(m_sourceUrl, m_comicBookRootUrl)) {
        const std::optional<QUrl> containingArchiveRootUrl
            = containingComicBookArchiveRootUrl(m_sourceUrl);
        m_comicBookRootUrl = containingArchiveRootUrl.has_value()
                && isUrlInsideArchiveRoot(m_sourceUrl, containingArchiveRootUrl.value())
            ? containingArchiveRootUrl.value()
            : QUrl();
    }

    if (tryDisplayPredecodedImage(m_sourceUrl)) {
        return;
    }

    startImageLoad(m_sourceUrl, generation);
}

void KiriImageView::startImageLoad(const QUrl &url, quint64 generation)
{
    auto *job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    m_job = job;

    connect(job, &KJob::result, this, [this, job, generation](KJob *finishedJob) {
        if (generation != m_loadGeneration || job != m_job) {
            return;
        }

        m_job = nullptr;

        if (finishedJob->error() != KJob::NoError) {
            finishLoadWithError(finishedJob->errorString());
            return;
        }

        startImageDecode(job->data(), generation);
    });

    connect(job, &QObject::destroyed, this, [this, job]() {
        if (job == m_job) {
            m_job = nullptr;
        }
    });
}

void KiriImageView::startComicBookLoad(const QUrl &archiveRootUrl, quint64 generation)
{
    auto *job = KIO::listRecursive(archiveRootUrl, KIO::HideProgressInfo,
        KIO::ListJob::ListFlags(KIO::ListJob::ListFlag::IncludeHidden));
    auto candidates = std::make_shared<std::vector<ImageNavigationCandidate>>();
    m_archiveListJob = job;

    connect(job, &KIO::ListJob::entries, this,
        [archiveRootUrl, candidates](KIO::Job *entriesJob, const KIO::UDSEntryList &entries) {
            auto *listJob = qobject_cast<KIO::ListJob *>(entriesJob);
            const QUrl directoryUrl = listJob == nullptr ? archiveRootUrl : listJob->url();
            appendArchiveImageNavigationCandidates(
                candidates.get(), entries, directoryUrl, archiveRootUrl);
        });

    connect(job, &KJob::result, this,
        [this, job, generation, archiveRootUrl, candidates](KJob *finishedJob) {
            if (generation != m_loadGeneration || job != m_archiveListJob) {
                return;
            }

            m_archiveListJob = nullptr;

            if (finishedJob->error() != KJob::NoError) {
                m_comicBookRootUrl = QUrl();
                finishLoadWithError(finishedJob->errorString());
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            if (candidates->empty()) {
                m_comicBookRootUrl = QUrl();
                finishLoadWithError(
                    tr("The selected comic book archive does not contain any supported images."));
                return;
            }

            m_comicBookRootUrl = archiveRootUrl;
            setSourceUrl(candidates->front().url);
        });

    connect(job, &QObject::destroyed, this, [this, job]() {
        if (job == m_archiveListJob) {
            m_archiveListJob = nullptr;
        }
    });
}

void KiriImageView::cancelLoad()
{
    if (m_job == nullptr && m_archiveListJob == nullptr) {
        return;
    }

    ++m_loadGeneration;

    if (m_job != nullptr) {
        auto *job = m_job;
        m_job = nullptr;
        job->kill(KJob::Quietly);
    }

    if (m_archiveListJob != nullptr) {
        auto *job = m_archiveListJob;
        m_archiveListJob = nullptr;
        job->kill(KJob::Quietly);
    }
}

void KiriImageView::openAdjacentImage(NavigationDirection direction)
{
    if (!hasDisplayedImage() || m_sourceUrl.isEmpty()) {
        return;
    }

    cancelContainerNavigation();

    if (isUrlInsideArchiveRoot(m_sourceUrl, m_comicBookRootUrl)) {
        openAdjacentComicBookImage(direction);
        return;
    }

    const QUrl currentUrl = navigationSourceUrl(m_sourceUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    cancelNavigation();

    auto *lister = new KCoreDirLister(this);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);

    const quint64 generation = ++m_navigationGeneration;
    m_navigationLister = lister;

    connect(lister, &KCoreDirLister::completed, this,
        [this, lister, generation, direction, currentUrl]() {
            finishNavigation(lister, generation, direction, currentUrl);
        });
    connect(lister, &KCoreDirLister::jobError, this,
        [this, lister, generation](KIO::Job *) { finishNavigationWithError(lister, generation); });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        finishNavigationWithError(lister, generation);
    }
}

void KiriImageView::openAdjacentComicBookImage(NavigationDirection direction)
{
    const QUrl currentUrl = m_sourceUrl.adjusted(QUrl::NormalizePathSegments);
    const QUrl archiveRootUrl = m_comicBookRootUrl;
    if (!currentUrl.isValid() || archiveRootUrl.isEmpty()) {
        return;
    }

    cancelNavigation();

    auto *job = KIO::listRecursive(archiveRootUrl, KIO::HideProgressInfo,
        KIO::ListJob::ListFlags(KIO::ListJob::ListFlag::IncludeHidden));
    auto candidates = std::make_shared<std::vector<ImageNavigationCandidate>>();
    const quint64 generation = ++m_navigationGeneration;
    m_navigationListJob = job;

    connect(job, &KIO::ListJob::entries, this,
        [archiveRootUrl, candidates](KIO::Job *entriesJob, const KIO::UDSEntryList &entries) {
            auto *listJob = qobject_cast<KIO::ListJob *>(entriesJob);
            const QUrl directoryUrl = listJob == nullptr ? archiveRootUrl : listJob->url();
            appendArchiveImageNavigationCandidates(
                candidates.get(), entries, directoryUrl, archiveRootUrl);
        });

    connect(job, &KJob::result, this,
        [this, job, generation, direction, currentUrl, candidates](KJob *finishedJob) {
            if (generation != m_navigationGeneration || job != m_navigationListJob) {
                return;
            }

            m_navigationListJob = nullptr;

            if (finishedJob->error() != KJob::NoError) {
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            const auto current = std::find_if(candidates->cbegin(), candidates->cend(),
                [&currentUrl](const ImageNavigationCandidate &candidate) {
                    return candidate.url.matches(currentUrl, QUrl::NormalizePathSegments);
                });
            if (current == candidates->cend()) {
                return;
            }

            QUrl targetUrl;
            if (direction == NavigationDirection::Previous) {
                if (current == candidates->cbegin()) {
                    return;
                }
                targetUrl = std::prev(current)->url;
            } else {
                const auto next = std::next(current);
                if (next == candidates->cend()) {
                    return;
                }
                targetUrl = next->url;
            }

            setSourceUrl(targetUrl);
        });

    connect(job, &QObject::destroyed, this, [this, job]() {
        if (job == m_navigationListJob) {
            m_navigationListJob = nullptr;
        }
    });
}

void KiriImageView::cancelNavigation()
{
    if (m_navigationLister == nullptr && m_navigationListJob == nullptr) {
        return;
    }

    ++m_navigationGeneration;

    if (m_navigationLister != nullptr) {
        auto *lister = m_navigationLister;
        m_navigationLister = nullptr;
        lister->stop();
        lister->deleteLater();
    }

    if (m_navigationListJob != nullptr) {
        auto *job = m_navigationListJob;
        m_navigationListJob = nullptr;
        job->kill(KJob::Quietly);
    }
}

void KiriImageView::finishNavigation(KCoreDirLister *lister, quint64 generation,
    NavigationDirection direction, const QUrl &currentUrl)
{
    if (generation != m_navigationGeneration || lister != m_navigationLister) {
        return;
    }

    const std::vector<ImageNavigationCandidate> candidates
        = imageNavigationCandidates(lister->items(KCoreDirLister::AllItems));

    m_navigationLister = nullptr;
    lister->deleteLater();

    const auto current = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentUrl](const ImageNavigationCandidate &candidate) {
            return candidate.url.matches(currentUrl, QUrl::NormalizePathSegments);
        });
    if (current == candidates.cend()) {
        return;
    }

    QUrl targetUrl;
    if (direction == NavigationDirection::Previous) {
        if (current == candidates.cbegin()) {
            return;
        }
        targetUrl = std::prev(current)->url;
    } else {
        const auto next = std::next(current);
        if (next == candidates.cend()) {
            return;
        }
        targetUrl = next->url;
    }

    setSourceUrl(targetUrl);
}

void KiriImageView::finishNavigationWithError(KCoreDirLister *lister, quint64 generation)
{
    if (generation != m_navigationGeneration || lister != m_navigationLister) {
        return;
    }

    m_navigationLister = nullptr;
    lister->deleteLater();
}

void KiriImageView::openAdjacentContainer(NavigationDirection direction)
{
    if (!containerNavigationAvailable()) {
        return;
    }

    const QUrl currentContainerUrl = m_containerNavigationUrl;
    const QUrl parentUrl = parentUrlForContainerNavigation(currentContainerUrl);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    cancelNavigation();
    cancelContainerNavigation();

    auto *lister = new KCoreDirLister(this);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);

    const quint64 generation = ++m_containerNavigationGeneration;
    m_containerNavigationLister = lister;

    connect(lister, &KCoreDirLister::completed, this,
        [this, lister, generation, direction, currentContainerUrl]() {
            finishContainerNavigation(lister, generation, direction, currentContainerUrl);
        });
    connect(lister, &KCoreDirLister::jobError, this, [this, lister, generation](KIO::Job *) {
        finishContainerNavigationWithError(lister, generation);
    });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        finishContainerNavigationWithError(lister, generation);
    }
}

void KiriImageView::cancelContainerNavigation()
{
    if (m_containerNavigationLister == nullptr && m_containerNavigationListJob == nullptr) {
        return;
    }

    ++m_containerNavigationGeneration;

    if (m_containerNavigationLister != nullptr) {
        auto *lister = m_containerNavigationLister;
        m_containerNavigationLister = nullptr;
        lister->stop();
        lister->deleteLater();
    }

    if (m_containerNavigationListJob != nullptr) {
        auto *job = m_containerNavigationListJob;
        m_containerNavigationListJob = nullptr;
        job->kill(KJob::Quietly);
    }
}

void KiriImageView::finishContainerNavigation(KCoreDirLister *lister, quint64 generation,
    NavigationDirection direction, const QUrl &currentContainerUrl)
{
    if (generation != m_containerNavigationGeneration || lister != m_containerNavigationLister) {
        return;
    }

    const std::vector<ContainerNavigationCandidate> candidates
        = containerNavigationCandidates(lister->items(KCoreDirLister::AllItems));

    m_containerNavigationLister = nullptr;
    lister->deleteLater();

    const auto current = std::find_if(candidates.cbegin(), candidates.cend(),
        [&currentContainerUrl](const ContainerNavigationCandidate &candidate) {
            return candidate.url.matches(currentContainerUrl, QUrl::NormalizePathSegments);
        });
    if (current == candidates.cend()) {
        return;
    }

    std::optional<ContainerNavigationCandidate> target;
    if (direction == NavigationDirection::Previous) {
        if (current == candidates.cbegin()) {
            return;
        }
        target = *std::prev(current);
    } else {
        const auto next = std::next(current);
        if (next == candidates.cend()) {
            return;
        }
        target = *next;
    }

    if (target->type == ContainerNavigationCandidateType::Directory) {
        openDirectoryContainer(target->url, generation);
    } else {
        openComicBookContainer(target->url, generation);
    }
}

void KiriImageView::finishContainerNavigationWithError(KCoreDirLister *lister, quint64 generation)
{
    if (generation != m_containerNavigationGeneration || lister != m_containerNavigationLister) {
        return;
    }

    m_containerNavigationLister = nullptr;
    lister->deleteLater();
}

void KiriImageView::openDirectoryContainer(const QUrl &containerUrl, quint64 generation)
{
    auto *lister = new KCoreDirLister(this);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    m_containerNavigationLister = lister;

    connect(lister, &KCoreDirLister::completed, this, [this, lister, generation, containerUrl]() {
        finishDirectoryContainerNavigation(lister, generation, containerUrl);
    });
    connect(lister, &KCoreDirLister::jobError, this,
        [this, lister, generation, containerUrl](KIO::Job *job) {
            const QString errorString = job == nullptr ? QString() : job->errorString();
            finishDirectoryContainerNavigationWithError(
                lister, generation, containerUrl, errorString);
        });

    if (!lister->openUrl(containerUrl, KCoreDirLister::Reload)) {
        finishDirectoryContainerNavigationWithError(lister, generation, containerUrl, QString());
    }
}

void KiriImageView::finishDirectoryContainerNavigation(
    KCoreDirLister *lister, quint64 generation, const QUrl &containerUrl)
{
    if (generation != m_containerNavigationGeneration || lister != m_containerNavigationLister) {
        return;
    }

    const std::vector<ImageNavigationCandidate> candidates
        = imageNavigationCandidates(lister->items(KCoreDirLister::AllItems));

    m_containerNavigationLister = nullptr;
    lister->deleteLater();

    if (candidates.empty()) {
        finishContainerNavigationWithEmptyContainer(containerUrl);
        return;
    }

    openImageFromContainerNavigation(candidates.front().url, containerUrl);
}

void KiriImageView::finishDirectoryContainerNavigationWithError(KCoreDirLister *lister,
    quint64 generation, const QUrl &containerUrl, const QString &errorString)
{
    if (generation != m_containerNavigationGeneration || lister != m_containerNavigationLister) {
        return;
    }

    m_containerNavigationLister = nullptr;
    lister->deleteLater();
    finishContainerNavigationLoadWithError(containerUrl, errorString);
}

void KiriImageView::openComicBookContainer(const QUrl &containerUrl, quint64 generation)
{
    const std::optional<QUrl> archiveRootUrl = comicBookArchiveRootUrl(containerUrl);
    if (!archiveRootUrl.has_value()) {
        finishContainerNavigationLoadWithError(
            containerUrl, tr("Could not open the selected comic book archive."));
        return;
    }

    const QUrl archiveRoot = archiveRootUrl.value();
    auto *job = KIO::listRecursive(archiveRoot, KIO::HideProgressInfo,
        KIO::ListJob::ListFlags(KIO::ListJob::ListFlag::IncludeHidden));
    auto candidates = std::make_shared<std::vector<ImageNavigationCandidate>>();
    m_containerNavigationListJob = job;

    connect(job, &KIO::ListJob::entries, this,
        [archiveRoot, candidates](KIO::Job *entriesJob, const KIO::UDSEntryList &entries) {
            auto *listJob = qobject_cast<KIO::ListJob *>(entriesJob);
            const QUrl directoryUrl = listJob == nullptr ? archiveRoot : listJob->url();
            appendArchiveImageNavigationCandidates(
                candidates.get(), entries, directoryUrl, archiveRoot);
        });

    connect(job, &KJob::result, this,
        [this, job, generation, containerUrl, candidates](KJob *finishedJob) {
            if (generation != m_containerNavigationGeneration
                || job != m_containerNavigationListJob) {
                return;
            }

            m_containerNavigationListJob = nullptr;

            if (finishedJob->error() != KJob::NoError) {
                finishContainerNavigationLoadWithError(containerUrl, finishedJob->errorString());
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            if (candidates->empty()) {
                finishContainerNavigationWithEmptyContainer(containerUrl);
                return;
            }

            openImageFromContainerNavigation(candidates->front().url, containerUrl);
        });

    connect(job, &QObject::destroyed, this, [this, job]() {
        if (job == m_containerNavigationListJob) {
            m_containerNavigationListJob = nullptr;
        }
    });
}

void KiriImageView::openImageFromContainerNavigation(const QUrl &imageUrl, const QUrl &containerUrl)
{
    setSourceUrlForLoad(imageUrl, containerUrl);
}

void KiriImageView::finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl)
{
    finishContainerNavigationLoadWithError(
        containerUrl, tr("The selected container does not contain any supported images."));
}

void KiriImageView::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, const QString &errorString)
{
    cancelLoad();
    m_loadingContainerNavigationUrl = QUrl();
    m_comicBookRootUrl = QUrl();

    clearImage();
    m_zoomContainerUrl = containerUrl;
    setZoomMode(ZoomMode::Fit);
    updateZoomState();
    setLoading(false);
    setContainerNavigationUrl(containerUrl);

    if (m_sourceUrl != containerUrl) {
        m_sourceUrl = containerUrl;
        Q_EMIT sourceUrlChanged();
    }

    const QString message
        = errorString.isEmpty() ? tr("Could not open the selected container.") : errorString;
    setErrorString(message);
    setStatus(Status::Error);
}

void KiriImageView::setContainerNavigationUrl(const QUrl &containerUrl)
{
    if (m_containerNavigationUrl == containerUrl) {
        return;
    }

    m_containerNavigationUrl = containerUrl;
    Q_EMIT containerNavigationChanged();
}

void KiriImageView::updateContainerNavigationFromDisplayedImage()
{
    if (!hasDisplayedImage() || m_displayedUrl.isEmpty()) {
        setContainerNavigationUrl(QUrl());
        return;
    }

    setContainerNavigationUrl(
        containerNavigationUrlForImage(m_displayedUrl, m_displayedComicBookRootUrl));
}

void KiriImageView::updatePageNavigation()
{
    cancelPageNavigationUpdate();

    if (!hasDisplayedImage() || m_displayedUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    const bool isComicBookPage
        = isUrlInsideArchiveRoot(m_displayedUrl, m_displayedComicBookRootUrl);
    const QUrl currentUrl = isComicBookPage ? m_displayedUrl.adjusted(QUrl::NormalizePathSegments)
                                            : navigationSourceUrl(m_displayedUrl);
    if (!currentUrl.isValid() || currentUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    setFallbackPageNavigationUrl(currentUrl);

    const quint64 generation = ++m_pageNavigationGeneration;
    if (isComicBookPage) {
        updateComicBookPageNavigation(generation, currentUrl, m_displayedComicBookRootUrl);
        return;
    }

    updateFilePageNavigation(generation, currentUrl);
}

void KiriImageView::updateFilePageNavigation(quint64 generation, const QUrl &currentUrl)
{
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        return;
    }

    auto *lister = new KCoreDirLister(this);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    m_pageNavigationLister = lister;

    connect(lister, &KCoreDirLister::completed, this, [this, lister, generation, currentUrl]() {
        if (generation != m_pageNavigationGeneration || lister != m_pageNavigationLister) {
            return;
        }

        const std::vector<ImageNavigationCandidate> candidates
            = imageNavigationCandidates(lister->items(KCoreDirLister::AllItems));
        m_pageNavigationLister = nullptr;
        lister->deleteLater();

        setPageNavigationUrls(imageNavigationCandidateUrls(candidates), currentUrl);
    });
    connect(lister, &KCoreDirLister::jobError, this, [this, lister, generation](KIO::Job *) {
        finishPageNavigationUpdateWithError(lister, generation);
    });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        finishPageNavigationUpdateWithError(lister, generation);
    }
}

void KiriImageView::updateComicBookPageNavigation(
    quint64 generation, const QUrl &currentUrl, const QUrl &archiveRootUrl)
{
    if (archiveRootUrl.isEmpty()) {
        return;
    }

    auto *job = KIO::listRecursive(archiveRootUrl, KIO::HideProgressInfo,
        KIO::ListJob::ListFlags(KIO::ListJob::ListFlag::IncludeHidden));
    auto candidates = std::make_shared<std::vector<ImageNavigationCandidate>>();
    m_pageNavigationListJob = job;

    connect(job, &KIO::ListJob::entries, this,
        [archiveRootUrl, candidates](KIO::Job *entriesJob, const KIO::UDSEntryList &entries) {
            auto *listJob = qobject_cast<KIO::ListJob *>(entriesJob);
            const QUrl directoryUrl = listJob == nullptr ? archiveRootUrl : listJob->url();
            appendArchiveImageNavigationCandidates(
                candidates.get(), entries, directoryUrl, archiveRootUrl);
        });

    connect(job, &KJob::result, this,
        [this, job, generation, currentUrl, candidates](KJob *finishedJob) {
            if (generation != m_pageNavigationGeneration || job != m_pageNavigationListJob) {
                return;
            }

            m_pageNavigationListJob = nullptr;
            if (finishedJob->error() != KJob::NoError) {
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            setPageNavigationUrls(imageNavigationCandidateUrls(*candidates), currentUrl);
        });

    connect(job, &QObject::destroyed, this, [this, job]() {
        if (job == m_pageNavigationListJob) {
            m_pageNavigationListJob = nullptr;
        }
    });
}

void KiriImageView::cancelPageNavigationUpdate()
{
    if (m_pageNavigationLister == nullptr && m_pageNavigationListJob == nullptr) {
        return;
    }

    ++m_pageNavigationGeneration;

    if (m_pageNavigationLister != nullptr) {
        auto *lister = m_pageNavigationLister;
        m_pageNavigationLister = nullptr;
        lister->stop();
        lister->deleteLater();
    }

    if (m_pageNavigationListJob != nullptr) {
        auto *job = m_pageNavigationListJob;
        m_pageNavigationListJob = nullptr;
        job->kill(KJob::Quietly);
    }
}

void KiriImageView::finishPageNavigationUpdateWithError(KCoreDirLister *lister, quint64 generation)
{
    if (generation != m_pageNavigationGeneration || lister != m_pageNavigationLister) {
        return;
    }

    m_pageNavigationLister = nullptr;
    lister->deleteLater();
}

void KiriImageView::setPageNavigationUrls(std::vector<QUrl> urls, const QUrl &currentUrl)
{
    const auto current = std::find_if(urls.cbegin(), urls.cend(), [&currentUrl](const QUrl &url) {
        return url.matches(currentUrl, QUrl::NormalizePathSegments);
    });

    int currentPageIndex = -1;
    if (current == urls.cend()) {
        if (currentUrl.isValid() && !currentUrl.isEmpty()) {
            urls.insert(urls.begin(), currentUrl.adjusted(QUrl::NormalizePathSegments));
            currentPageIndex = 0;
        }
    } else {
        currentPageIndex = static_cast<int>(std::distance(urls.cbegin(), current));
    }

    if (m_pageNavigationUrls == urls && m_currentPageIndex == currentPageIndex) {
        return;
    }

    m_pageNavigationUrls = std::move(urls);
    m_currentPageIndex = currentPageIndex;
    Q_EMIT pageNavigationChanged();
}

void KiriImageView::setFallbackPageNavigationUrl(const QUrl &currentUrl)
{
    if (!currentUrl.isValid() || currentUrl.isEmpty()) {
        clearPageNavigation();
        return;
    }

    setPageNavigationUrls({ currentUrl.adjusted(QUrl::NormalizePathSegments) }, currentUrl);
}

void KiriImageView::clearPageNavigation()
{
    if (m_pageNavigationUrls.empty() && m_currentPageIndex == -1) {
        return;
    }

    m_pageNavigationUrls.clear();
    m_currentPageIndex = -1;
    Q_EMIT pageNavigationChanged();
}

void KiriImageView::scheduleAdjacentImagePredecode()
{
    cancelPredecode();
    if (!hasDisplayedImage() || m_displayedUrl.isEmpty()) {
        return;
    }

    const quint64 generation = m_predecodeGeneration;
    if (isUrlInsideArchiveRoot(m_displayedUrl, m_displayedComicBookRootUrl)) {
        scheduleComicBookAdjacentImagePredecode(generation);
        return;
    }

    scheduleFileAdjacentImagePredecode(generation);
}

void KiriImageView::scheduleFileAdjacentImagePredecode(quint64 generation)
{
    const QUrl currentUrl = navigationSourceUrl(m_displayedUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        startPredecodeImageLoads({}, QUrl(), generation);
        return;
    }

    auto *lister = new KCoreDirLister(this);
    lister->setAutoErrorHandlingEnabled(false);
    lister->setAutoUpdate(false);
    lister->setDelayedMimeTypes(true);
    lister->setShowHiddenFiles(true);
    m_predecodeLister = lister;

    connect(lister, &KCoreDirLister::completed, this, [this, lister, generation, currentUrl]() {
        if (generation != m_predecodeGeneration || lister != m_predecodeLister) {
            return;
        }

        const std::vector<ImageNavigationCandidate> candidates
            = imageNavigationCandidates(lister->items(KCoreDirLister::AllItems));
        m_predecodeLister = nullptr;
        lister->deleteLater();

        startPredecodeImageLoads(
            predecodeWindowImageUrls(candidates, currentUrl), QUrl(), generation);
    });
    connect(lister, &KCoreDirLister::jobError, this, [this, lister, generation](KIO::Job *) {
        if (generation != m_predecodeGeneration || lister != m_predecodeLister) {
            return;
        }

        m_predecodeLister = nullptr;
        lister->deleteLater();
        startPredecodeImageLoads({}, QUrl(), generation);
    });

    if (!lister->openUrl(parentUrl, KCoreDirLister::Reload)) {
        m_predecodeLister = nullptr;
        lister->deleteLater();
        startPredecodeImageLoads({}, QUrl(), generation);
    }
}

void KiriImageView::scheduleComicBookAdjacentImagePredecode(quint64 generation)
{
    const QUrl currentUrl = m_displayedUrl.adjusted(QUrl::NormalizePathSegments);
    const QUrl archiveRootUrl = m_displayedComicBookRootUrl;
    if (!currentUrl.isValid() || archiveRootUrl.isEmpty()) {
        startPredecodeImageLoads({}, archiveRootUrl, generation);
        return;
    }

    auto *job = KIO::listRecursive(archiveRootUrl, KIO::HideProgressInfo,
        KIO::ListJob::ListFlags(KIO::ListJob::ListFlag::IncludeHidden));
    auto candidates = std::make_shared<std::vector<ImageNavigationCandidate>>();
    m_predecodeListJob = job;

    connect(job, &KIO::ListJob::entries, this,
        [archiveRootUrl, candidates](KIO::Job *entriesJob, const KIO::UDSEntryList &entries) {
            auto *listJob = qobject_cast<KIO::ListJob *>(entriesJob);
            const QUrl directoryUrl = listJob == nullptr ? archiveRootUrl : listJob->url();
            appendArchiveImageNavigationCandidates(
                candidates.get(), entries, directoryUrl, archiveRootUrl);
        });

    connect(job, &KJob::result, this,
        [this, job, generation, currentUrl, archiveRootUrl, candidates](KJob *finishedJob) {
            if (generation != m_predecodeGeneration || job != m_predecodeListJob) {
                return;
            }

            m_predecodeListJob = nullptr;
            if (finishedJob->error() != KJob::NoError) {
                startPredecodeImageLoads({}, archiveRootUrl, generation);
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            startPredecodeImageLoads(
                predecodeWindowImageUrls(*candidates, currentUrl), archiveRootUrl, generation);
        });

    connect(job, &QObject::destroyed, this, [this, job]() {
        if (job == m_predecodeListJob) {
            m_predecodeListJob = nullptr;
        }
    });
}

void KiriImageView::startPredecodeImageLoads(
    const std::vector<QUrl> &urls, const QUrl &comicBookRootUrl, quint64 generation)
{
    if (generation != m_predecodeGeneration) {
        return;
    }

    m_predecodeWindowUrls.clear();
    m_predecodeQueue.clear();

    for (const QUrl &url : urls) {
        const QUrl normalizedUrl = normalizedImageUrl(url);
        const bool alreadyInWindow
            = std::find(m_predecodeWindowUrls.cbegin(), m_predecodeWindowUrls.cend(), normalizedUrl)
            != m_predecodeWindowUrls.cend();
        if (!normalizedUrl.isValid() || normalizedUrl.isEmpty() || alreadyInWindow) {
            continue;
        }

        m_predecodeWindowUrls.push_back(normalizedUrl);
    }

    trimPredecodedImagesToPredecodeWindow();
    cacheDisplayedImageForPredecode();

    const QUrl displayedUrl = normalizedImageUrl(m_displayedUrl);
    for (const QUrl &url : m_predecodeWindowUrls) {
        if (url == displayedUrl) {
            continue;
        }
        if (!hasPredecodedImage(url) && !isPredecodeInFlight(url)) {
            m_predecodeQueue.push_back(PredecodeRequest { url, comicBookRootUrl });
        }
    }

    startNextPredecodeImageLoad(generation);
}

void KiriImageView::startNextPredecodeImageLoad(quint64 generation)
{
    if (generation != m_predecodeGeneration || !m_activePredecodeUrl.isEmpty()) {
        return;
    }

    while (!m_predecodeQueue.empty()) {
        PredecodeRequest request = std::move(m_predecodeQueue.front());
        m_predecodeQueue.erase(m_predecodeQueue.begin());

        if (!request.url.isValid() || request.url.isEmpty() || !predecodeWindowContains(request.url)
            || hasPredecodedImage(request.url)) {
            continue;
        }

        startPredecodeImageLoad(request.url, request.comicBookRootUrl, generation);
        return;
    }
}

void KiriImageView::startPredecodeImageLoad(
    const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation)
{
    if (!url.isValid() || url.isEmpty() || !m_activePredecodeUrl.isEmpty()
        || hasPredecodedImage(url) || !predecodeWindowContains(url)) {
        return;
    }

    auto *job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    const QUrl normalizedUrl = normalizedImageUrl(url);
    m_predecodeJob = job;
    m_activePredecodeUrl = normalizedUrl;

    connect(job, &KJob::result, this,
        [this, job, generation, url, comicBookRootUrl, normalizedUrl](KJob *finishedJob) {
            const bool activeJob = job == m_predecodeJob && normalizedUrl == m_activePredecodeUrl;
            if (activeJob) {
                m_predecodeJob = nullptr;
            }

            if (generation != m_predecodeGeneration || !activeJob) {
                return;
            }

            if (finishedJob->error() != KJob::NoError) {
                m_activePredecodeUrl = QUrl();
                startNextPredecodeImageLoad(generation);
                return;
            }

            startPredecodeImageDecode(job->data(), url, comicBookRootUrl, generation);
        });

    connect(job, &QObject::destroyed, this, [this, job, generation]() {
        if (job != m_predecodeJob) {
            return;
        }

        m_predecodeJob = nullptr;
        m_activePredecodeUrl = QUrl();
        if (generation == m_predecodeGeneration) {
            startNextPredecodeImageLoad(generation);
        }
    });
}

void KiriImageView::startPredecodeImageDecode(
    QByteArray data, const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation)
{
    const QPointer<KiriImageView> view(this);
    QThreadPool::globalInstance()->start(QRunnable::create(
        [view, data = std::move(data), url, comicBookRootUrl, generation]() mutable {
            auto result = std::make_shared<DecodedImageResult>(decodeImageData(data));
            if (view == nullptr) {
                return;
            }

            QMetaObject::invokeMethod(
                view.data(),
                [view, generation, url, comicBookRootUrl, result]() {
                    if (view == nullptr || generation != view->m_predecodeGeneration) {
                        return;
                    }

                    const QUrl normalizedUrl = normalizedImageUrl(url);
                    if (normalizedUrl != view->m_activePredecodeUrl) {
                        return;
                    }

                    if (decodedImageResultIsPredecodeCacheable(*result, predecodeCacheByteBudget)) {
                        view->cachePredecodedImage(url, comicBookRootUrl, result->image);
                    }

                    view->m_activePredecodeUrl = QUrl();
                    view->startNextPredecodeImageLoad(generation);
                },
                Qt::QueuedConnection);
        }));
}

void KiriImageView::cancelPredecode()
{
    ++m_predecodeGeneration;

    if (m_predecodeLister != nullptr) {
        auto *lister = m_predecodeLister;
        m_predecodeLister = nullptr;
        lister->stop();
        lister->deleteLater();
    }

    if (m_predecodeListJob != nullptr) {
        auto *job = m_predecodeListJob;
        m_predecodeListJob = nullptr;
        job->kill(KJob::Quietly);
    }

    if (m_predecodeJob != nullptr) {
        auto *job = m_predecodeJob;
        m_predecodeJob = nullptr;
        job->kill(KJob::Quietly);
    }
    m_activePredecodeUrl = QUrl();
    m_predecodeQueue.clear();
}

bool KiriImageView::tryDisplayPredecodedImage(const QUrl &url)
{
    QImage image;
    QUrl comicBookRootUrl;
    if (!findPredecodedImage(url, &image, &comicBookRootUrl)) {
        return false;
    }

    m_comicBookRootUrl = comicBookRootUrl;
    finishLoadSuccessfully(image, true);
    scheduleAdjacentImagePredecode();
    return true;
}

bool KiriImageView::findPredecodedImage(
    const QUrl &url, QImage *image, QUrl *comicBookRootUrl) const
{
    const QUrl normalizedUrl = normalizedImageUrl(url);
    const auto cached = std::find_if(m_predecodedImages.cbegin(), m_predecodedImages.cend(),
        [&normalizedUrl](const PredecodedImage &entry) { return entry.url == normalizedUrl; });
    if (cached == m_predecodedImages.cend()) {
        return false;
    }

    *image = cached->image;
    *comicBookRootUrl = cached->comicBookRootUrl;
    return true;
}

void KiriImageView::cachePredecodedImage(
    const QUrl &url, const QUrl &comicBookRootUrl, const QImage &image)
{
    const qsizetype byteCost = imageByteCost(image);
    if (byteCost <= 0 || byteCost > predecodeCacheByteBudget) {
        return;
    }

    const QUrl normalizedUrl = normalizedImageUrl(url);
    if (!predecodeWindowContains(normalizedUrl)) {
        return;
    }

    m_predecodedImages.erase(
        std::remove_if(m_predecodedImages.begin(), m_predecodedImages.end(),
            [&normalizedUrl](const PredecodedImage &entry) { return entry.url == normalizedUrl; }),
        m_predecodedImages.end());
    m_predecodedImages.push_back(
        PredecodedImage { normalizedUrl, comicBookRootUrl, image, byteCost });

    trimPredecodedImagesToPredecodeWindow();
}

void KiriImageView::cacheDisplayedImageForPredecode()
{
    if (!m_displayedImageIsPredecodeCacheable || m_displayedUrl.isEmpty() || m_image.isNull()) {
        return;
    }

    cachePredecodedImage(m_displayedUrl, m_displayedComicBookRootUrl, m_image);
}

bool KiriImageView::predecodeWindowContains(const QUrl &url) const
{
    const QUrl normalizedUrl = normalizedImageUrl(url);
    return std::find(m_predecodeWindowUrls.cbegin(), m_predecodeWindowUrls.cend(), normalizedUrl)
        != m_predecodeWindowUrls.cend();
}

void KiriImageView::trimPredecodedImagesToPredecodeWindow()
{
    auto priority = [this](const QUrl &url) {
        const auto priorityEntry
            = std::find(m_predecodeWindowUrls.cbegin(), m_predecodeWindowUrls.cend(), url);
        if (priorityEntry == m_predecodeWindowUrls.cend()) {
            return m_predecodeWindowUrls.size();
        }

        return static_cast<std::size_t>(
            std::distance(m_predecodeWindowUrls.cbegin(), priorityEntry));
    };

    m_predecodedImages.erase(
        std::remove_if(m_predecodedImages.begin(), m_predecodedImages.end(),
            [this](const PredecodedImage &entry) { return !predecodeWindowContains(entry.url); }),
        m_predecodedImages.end());

    std::stable_sort(m_predecodedImages.begin(), m_predecodedImages.end(),
        [&priority](const PredecodedImage &left, const PredecodedImage &right) {
            return priority(left.url) < priority(right.url);
        });

    qsizetype totalByteCost = 0;
    for (const PredecodedImage &entry : m_predecodedImages) {
        totalByteCost += entry.byteCost;
    }

    while (totalByteCost > predecodeCacheByteBudget && !m_predecodedImages.empty()) {
        totalByteCost -= m_predecodedImages.back().byteCost;
        m_predecodedImages.pop_back();
    }
}

bool KiriImageView::hasPredecodedImage(const QUrl &url) const
{
    const QUrl normalizedUrl = normalizedImageUrl(url);
    return std::any_of(m_predecodedImages.cbegin(), m_predecodedImages.cend(),
        [&normalizedUrl](const PredecodedImage &entry) { return entry.url == normalizedUrl; });
}

bool KiriImageView::isPredecodeInFlight(const QUrl &url) const
{
    const QUrl normalizedUrl = normalizedImageUrl(url);
    return normalizedUrl == m_activePredecodeUrl
        || std::any_of(m_predecodeQueue.cbegin(), m_predecodeQueue.cend(),
            [&normalizedUrl](
                const PredecodeRequest &request) { return request.url == normalizedUrl; });
}

void KiriImageView::startImageDecode(QByteArray data, quint64 generation)
{
    const QPointer<KiriImageView> view(this);
    QThreadPool::globalInstance()->start(
        QRunnable::create([view, data = std::move(data), generation]() mutable {
            auto result = std::make_shared<DecodedImageResult>(decodeImageData(data));
            if (view == nullptr) {
                return;
            }

            QMetaObject::invokeMethod(
                view.data(),
                [view, generation, result]() mutable {
                    if (view == nullptr || generation != view->m_loadGeneration) {
                        return;
                    }

                    if (!result->success) {
                        view->finishLoadWithError(result->errorString);
                        return;
                    }

                    if (result->isSvg) {
                        view->finishSvgLoadSuccessfully(
                            std::move(result->svgData), result->svgIntrinsicSize);
                        view->scheduleAdjacentImagePredecode();
                        return;
                    }

                    const bool predecodeCacheable
                        = decodedImageResultIsPredecodeCacheable(*result, predecodeCacheByteBudget);
                    view->finishLoadSuccessfully(result->image, predecodeCacheable);
                    if (!result->decodedAnimationFrames.empty()) {
                        view->startDecodedAnimation(
                            std::move(result->decodedAnimationFrames), result->animationLoopCount);
                    } else if (result->hasAnimationReaderFrames) {
                        view->startAnimation(result->animationData, result->animationFormat,
                            result->animationLoopCount, result->firstFrameDelay);
                    }
                    view->scheduleAdjacentImagePredecode();
                },
                Qt::QueuedConnection);
        }));
}

void KiriImageView::finishLoadWithError(const QString &errorString)
{
    const QUrl containerNavigationUrl = m_loadingContainerNavigationUrl;
    m_loadingContainerNavigationUrl = QUrl();
    if (!containerNavigationUrl.isEmpty()) {
        finishContainerNavigationLoadWithError(containerNavigationUrl, errorString);
        return;
    }

    setLoading(false);

    if (hasDisplayedImage()) {
        setErrorString(errorString);
        setStatus(Status::Ready);

        if (!m_displayedUrl.isEmpty() && m_sourceUrl != m_displayedUrl) {
            m_sourceUrl = m_displayedUrl;
            Q_EMIT sourceUrlChanged();
        }
        m_comicBookRootUrl = m_displayedComicBookRootUrl;
        updatePageNavigation();
        scheduleAdjacentImagePredecode();
        return;
    }

    clearImage();
    m_zoomContainerUrl = QUrl();
    setContainerNavigationUrl(QUrl());
    setErrorString(errorString);
    setStatus(Status::Error);
}

void KiriImageView::finishLoadSuccessfully(const QImage &image, bool predecodeCacheable)
{
    prepareSuccessfulImageLoad();
    m_displayedImageIsPredecodeCacheable = predecodeCacheable;
    setDisplayedImage(image);
    updateZoomState();
    finishSuccessfulImageLoad();
}

void KiriImageView::finishSvgLoadSuccessfully(QByteArray data, const QSize &intrinsicSize)
{
    if (data.isEmpty() || intrinsicSize.isEmpty()) {
        finishLoadWithError(tr("Could not decode the selected SVG image."));
        return;
    }

    const QUrl loadedContainerUrl = containerNavigationUrlForImage(m_sourceUrl, m_comicBookRootUrl);
    ZoomMode loadedZoomMode = ZoomMode::Fit;
    if (sameContainerNavigationUrl(loadedContainerUrl, m_zoomContainerUrl)) {
        loadedZoomMode = m_zoomMode;
    }
    const qreal loadedZoomPercent = loadedZoomMode == ZoomMode::Manual
        ? m_zoomPercent
        : fitZoomPercentForImageSize(loadedZoomMode, intrinsicSize);
    const QSizeF loadedDisplaySize = displaySizeForZoomPercent(loadedZoomPercent, intrinsicSize);
    const QSize rasterSize
        = svgRasterSize(loadedDisplaySize, displayDevicePixelRatio(), maximumTextureSize());
    const QImage image = renderSvgImage(data, rasterSize);
    if (image.isNull()) {
        finishLoadWithError(tr("Could not render the selected SVG image."));
        return;
    }

    stopAnimation();
    m_displayedImageIsPredecodeCacheable = false;
    if (m_zoomMode != loadedZoomMode) {
        m_zoomMode = loadedZoomMode;
        Q_EMIT zoomModeChanged();
    }
    if (!approximatelyEqual(m_zoomPercent, loadedZoomPercent)) {
        m_zoomPercent = loadedZoomPercent;
        Q_EMIT zoomPercentChanged();
    }
    m_zoomContainerUrl = loadedContainerUrl;
    setDisplayedSvgImage(std::move(data), intrinsicSize, image, rasterSize);
    setDisplaySize(loadedDisplaySize);
    finishSuccessfulImageLoad();
}

void KiriImageView::prepareSuccessfulImageLoad()
{
    stopAnimation();
    const QUrl loadedContainerUrl = containerNavigationUrlForImage(m_sourceUrl, m_comicBookRootUrl);
    if (!sameContainerNavigationUrl(loadedContainerUrl, m_zoomContainerUrl)) {
        setZoomMode(ZoomMode::Fit);
    }
    m_zoomContainerUrl = loadedContainerUrl;
}

void KiriImageView::finishSuccessfulImageLoad()
{
    m_displayedUrl = m_sourceUrl;
    m_displayedComicBookRootUrl = m_comicBookRootUrl;
    updateWindowTitleFileName();
    if (!m_loadingContainerNavigationUrl.isEmpty()) {
        setContainerNavigationUrl(m_loadingContainerNavigationUrl);
        m_loadingContainerNavigationUrl = QUrl();
    } else {
        updateContainerNavigationFromDisplayedImage();
    }
    setErrorString(QString());
    setLoading(false);
    setStatus(Status::Ready);
    updatePageNavigation();
}

void KiriImageView::startAnimation(
    const QByteArray &data, const QByteArray &format, int loopCount, int firstFrameDelay)
{
    m_animationData = data;
    m_animationFormat = format;
    m_animationLoopCount = loopCount;
    m_completedAnimationLoops = 0;

    QString errorString;
    if (!resetAnimationReader(&errorString)) {
        finishWithAnimationError(errorString);
        return;
    }

    const QImage firstFrame = m_animationReader->read();
    if (firstFrame.isNull()) {
        finishWithAnimationError(m_animationReader->errorString());
        return;
    }

    if (m_animationReader->canRead()) {
        m_animationTimer.start(normalizedAnimationFrameDelay(firstFrameDelay));
    }
}

void KiriImageView::startDecodedAnimation(
    std::vector<KiriView::AnimationFrame> frames, int loopCount)
{
    m_decodedAnimationFrames = std::move(frames);
    m_decodedAnimationFrameIndex = 0;
    m_animationLoopCount = loopCount;
    m_completedAnimationLoops = 0;

    if (m_decodedAnimationFrames.size() > 1) {
        m_animationTimer.start(
            normalizedAnimationFrameDelay(m_decodedAnimationFrames.front().delay));
    }
}

void KiriImageView::advanceAnimationFrame()
{
    if (!m_decodedAnimationFrames.empty()) {
        advanceDecodedAnimationFrame();
        return;
    }

    if (m_animationReader == nullptr) {
        return;
    }

    if (!m_animationReader->canRead()) {
        if (!hasRemainingAnimationLoops()) {
            stopAnimation();
            return;
        }

        ++m_completedAnimationLoops;

        QString errorString;
        if (!resetAnimationReader(&errorString)) {
            finishWithAnimationError(errorString);
            return;
        }
    }

    QImage frame = m_animationReader->read();
    if (frame.isNull()) {
        finishWithAnimationError(m_animationReader->errorString());
        return;
    }

    const int delay = normalizedAnimationFrameDelay(m_animationReader->nextImageDelay());
    setDisplayedImage(frame);

    if (m_animationReader->canRead() || hasRemainingAnimationLoops()) {
        m_animationTimer.start(delay);
    } else {
        stopAnimation();
    }
}

void KiriImageView::advanceDecodedAnimationFrame()
{
    if (m_decodedAnimationFrames.empty()) {
        return;
    }

    if (m_decodedAnimationFrameIndex + 1 >= m_decodedAnimationFrames.size()) {
        if (!hasRemainingAnimationLoops()) {
            stopAnimation();
            return;
        }

        ++m_completedAnimationLoops;
        m_decodedAnimationFrameIndex = 0;
    } else {
        ++m_decodedAnimationFrameIndex;
    }

    const KiriView::AnimationFrame &frame
        = m_decodedAnimationFrames.at(m_decodedAnimationFrameIndex);
    setDisplayedImage(frame.image);

    if (m_decodedAnimationFrameIndex + 1 < m_decodedAnimationFrames.size()
        || hasRemainingAnimationLoops()) {
        m_animationTimer.start(normalizedAnimationFrameDelay(frame.delay));
    } else {
        stopAnimation();
    }
}

bool KiriImageView::resetAnimationReader(QString *errorString)
{
    m_animationReader.reset();
    m_animationBuffer.reset();

    auto buffer = std::make_unique<QBuffer>();
    buffer->setData(m_animationData);

    if (!buffer->open(QIODevice::ReadOnly)) {
        *errorString = tr("Could not read the selected image data.");
        return false;
    }

    auto reader = std::make_unique<QImageReader>(buffer.get(), m_animationFormat);
    reader->setAutoTransform(true);

    if (!reader->canRead()) {
        *errorString = reader->errorString();
        return false;
    }

    m_animationBuffer = std::move(buffer);
    m_animationReader = std::move(reader);
    return true;
}

bool KiriImageView::hasRemainingAnimationLoops() const
{
    return m_animationLoopCount < 0 || m_completedAnimationLoops < m_animationLoopCount;
}

bool KiriImageView::hasDisplayedImage() const { return !m_image.isNull(); }

void KiriImageView::stopAnimation()
{
    m_animationTimer.stop();
    m_animationReader.reset();
    m_animationBuffer.reset();
    m_animationData.clear();
    m_animationFormat.clear();
    m_decodedAnimationFrames.clear();
    m_decodedAnimationFrameIndex = 0;
    m_animationLoopCount = 0;
    m_completedAnimationLoops = 0;
}

void KiriImageView::finishWithAnimationError(const QString &errorString)
{
    setLoading(false);
    clearImage();
    setZoomMode(ZoomMode::Fit);
    updateZoomState();
    setContainerNavigationUrl(QUrl());
    clearPageNavigation();
    const QString message = errorString.isEmpty()
        ? tr("Could not decode the selected image animation.")
        : errorString;
    setErrorString(message);
    setStatus(Status::Error);
}

void KiriImageView::setDisplayedImage(const QImage &image)
{
    clearDisplayedSvgImage();
    m_image = displayReadyImage(image);
    ++m_imageRevision;
    setImageSize(m_image.size());
    update();
}

void KiriImageView::setDisplayedSvgImage(
    QByteArray data, const QSize &intrinsicSize, const QImage &image, const QSize &rasterSize)
{
    m_displayedImageIsSvg = true;
    m_svgData = std::move(data);
    m_svgIntrinsicSize = intrinsicSize;
    m_svgRasterSize = rasterSize;
    m_image = displayReadyImage(image);
    ++m_imageRevision;
    setImageSize(intrinsicSize);
    update();
}

void KiriImageView::clearDisplayedSvgImage()
{
    m_displayedImageIsSvg = false;
    m_svgData.clear();
    m_svgIntrinsicSize = QSize();
    m_svgRasterSize = QSize();
}

bool KiriImageView::updateDisplayedSvgRaster()
{
    if (!m_displayedImageIsSvg) {
        return true;
    }

    const QSize rasterSize
        = svgRasterSize(m_displaySize, displayDevicePixelRatio(), maximumTextureSize());
    if (rasterSize.isEmpty()) {
        return false;
    }
    if (!m_image.isNull() && m_svgRasterSize == rasterSize) {
        return true;
    }

    const QImage image = renderSvgImage(m_svgData, rasterSize);
    if (image.isNull()) {
        return false;
    }

    m_image = displayReadyImage(image);
    m_svgRasterSize = rasterSize;
    ++m_imageRevision;
    update();
    return true;
}

void KiriImageView::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }

    m_loading = loading;
    Q_EMIT loadingChanged();
}

void KiriImageView::setStatus(Status status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    Q_EMIT statusChanged();
}

void KiriImageView::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString) {
        return;
    }

    m_errorString = errorString;
    Q_EMIT errorStringChanged();
}

void KiriImageView::setWindowTitleFileName(const QString &fileName)
{
    if (m_windowTitleFileName == fileName) {
        return;
    }

    m_windowTitleFileName = fileName;
    Q_EMIT windowTitleFileNameChanged();
}

void KiriImageView::updateWindowTitleFileName()
{
    setWindowTitleFileName(
        windowTitleFileNameForDisplayedUrl(m_displayedUrl, m_displayedComicBookRootUrl));
}

void KiriImageView::setImageSize(const QSize &imageSize)
{
    if (m_imageSize == imageSize) {
        return;
    }

    m_imageSize = imageSize;
    Q_EMIT imageSizeChanged();
    updateZoomState();
}

void KiriImageView::setDisplaySize(const QSizeF &displaySize)
{
    const QSizeF normalizedDisplaySize(
        std::max<qreal>(0.0, displaySize.width()), std::max<qreal>(0.0, displaySize.height()));
    if (approximatelyEqual(m_displaySize, normalizedDisplaySize)) {
        return;
    }

    m_displaySize = normalizedDisplaySize;
    Q_EMIT displaySizeChanged();
    updateDisplayedSvgRaster();
    update();
}

void KiriImageView::setZoomMode(ZoomMode zoomMode)
{
    if (m_zoomMode == zoomMode) {
        return;
    }

    m_zoomMode = zoomMode;
    Q_EMIT zoomModeChanged();
}

void KiriImageView::updateZoomState()
{
    if (m_zoomMode != ZoomMode::Manual) {
        const qreal zoomPercent = fitZoomPercent(m_zoomMode);
        if (!approximatelyEqual(m_zoomPercent, zoomPercent)) {
            m_zoomPercent = zoomPercent;
            Q_EMIT zoomPercentChanged();
        }
    }

    setDisplaySize(displaySizeForZoomPercent(m_zoomPercent));
    updateDisplayedSvgRaster();
}

qreal KiriImageView::displayDevicePixelRatio() const
{
    const QQuickWindow *quickWindow = window();
    if (quickWindow == nullptr) {
        return 1.0;
    }

    const qreal devicePixelRatio = quickWindow->effectiveDevicePixelRatio();
    if (!std::isfinite(devicePixelRatio) || devicePixelRatio <= 0.0) {
        return 1.0;
    }

    return devicePixelRatio;
}

int KiriImageView::maximumTextureSize() const
{
    const QQuickWindow *quickWindow = window();
    QRhi *rhi = quickWindow == nullptr ? nullptr : quickWindow->rhi();
    if (rhi == nullptr) {
        return KiriView::fallbackTextureSizeMax;
    }

    const int maximumTextureSize = rhi->resourceLimit(QRhi::TextureSizeMax);
    return maximumTextureSize > 0 ? maximumTextureSize : KiriView::fallbackTextureSizeMax;
}

qreal KiriImageView::fitZoomPercent(ZoomMode zoomMode) const
{
    return fitZoomPercentForImageSize(zoomMode, m_imageSize);
}

qreal KiriImageView::fitZoomPercentForImageSize(ZoomMode zoomMode, const QSize &imageSize) const
{
    const qreal devicePixelRatio = displayDevicePixelRatio();
    const qreal fallbackZoomPercent = devicePixelRatio * 100.0;

    if (imageSize.isEmpty()) {
        return fallbackZoomPercent;
    }

    const auto fitWidthZoomPercent = [this, devicePixelRatio, fallbackZoomPercent, imageSize]() {
        if (m_viewportSize.width() <= 0.0) {
            return fallbackZoomPercent;
        }

        return std::max<qreal>(
            0.0, m_viewportSize.width() * devicePixelRatio * 100.0 / imageSize.width());
    };
    const auto fitHeightZoomPercent = [this, devicePixelRatio, fallbackZoomPercent, imageSize]() {
        if (m_viewportSize.height() <= 0.0) {
            return fallbackZoomPercent;
        }

        return std::max<qreal>(
            0.0, m_viewportSize.height() * devicePixelRatio * 100.0 / imageSize.height());
    };

    switch (zoomMode) {
    case ZoomMode::Fit:
        if (m_viewportSize.isEmpty()) {
            return fallbackZoomPercent;
        }
        return std::min(fitWidthZoomPercent(), fitHeightZoomPercent());
    case ZoomMode::FitHeight:
        return fitHeightZoomPercent();
    case ZoomMode::FitWidth:
        return fitWidthZoomPercent();
    case ZoomMode::Manual:
        return m_zoomPercent;
    }

    return fallbackZoomPercent;
}

QSizeF KiriImageView::displaySizeForZoomPercent(qreal zoomPercent) const
{
    return displaySizeForZoomPercent(zoomPercent, m_imageSize);
}

QSizeF KiriImageView::displaySizeForZoomPercent(qreal zoomPercent, const QSize &imageSize) const
{
    if (imageSize.isEmpty() || !std::isfinite(zoomPercent) || zoomPercent <= 0.0) {
        return {};
    }

    const qreal scale = zoomPercent / (displayDevicePixelRatio() * 100.0);
    return QSizeF(imageSize.width() * scale, imageSize.height() * scale);
}

void KiriImageView::clearImage()
{
    cancelPredecode();
    cancelPageNavigationUpdate();
    m_predecodeWindowUrls.clear();
    m_predecodedImages.clear();
    stopAnimation();
    m_displayedImageIsPredecodeCacheable = false;
    m_displayedUrl = QUrl();
    m_displayedComicBookRootUrl = QUrl();
    m_zoomContainerUrl = QUrl();
    clearDisplayedSvgImage();
    updateWindowTitleFileName();
    clearPageNavigation();

    if (!m_image.isNull()) {
        m_image = QImage();
        ++m_imageRevision;
        update();
    }

    setImageSize(QSize());
}
