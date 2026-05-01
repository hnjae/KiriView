// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "kiriimageview.h"

#include "imageanimationplayer.h"
#include "imageformatregistry.h"
#include "imagenavigationservice.h"
#include "imagepredecodecoordinator.h"
#include "kiriimagedecoder.h"
#include "kiriimagerendernode.h"
#include "predecodecache.h"

#include <QByteArray>
#include <QQuickWindow>
#include <Qt>
#include <cmath>
#include <memory>
#include <optional>
#include <rhi/qrhi.h>
#include <utility>

namespace {
using KiriView::containerNavigationUrlForImage;
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::displayReadyImage;
using KiriView::imageZoomApproximatelyEqual;
using KiriView::ImageZoomMode;
using KiriView::NavigationDirection;
using KiriView::renderSvgImage;
using KiriView::svgRasterSize;
using KiriView::windowTitleFileNameForDisplayedUrl;

ImageZoomMode toImageZoomMode(KiriImageView::ZoomMode zoomMode)
{
    switch (zoomMode) {
    case KiriImageView::ZoomMode::Fit:
        return ImageZoomMode::Fit;
    case KiriImageView::ZoomMode::FitHeight:
        return ImageZoomMode::FitHeight;
    case KiriImageView::ZoomMode::FitWidth:
        return ImageZoomMode::FitWidth;
    case KiriImageView::ZoomMode::Manual:
        return ImageZoomMode::Manual;
    }

    return ImageZoomMode::Fit;
}

KiriImageView::ZoomMode fromImageZoomMode(ImageZoomMode zoomMode)
{
    switch (zoomMode) {
    case ImageZoomMode::Fit:
        return KiriImageView::ZoomMode::Fit;
    case ImageZoomMode::FitHeight:
        return KiriImageView::ZoomMode::FitHeight;
    case ImageZoomMode::FitWidth:
        return KiriImageView::ZoomMode::FitWidth;
    case ImageZoomMode::Manual:
        return KiriImageView::ZoomMode::Manual;
    }

    return KiriImageView::ZoomMode::Fit;
}
}

KiriImageView::KiriImageView(QQuickItem *parent)
    : QQuickItem(parent)
{
    auto frameReady = [this](const QImage &image) { setDisplayedImage(image); };
    auto animationError
        = [this](const QString &errorString) { finishWithAnimationError(errorString); };
    m_animationPlayer = std::make_unique<KiriView::ImageAnimationPlayer>(
        this, std::move(frameReady), std::move(animationError));
    m_navigationService = std::make_unique<KiriView::ImageNavigationService>(this);
    m_navigationService->setOpenUrlCallback([this](const QUrl &url) { setSourceUrl(url); });
    m_navigationService->setOpenContainerImageCallback(
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            openImageFromContainerNavigation(imageUrl, containerUrl);
        });
    m_navigationService->setContainerNavigationErrorCallback(
        [this](const QUrl &containerUrl, KiriView::ContainerNavigationError error,
            const QString &errorString) {
            if (error == KiriView::ContainerNavigationError::EmptyContainer) {
                finishContainerNavigationWithEmptyContainer(containerUrl);
                return;
            }

            if (error == KiriView::ContainerNavigationError::InvalidComicBookArchive) {
                finishContainerNavigationLoadWithError(
                    containerUrl, tr("Could not open the selected comic book archive."));
                return;
            }

            finishContainerNavigationLoadWithError(containerUrl, errorString);
        });
    m_navigationService->setPageNavigationChangedCallback(
        [this]() { Q_EMIT pageNavigationChanged(); });
    m_imageLoader = std::make_unique<KiriView::ImageLoader>(this);
    m_imageLoader->setSourceResolvedCallback(
        [this](const QUrl &sourceUrl) { setSourceUrlFromResolvedLoad(sourceUrl); });
    m_imageLoader->setErrorCallback(
        [this](const KiriView::ImageLoadSession &session, KiriView::ImageLoadError error,
            const QString &errorString) { finishLoadWithError(session, error, errorString); });
    m_imageLoader->setDecodedImageCallback(
        [this](KiriView::ImageLoadSession session,
            std::shared_ptr<KiriView::DecodedImageResult> result) {
            finishDecodedImageLoad(std::move(session), std::move(result));
        });
    m_imageLoader->setPredecodedImageCallback(
        [this](KiriView::ImageLoadSession session, const QImage &image) {
            finishPredecodedImageLoad(std::move(session), image);
        });
    m_imageLoader->setTakePredecodedImageCallback(
        [this](const QUrl &url) { return takePredecodedImage(url); });
    m_predecodeCoordinator = std::make_unique<KiriView::ImagePredecodeCoordinator>(this);
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

QSize KiriImageView::imageSize() const { return m_zoomState.imageSize(); }

QSizeF KiriImageView::viewportSize() const { return m_zoomState.viewportSize(); }

