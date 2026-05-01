// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimageview.h"

#include "imageanimationplayer.h"
#include "kiriimagedecoder.h"
#include "kiriimagenavigation.h"
#include "kiriimagerendernode.h"
#include "predecodecache.h"

#include <KCoreDirLister>
#include <KIO/Job>
#include <KIO/ListJob>
#include <KIO/StoredTransferJob>
#include <KIO/UDSEntry>
#include <KJob>
#include <QByteArray>
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
constexpr qreal minimumManualZoomPercent = 10.0;
constexpr qreal maximumManualZoomPercent = 800.0;

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

bool approximatelyEqual(qreal left, qreal right) { return std::abs(left - right) < 0.001; }

bool approximatelyEqual(const QSizeF &left, const QSizeF &right)
{
    return approximatelyEqual(left.width(), right.width())
        && approximatelyEqual(left.height(), right.height());
}

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

quint64 KiriImageView::AsyncObjectSlot::start(QObject *object, CancelCallback cancelCallback)
{
    cancel();
    m_object = object;
    m_cancelCallback = std::move(cancelCallback);
    ++m_token;
    return m_token;
}

bool KiriImageView::AsyncObjectSlot::accepts(quint64 token, const QObject *object) const
{
    return token == m_token && object == m_object;
}

bool KiriImageView::AsyncObjectSlot::claim(quint64 token, QObject *object)
{
    if (!accepts(token, object)) {
        return false;
    }

    m_object = nullptr;
    m_cancelCallback = {};
    return true;
}

void KiriImageView::AsyncObjectSlot::clear(QObject *object)
{
    if (object != m_object) {
        return;
    }

    m_object = nullptr;
    m_cancelCallback = {};
}

void KiriImageView::AsyncObjectSlot::cancel()
{
    if (m_object == nullptr) {
        return;
    }

    QObject *object = m_object;
    CancelCallback cancelCallback = std::move(m_cancelCallback);
    m_object = nullptr;
    ++m_token;
    if (cancelCallback) {
        cancelCallback(object);
    }
}

class KiriImageView::PredecodeCoordinator
{
public:
    struct Context {
        QUrl displayedUrl;
        QUrl comicBookRootUrl;
        bool displayedImageIsCacheable = false;
        QImage displayedImage;
    };

    explicit PredecodeCoordinator(KiriImageView *view)
        : m_view(view)
    {
    }

    void schedule(Context context);
    void cancel();
    void clear();
    bool tryTake(const QUrl &url, QImage *image, QUrl *comicBookRootUrl) const;

private:
    void scheduleFileAdjacentImagePredecode(const Context &context, quint64 generation);
    void scheduleComicBookAdjacentImagePredecode(const Context &context, quint64 generation);
    void startPredecodeImageLoads(const std::vector<QUrl> &urls, const QUrl &comicBookRootUrl,
        const Context &context, quint64 generation);
    void startNextPredecodeImageLoad(quint64 generation);
    void startPredecodeImageLoad(const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation);
    void startPredecodeImageDecode(
        QByteArray data, const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation);

    KiriImageView *m_view = nullptr;
    KiriImageView::AsyncObjectSlot m_listerSlot;
    KiriImageView::AsyncObjectSlot m_listJobSlot;
    KiriImageView::AsyncObjectSlot m_imageLoadSlot;
    KiriView::PredecodeCache m_cache;
    QUrl m_activePredecodeUrl;
    quint64 m_generation = 0;
};