void KiriImageView::setViewportSize(const QSizeF &viewportSize)
{
    const KiriView::ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setViewportSize(viewportSize, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
}

QSizeF KiriImageView::displaySize() const { return m_zoomState.displaySize(); }

qreal KiriImageView::zoomPercent() const { return m_zoomState.zoomPercent(); }

void KiriImageView::setZoomPercent(qreal zoomPercent)
{
    const KiriView::ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setManualZoomPercent(zoomPercent, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
}

KiriImageView::ZoomMode KiriImageView::zoomMode() const
{
    return fromImageZoomMode(m_zoomState.zoomMode());
}

QStringList KiriImageView::openDialogNameFilters() const
{
    return KiriView::openDialogNameFilters();
}

int KiriImageView::currentPageNumber() const { return m_navigationService->currentPageNumber(); }

int KiriImageView::imageCount() const { return m_navigationService->imageCount(); }

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
    const std::optional<QUrl> pageUrl = m_navigationService->urlAtPage(pageNumber);
    if (!pageUrl.has_value()) {
        return;
    }

    setSourceUrl(*pageUrl);
}

void KiriImageView::resetZoom()
{
    const KiriView::ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.resetZoom(displayDevicePixelRatio());
    applyZoomStateChanges(previous);
}

void KiriImageView::setFitMode(ZoomMode zoomMode)
{
    if (zoomMode == ZoomMode::Manual) {
        return;
    }

    const KiriView::ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setFitMode(toImageZoomMode(zoomMode), displayDevicePixelRatio())) {
        return;
    }
    applyZoomStateChanges(previous);
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
        resetZoom();
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
        resetZoom();
        setStatus(Status::Loading);
    } else {
        setStatus(Status::Ready);
    }

    m_imageLoader->start(m_sourceUrl, m_displayedComicBookRootUrl, m_loadingContainerNavigationUrl);
}

void KiriImageView::cancelLoad() { m_imageLoader->cancel(); }

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
    m_navigationService->openAdjacentImage(
        KiriView::ImageNavigationService::DisplayContext {
            hasDisplayedImage(), m_displayedUrl, m_displayedComicBookRootUrl },
        direction);
}

void KiriImageView::cancelNavigation() { m_navigationService->cancelNavigation(); }

void KiriImageView::openAdjacentContainer(NavigationDirection direction)
{
    m_navigationService->openAdjacentContainer(m_containerNavigationUrl, direction);
}

void KiriImageView::cancelContainerNavigation()
{
    m_navigationService->cancelContainerNavigation();
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
    const KiriView::ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.clearContainer();
    m_zoomState.prepareImageContainer(containerUrl);
    m_zoomState.resetZoom(displayDevicePixelRatio());
    applyZoomStateChanges(previous);
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
    m_navigationService->updatePageNavigation(KiriView::ImageNavigationService::DisplayContext {
        hasDisplayedImage(), m_displayedUrl, m_displayedComicBookRootUrl });
}

void KiriImageView::cancelPageNavigationUpdate()
{
    m_navigationService->cancelPageNavigationUpdate();
}

void KiriImageView::clearPageNavigation() { m_navigationService->clearPageNavigation(); }

void KiriImageView::scheduleAdjacentImagePredecode()
{
    if (!hasDisplayedImage() || m_displayedUrl.isEmpty()) {
        cancelPredecode();
        return;
    }

    m_predecodeCoordinator->schedule(KiriView::ImagePredecodeCoordinator::Context { m_displayedUrl,
        m_displayedComicBookRootUrl, m_displayedImageIsPredecodeCacheable, m_image });
}

void KiriImageView::cancelPredecode()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->cancel();
    }
}

std::optional<KiriView::PredecodedImage> KiriImageView::takePredecodedImage(const QUrl &url) const
{
    QImage image;
    QUrl comicBookRootUrl;
    if (m_predecodeCoordinator == nullptr
        || !m_predecodeCoordinator->tryTake(url, &image, &comicBookRootUrl)) {
        return std::nullopt;
    }

    return KiriView::PredecodedImage { image, comicBookRootUrl };
}

void KiriImageView::finishPredecodedImageLoad(
    KiriView::ImageLoadSession session, const QImage &image)
{
    finishLoadSuccessfully(session, image, true);
    scheduleAdjacentImagePredecode();
}

void KiriImageView::finishDecodedImageLoad(
    KiriView::ImageLoadSession session, std::shared_ptr<KiriView::DecodedImageResult> result)
{
    if (result->isSvg) {
        finishSvgLoadSuccessfully(
            std::move(session), std::move(result->svgData), result->svgIntrinsicSize);
        scheduleAdjacentImagePredecode();
        return;
    }

    const bool predecodeCacheable
        = decodedImageResultIsPredecodeCacheable(*result, KiriView::PredecodeCache::byteBudget());
    finishLoadSuccessfully(session, result->image, predecodeCacheable);
    if (!result->decodedAnimationFrames.empty()) {
        m_animationPlayer->startDecoded(
            std::move(result->decodedAnimationFrames), result->animationLoopCount);
    } else if (result->hasAnimationReaderFrames) {
        m_animationPlayer->start(result->animationData, result->animationFormat,
            result->animationLoopCount, result->firstFrameDelay);
    }
    scheduleAdjacentImagePredecode();
}

void KiriImageView::finishLoadWithError(const KiriView::ImageLoadSession &session,
    KiriView::ImageLoadError error, const QString &errorString)
{
    const QUrl containerNavigationUrl = session.containerNavigationUrl;
    m_loadingContainerNavigationUrl = QUrl();

    const QString message = error == KiriView::ImageLoadError::EmptyComicBookArchive
        ? tr("The selected comic book archive does not contain any supported images.")
        : errorString;
    if (!containerNavigationUrl.isEmpty()) {
        finishContainerNavigationLoadWithError(containerNavigationUrl, message);
        return;
    }

    setLoading(false);

    if (hasDisplayedImage()) {
        setErrorString(message);
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
    m_zoomState.clearContainer();
    setContainerNavigationUrl(QUrl());
    setErrorString(message);
    setStatus(Status::Error);
}

void KiriImageView::finishLoadSuccessfully(
    const KiriView::ImageLoadSession &session, const QImage &image, bool predecodeCacheable)
{
    prepareSuccessfulImageLoad(session);
    m_displayedImageIsPredecodeCacheable = predecodeCacheable;
    setDisplayedImage(image);
    updateZoomState();
    finishSuccessfulImageLoad(session);
}

void KiriImageView::finishSvgLoadSuccessfully(
    KiriView::ImageLoadSession session, QByteArray data, const QSize &intrinsicSize)
{
    if (data.isEmpty() || intrinsicSize.isEmpty()) {
        finishLoadWithError(session, KiriView::ImageLoadError::Generic,
            tr("Could not decode the selected SVG image."));
        return;
    }

    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
    const KiriView::LoadedImageZoom loadedZoom
        = m_zoomState.loadedImageZoom(loadedContainerUrl, intrinsicSize, displayDevicePixelRatio());
    const QSize rasterSize
        = svgRasterSize(loadedZoom.displaySize, displayDevicePixelRatio(), maximumTextureSize());
    const QImage image = renderSvgImage(data, rasterSize);
    if (image.isNull()) {
        finishLoadWithError(session, KiriView::ImageLoadError::Generic,
            tr("Could not render the selected SVG image."));
        return;
    }

    stopAnimation();
    m_displayedImageIsPredecodeCacheable = false;
    const KiriView::ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.setLoadedSvgZoom(loadedContainerUrl, loadedZoom);
    applyZoomStateChanges(previous);
    setDisplayedSvgImage(std::move(data), intrinsicSize, image, rasterSize);
    finishSuccessfulImageLoad(session);
}

void KiriImageView::prepareSuccessfulImageLoad(const KiriView::ImageLoadSession &session)
{
    stopAnimation();
    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
    const KiriView::ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.prepareImageContainer(loadedContainerUrl);
    applyZoomStateChanges(previous);
}

void KiriImageView::finishSuccessfulImageLoad(const KiriView::ImageLoadSession &session)
{
    setSourceUrlFromResolvedLoad(session.imageUrl);
    m_displayedUrl = session.imageUrl;
    m_displayedComicBookRootUrl = session.comicBookRootUrl;
    updateWindowTitleFileName();
    if (!session.containerNavigationUrl.isEmpty()) {
        setContainerNavigationUrl(session.containerNavigationUrl);
    } else {
        updateContainerNavigationFromDisplayedImage();
    }
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
    resetZoom();
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
        = svgRasterSize(m_zoomState.displaySize(), displayDevicePixelRatio(), maximumTextureSize());
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
    const KiriView::ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setImageSize(imageSize, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
}

void KiriImageView::updateZoomState()
{
    const KiriView::ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.update(displayDevicePixelRatio());
    applyZoomStateChanges(previous);
}

void KiriImageView::applyZoomStateChanges(const KiriView::ImageZoomSnapshot &previous)
{
    const KiriView::ImageZoomSnapshot current = m_zoomState.snapshot();
    if (previous.imageSize != current.imageSize) {
        Q_EMIT imageSizeChanged();
    }
    if (!imageZoomApproximatelyEqual(previous.viewportSize, current.viewportSize)) {
        Q_EMIT viewportSizeChanged();
    }
    if (previous.zoomMode != current.zoomMode) {
        Q_EMIT zoomModeChanged();
    }
    if (!imageZoomApproximatelyEqual(previous.zoomPercent, current.zoomPercent)) {
        Q_EMIT zoomPercentChanged();
    }

    if (!imageZoomApproximatelyEqual(previous.displaySize, current.displaySize)) {
        Q_EMIT displaySizeChanged();
        updateDisplayedSvgRaster();
        update();
    }
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
    m_zoomState.clearContainer();
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