KiriImageView::KiriImageView(QQuickItem *parent)
    : QQuickItem(parent)
{
    auto frameReady = [this](const QImage &image) { setDisplayedImage(image); };
    auto animationError
        = [this](const QString &errorString) { finishWithAnimationError(errorString); };
    m_animationPlayer = std::make_unique<KiriView::ImageAnimationPlayer>(
        this, std::move(frameReady), std::move(animationError));
    m_predecodeCoordinator = std::make_unique<PredecodeCoordinator>(this);
    setFlag(ItemHasContents, true);
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
    setErrorString(QString());

    if (m_sourceUrl.isEmpty()) {
        clearImage();
        setZoomMode(ZoomMode::Fit);
        updateZoomState();
        setLoading(false);
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

    LoadSession session;
    session.id = ++m_nextLoadSessionId;
    session.requestedSourceUrl = m_sourceUrl;
    session.imageUrl = m_sourceUrl;
    session.containerNavigationUrl = m_loadingContainerNavigationUrl;

    const std::optional<QUrl> selectedArchiveRootUrl = comicBookArchiveRootUrl(m_sourceUrl);
    if (selectedArchiveRootUrl.has_value()) {
        session.comicBookRootUrl = selectedArchiveRootUrl.value();
        m_loadSession = session;
        startComicBookLoad(session);
        return;
    }

    if (isUrlInsideArchiveRoot(m_sourceUrl, m_displayedComicBookRootUrl)) {
        session.comicBookRootUrl = m_displayedComicBookRootUrl;
    } else {
        const std::optional<QUrl> containingArchiveRootUrl
            = containingComicBookArchiveRootUrl(m_sourceUrl);
        session.comicBookRootUrl = containingArchiveRootUrl.has_value()
                && isUrlInsideArchiveRoot(m_sourceUrl, containingArchiveRootUrl.value())
            ? containingArchiveRootUrl.value()
            : QUrl();
    }

    m_loadSession = session;
    if (tryDisplayPredecodedImage(session)) {
        return;
    }

    startImageLoad(session);
}

void KiriImageView::startImageLoad(LoadSession session)
{
    auto *job = KIO::storedGet(session.imageUrl, KIO::NoReload, KIO::HideProgressInfo);
    const quint64 token = m_imageLoadSlot.start(job, cancelKJob);

    connect(job, &KJob::result, this, [this, job, token, session](KJob *finishedJob) {
        if (!m_imageLoadSlot.claim(token, job) || !isCurrentLoadSession(session)) {
            return;
        }

        if (finishedJob->error() != KJob::NoError) {
            finishLoadWithError(session, finishedJob->errorString());
            return;
        }

        startImageDecode(job->data(), session);
    });

    connect(job, &QObject::destroyed, this, [this, job]() { m_imageLoadSlot.clear(job); });
}

void KiriImageView::startComicBookLoad(LoadSession session)
{
    auto *job = KIO::listRecursive(
        session.comicBookRootUrl, KIO::HideProgressInfo, recursiveImageListFlags());
    auto candidates = collectArchiveImageCandidates(job, this, session.comicBookRootUrl);
    const quint64 token = m_archiveListSlot.start(job, cancelKJob);

    connect(job, &KJob::result, this,
        [this, job, token, session, candidates](KJob *finishedJob) mutable {
            if (!m_archiveListSlot.claim(token, job) || !isCurrentLoadSession(session)) {
                return;
            }

            if (finishedJob->error() != KJob::NoError) {
                finishLoadWithError(session, finishedJob->errorString());
                return;
            }

            sortImageNavigationCandidates(candidates.get());
            if (candidates->empty()) {
                finishLoadWithError(session,
                    tr("The selected comic book archive does not contain any supported images."));
                return;
            }

            session.imageUrl = candidates->front().url;
            m_loadSession = session;
            setSourceUrlFromResolvedLoad(session.imageUrl);
            startImageLoad(session);
        });

    connect(job, &QObject::destroyed, this, [this, job]() { m_archiveListSlot.clear(job); });
}

void KiriImageView::cancelLoad()
{
    m_loadSession.reset();
    m_imageLoadSlot.cancel();
    m_archiveListSlot.cancel();
}

bool KiriImageView::isCurrentLoadSession(const LoadSession &session) const
{
    return m_loadSession.has_value() && m_loadSession->id == session.id;
}

void KiriImageView::clearLoadSession(const LoadSession &session)
{
    if (isCurrentLoadSession(session)) {
        m_loadSession.reset();
    }
}

void KiriImageView::setSourceUrlFromResolvedLoad(const QUrl &sourceUrl)
{
    if (m_sourceUrl == sourceUrl) {
        return;
    }

    m_sourceUrl = sourceUrl;
    Q_EMIT sourceUrlChanged();
}

void KiriImageView::openAdjacentImage(NavigationDirection direction)
{
    if (!hasDisplayedImage() || m_displayedUrl.isEmpty()) {
        return;
    }

    cancelContainerNavigation();

    if (isUrlInsideArchiveRoot(m_displayedUrl, m_displayedComicBookRootUrl)) {
        openAdjacentComicBookImage(direction);
        return;
    }

    const QUrl currentUrl = navigationSourceUrl(m_displayedUrl);
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

void KiriImageView::openAdjacentComicBookImage(NavigationDirection direction)
{
    const QUrl currentUrl = m_displayedUrl.adjusted(QUrl::NormalizePathSegments);
    const QUrl archiveRootUrl = m_displayedComicBookRootUrl;
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

    connect(job, &QObject::destroyed, this, [this, job]() { m_navigationListSlot.clear(job); });
}

void KiriImageView::cancelNavigation()
{
    m_navigationListerSlot.cancel();
    m_navigationListSlot.cancel();
}

void KiriImageView::finishNavigation(KCoreDirLister *lister, quint64 generation,
    NavigationDirection direction, const QUrl &currentUrl)
{
    if (!m_navigationListerSlot.claim(generation, lister)) {
        return;
    }

    const std::vector<ImageNavigationCandidate> candidates = imageCandidatesFromLister(lister);
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
    if (!m_navigationListerSlot.claim(generation, lister)) {
        return;
    }

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

void KiriImageView::cancelContainerNavigation()
{
    m_containerNavigationListerSlot.cancel();
    m_containerNavigationListSlot.cancel();
}

void KiriImageView::finishContainerNavigation(KCoreDirLister *lister, quint64 generation,
    NavigationDirection direction, const QUrl &currentContainerUrl)
{
    if (!m_containerNavigationListerSlot.claim(generation, lister)) {
        return;
    }

    const std::vector<ContainerNavigationCandidate> candidates
        = containerNavigationCandidates(lister->items(KCoreDirLister::AllItems));

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
        openDirectoryContainer(target->url);
    } else {
        openComicBookContainer(target->url);
    }
}

void KiriImageView::finishContainerNavigationWithError(KCoreDirLister *lister, quint64 generation)
{
    if (!m_containerNavigationListerSlot.claim(generation, lister)) {
        return;
    }

    lister->deleteLater();
}

void KiriImageView::openDirectoryContainer(const QUrl &containerUrl)
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

void KiriImageView::finishDirectoryContainerNavigation(
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

void KiriImageView::finishDirectoryContainerNavigationWithError(KCoreDirLister *lister,
    quint64 generation, const QUrl &containerUrl, const QString &errorString)
{
    if (!m_containerNavigationListerSlot.claim(generation, lister)) {
        return;
    }

    lister->deleteLater();
    finishContainerNavigationLoadWithError(containerUrl, errorString);
}

void KiriImageView::openComicBookContainer(const QUrl &containerUrl)
{
    const std::optional<QUrl> archiveRootUrl = comicBookArchiveRootUrl(containerUrl);
    if (!archiveRootUrl.has_value()) {
        finishContainerNavigationLoadWithError(
            containerUrl, tr("Could not open the selected comic book archive."));
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

    connect(job, &QObject::destroyed, this,
        [this, job]() { m_containerNavigationListSlot.clear(job); });
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

    if (isComicBookPage) {
        updateComicBookPageNavigation(currentUrl, m_displayedComicBookRootUrl);
        return;
    }

    updateFilePageNavigation(currentUrl);
}

void KiriImageView::updateFilePageNavigation(const QUrl &currentUrl)
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

void KiriImageView::updateComicBookPageNavigation(
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

void KiriImageView::cancelPageNavigationUpdate()
{
    m_pageNavigationListerSlot.cancel();
    m_pageNavigationListSlot.cancel();
}

void KiriImageView::finishPageNavigationUpdateWithError(KCoreDirLister *lister, quint64 generation)
{
    if (!m_pageNavigationListerSlot.claim(generation, lister)) {
        return;
    }

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

void KiriImageView::PredecodeCoordinator::schedule(Context context)
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

void KiriImageView::PredecodeCoordinator::scheduleFileAdjacentImagePredecode(
    const Context &context, quint64 generation)
{
    const QUrl currentUrl = navigationSourceUrl(context.displayedUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!parentUrl.isValid() || parentUrl.isEmpty()) {
        startPredecodeImageLoads({}, QUrl(), context, generation);
        return;
    }

    auto *lister = createImageCandidateLister(m_view);
    const quint64 token = m_listerSlot.start(lister, cancelDirLister);

    QObject::connect(lister, &KCoreDirLister::completed, m_view,
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
    QObject::connect(lister, &KCoreDirLister::jobError, m_view,
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

void KiriImageView::PredecodeCoordinator::scheduleComicBookAdjacentImagePredecode(
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
    auto candidates = collectArchiveImageCandidates(job, m_view, archiveRootUrl);
    const quint64 token = m_listJobSlot.start(job, cancelKJob);

    QObject::connect(job, &KJob::result, m_view,
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

    QObject::connect(job, &QObject::destroyed, m_view, [this, job]() { m_listJobSlot.clear(job); });
}

void KiriImageView::PredecodeCoordinator::startPredecodeImageLoads(const std::vector<QUrl> &urls,
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

void KiriImageView::PredecodeCoordinator::startNextPredecodeImageLoad(quint64 generation)
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

void KiriImageView::PredecodeCoordinator::startPredecodeImageLoad(
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

    QObject::connect(job, &KJob::result, m_view,
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

    QObject::connect(
        job, &QObject::destroyed, m_view, [this, job, token, generation, normalizedUrl]() {
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

void KiriImageView::PredecodeCoordinator::startPredecodeImageDecode(
    QByteArray data, const QUrl &url, const QUrl &comicBookRootUrl, quint64 generation)
{
    const QPointer<KiriImageView> view(m_view);
    QThreadPool::globalInstance()->start(QRunnable::create(
        [view, data = std::move(data), url, comicBookRootUrl, generation]() mutable {
            auto result = std::make_shared<DecodedImageResult>(decodeImageData(data));
            if (view == nullptr) {
                return;
            }

            QMetaObject::invokeMethod(
                view.data(),
                [view, generation, url, comicBookRootUrl, result]() {
                    if (view == nullptr || view->m_predecodeCoordinator == nullptr) {
                        return;
                    }

                    auto *coordinator = view->m_predecodeCoordinator.get();
                    if (generation != coordinator->m_generation) {
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

void KiriImageView::PredecodeCoordinator::cancel()
{
    ++m_generation;
    m_listerSlot.cancel();
    m_listJobSlot.cancel();
    m_imageLoadSlot.cancel();
    m_activePredecodeUrl = QUrl();
    m_cache.clearQueuedLoads();
}

void KiriImageView::PredecodeCoordinator::clear()
{
    cancel();
    m_cache.clear();
}

bool KiriImageView::PredecodeCoordinator::tryTake(
    const QUrl &url, QImage *image, QUrl *comicBookRootUrl) const
{
    return m_cache.findImage(url, image, comicBookRootUrl);
}

void KiriImageView::scheduleAdjacentImagePredecode()
{
    if (!hasDisplayedImage() || m_displayedUrl.isEmpty()) {
        cancelPredecode();
        return;
    }

    m_predecodeCoordinator->schedule(PredecodeCoordinator::Context { m_displayedUrl,
        m_displayedComicBookRootUrl, m_displayedImageIsPredecodeCacheable, m_image });
}

void KiriImageView::cancelPredecode()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->cancel();
    }
}

bool KiriImageView::tryDisplayPredecodedImage(LoadSession session)
{
    QImage image;
    QUrl comicBookRootUrl;
    if (m_predecodeCoordinator == nullptr
        || !m_predecodeCoordinator->tryTake(session.imageUrl, &image, &comicBookRootUrl)) {
        return false;
    }

    session.comicBookRootUrl = comicBookRootUrl;
    m_loadSession = session;
    finishLoadSuccessfully(session, image, true);
    scheduleAdjacentImagePredecode();
    return true;
}

void KiriImageView::startImageDecode(QByteArray data, LoadSession session)
{
    const QPointer<KiriImageView> view(this);
    QThreadPool::globalInstance()->start(
        QRunnable::create([view, data = std::move(data), session]() mutable {
            auto result = std::make_shared<DecodedImageResult>(decodeImageData(data));
            if (view == nullptr) {
                return;
            }

            QMetaObject::invokeMethod(
                view.data(),
                [view, session, result]() mutable {
                    if (view == nullptr || !view->isCurrentLoadSession(session)) {
                        return;
                    }

                    if (!result->success) {
                        view->finishLoadWithError(session, result->errorString);
                        return;
                    }

                    if (result->isSvg) {
                        view->finishSvgLoadSuccessfully(
                            session, std::move(result->svgData), result->svgIntrinsicSize);
                        view->scheduleAdjacentImagePredecode();
                        return;
                    }

                    const bool predecodeCacheable = decodedImageResultIsPredecodeCacheable(
                        *result, KiriView::PredecodeCache::byteBudget());
                    view->finishLoadSuccessfully(session, result->image, predecodeCacheable);
                    if (!result->decodedAnimationFrames.empty()) {
                        view->m_animationPlayer->startDecoded(
                            std::move(result->decodedAnimationFrames), result->animationLoopCount);
                    } else if (result->hasAnimationReaderFrames) {
                        view->m_animationPlayer->start(result->animationData,
                            result->animationFormat, result->animationLoopCount,
                            result->firstFrameDelay);
                    }
                    view->scheduleAdjacentImagePredecode();
                },
                Qt::QueuedConnection);
        }));
}

void KiriImageView::finishLoadWithError(const LoadSession &session, const QString &errorString)
{
    if (!isCurrentLoadSession(session)) {
        return;
    }

    const QUrl containerNavigationUrl = session.containerNavigationUrl;
    clearLoadSession(session);
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

void KiriImageView::finishLoadSuccessfully(
    const LoadSession &session, const QImage &image, bool predecodeCacheable)
{
    if (!isCurrentLoadSession(session)) {
        return;
    }

    prepareSuccessfulImageLoad(session);
    m_displayedImageIsPredecodeCacheable = predecodeCacheable;
    setDisplayedImage(image);
    updateZoomState();
    finishSuccessfulImageLoad(session);
}

void KiriImageView::finishSvgLoadSuccessfully(
    LoadSession session, QByteArray data, const QSize &intrinsicSize)
{
    if (!isCurrentLoadSession(session)) {
        return;
    }

    if (data.isEmpty() || intrinsicSize.isEmpty()) {
        finishLoadWithError(session, tr("Could not decode the selected SVG image."));
        return;
    }

    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
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
        finishLoadWithError(session, tr("Could not render the selected SVG image."));
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
    finishSuccessfulImageLoad(session);
}

void KiriImageView::prepareSuccessfulImageLoad(const LoadSession &session)
{
    stopAnimation();
    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
    if (!sameContainerNavigationUrl(loadedContainerUrl, m_zoomContainerUrl)) {
        setZoomMode(ZoomMode::Fit);
    }
    m_zoomContainerUrl = loadedContainerUrl;
}

void KiriImageView::finishSuccessfulImageLoad(const LoadSession &session)
{
    if (!isCurrentLoadSession(session)) {
        return;
    }

    setSourceUrlFromResolvedLoad(session.imageUrl);
    m_displayedUrl = session.imageUrl;
    m_displayedComicBookRootUrl = session.comicBookRootUrl;
    updateWindowTitleFileName();
    if (!session.containerNavigationUrl.isEmpty()) {
        setContainerNavigationUrl(session.containerNavigationUrl);
    } else {
        updateContainerNavigationFromDisplayedImage();
    }
    clearLoadSession(session);
    m_loadingContainerNavigationUrl = QUrl();
    setErrorString(QString());
    setLoading(false);
    setStatus(Status::Ready);
    updatePageNavigation();
}

bool KiriImageView::hasDisplayedImage() const { return !m_image.isNull(); }

void KiriImageView::stopAnimation() { m_animationPlayer->stop(); }

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
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->clear();
    }
    cancelPageNavigationUpdate();
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
