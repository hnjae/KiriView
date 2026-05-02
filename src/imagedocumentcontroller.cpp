// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "displayedimagestate.h"
#include "imagenavigationservice.h"
#include "imagepredecodecoordinator.h"
#include "kiriimagedecoder.h"
#include "predecodecache.h"

#include <QCoreApplication>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>

namespace {
using KiriView::containerNavigationUrlForImage;
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::imageZoomApproximatelyEqual;
using KiriView::NavigationDirection;
using KiriView::renderSvgImage;
using KiriView::svgRasterSize;
using KiriView::windowTitleFileNameForDisplayedUrl;

QString imageViewText(const char *sourceText)
{
    return QCoreApplication::translate("KiriImageView", sourceText);
}
}

namespace KiriView {
ImageDocumentController::ImageDocumentController(
    QObject *parent, RenderContextProvider renderContextProvider, ChangeCallback changeCallback)
    : QObject(parent)
    , m_renderContextProvider(std::move(renderContextProvider))
    , m_changeCallback(std::move(changeCallback))
    , m_state([this](ImageDocumentChange change) { notify(change); })
{
    m_displayedImageState = std::make_unique<DisplayedImageState>(
        this,
        [this](const QSize &imageSize) {
            setImageSize(imageSize);
            notify(ImageDocumentChange::Repaint);
        },
        [this](const QString &errorString) { finishWithAnimationError(errorString); });
    m_navigationService = std::make_unique<ImageNavigationService>(this);
    m_navigationService->setOpenUrlCallback([this](const QUrl &url) { setSourceUrl(url); });
    m_navigationService->setOpenContainerImageCallback(
        [this](const QUrl &imageUrl, const QUrl &containerUrl) {
            openImageFromContainerNavigation(imageUrl, containerUrl);
        });
    m_navigationService->setContainerNavigationErrorCallback(
        [this](
            const QUrl &containerUrl, ContainerNavigationError error, const QString &errorString) {
            if (error == ContainerNavigationError::EmptyContainer) {
                finishContainerNavigationWithEmptyContainer(containerUrl);
                return;
            }

            if (error == ContainerNavigationError::InvalidComicBookArchive) {
                finishContainerNavigationLoadWithError(
                    containerUrl, imageViewText("Could not open the selected comic book archive."));
                return;
            }

            finishContainerNavigationLoadWithError(containerUrl, errorString);
        });
    m_navigationService->setPageNavigationChangedCallback(
        [this]() { notify(ImageDocumentChange::PageNavigation); });
    m_imageLoader = std::make_unique<ImageLoader>(this);
    m_imageLoader->setSourceResolvedCallback(
        [this](const QUrl &sourceUrl) { setSourceUrlFromResolvedLoad(sourceUrl); });
    m_imageLoader->setErrorCallback(
        [this](const ImageLoadSession &session, ImageLoadError error, const QString &errorString) {
            finishLoadWithError(session, error, errorString);
        });
    m_imageLoader->setDecodedImageCallback(
        [this](ImageLoadSession session, std::shared_ptr<DecodedImageResult> result) {
            finishDecodedImageLoad(std::move(session), std::move(result));
        });
    m_imageLoader->setPredecodedImageCallback(
        [this](ImageLoadSession session, const QImage &image) {
            finishPredecodedImageLoad(std::move(session), image);
        });
    m_imageLoader->setTakePredecodedImageCallback(
        [this](const QUrl &url) { return takePredecodedImage(url); });
    m_predecodeCoordinator = std::make_unique<ImagePredecodeCoordinator>(this);
}

ImageDocumentController::~ImageDocumentController()
{
    stopAnimation();
    cancelPredecode();
    cancelPageNavigationUpdate();
    cancelContainerNavigation();
    cancelNavigation();
    cancelLoad();
}

QUrl ImageDocumentController::sourceUrl() const { return m_state.sourceUrl(); }

void ImageDocumentController::setSourceUrl(const QUrl &sourceUrl)
{
    setSourceUrlForLoad(sourceUrl, QUrl());
}

ImageDocumentStatus ImageDocumentController::status() const { return m_state.status(); }

bool ImageDocumentController::loading() const { return m_state.loading(); }

QString ImageDocumentController::errorString() const { return m_state.errorString(); }

QString ImageDocumentController::windowTitleFileName() const
{
    return m_state.windowTitleFileName();
}

QSize ImageDocumentController::imageSize() const { return m_zoomState.imageSize(); }

QSizeF ImageDocumentController::viewportSize() const { return m_zoomState.viewportSize(); }

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setViewportSize(viewportSize, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
}

QSizeF ImageDocumentController::displaySize() const { return m_zoomState.displaySize(); }

qreal ImageDocumentController::zoomPercent() const { return m_zoomState.zoomPercent(); }

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setManualZoomPercent(zoomPercent, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
}

ImageZoomMode ImageDocumentController::zoomMode() const { return m_zoomState.zoomMode(); }

int ImageDocumentController::currentPageNumber() const
{
    return m_navigationService->currentPageNumber();
}

int ImageDocumentController::imageCount() const { return m_navigationService->imageCount(); }

bool ImageDocumentController::containerNavigationAvailable() const
{
    return m_state.containerNavigationAvailable();
}

const QImage &ImageDocumentController::image() const { return m_displayedImageState->image(); }

quint64 ImageDocumentController::imageRevision() const { return m_displayedImageState->revision(); }

void ImageDocumentController::openPreviousImage()
{
    openAdjacentImage(NavigationDirection::Previous);
}

void ImageDocumentController::openNextImage() { openAdjacentImage(NavigationDirection::Next); }

void ImageDocumentController::openPreviousContainer()
{
    openAdjacentContainer(NavigationDirection::Previous);
}

void ImageDocumentController::openNextContainer()
{
    openAdjacentContainer(NavigationDirection::Next);
}

void ImageDocumentController::openImageAtPage(int pageNumber)
{
    const std::optional<QUrl> pageUrl = m_navigationService->urlAtPage(pageNumber);
    if (!pageUrl.has_value()) {
        return;
    }

    setSourceUrl(*pageUrl);
}

void ImageDocumentController::resetZoom()
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.resetZoom(displayDevicePixelRatio());
    applyZoomStateChanges(previous);
}

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    if (zoomMode == ImageZoomMode::Manual) {
        return;
    }

    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setFitMode(zoomMode, displayDevicePixelRatio())) {
        return;
    }
    applyZoomStateChanges(previous);
}

void ImageDocumentController::updateRenderContext()
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.update(displayDevicePixelRatio());
    applyZoomStateChanges(previous);
}

void ImageDocumentController::setSourceUrlForLoad(
    const QUrl &sourceUrl, const QUrl &containerNavigationUrl)
{
    if (m_state.sourceUrl() == sourceUrl) {
        m_state.clearLoadingContainerNavigationUrl();
        if (!containerNavigationUrl.isEmpty()) {
            setContainerNavigationUrl(containerNavigationUrl);
        }
        return;
    }

    cancelNavigation();
    cancelContainerNavigation();
    m_state.setLoadingContainerNavigationUrl(containerNavigationUrl);
    m_state.setSourceUrl(sourceUrl);
    startLoad();
}

void ImageDocumentController::startLoad()
{
    cancelLoad();
    cancelPredecode();
    setErrorString(QString());

    if (m_state.sourceUrl().isEmpty()) {
        clearImage();
        resetZoom();
        setLoading(false);
        m_state.clearLoadingContainerNavigationUrl();
        setContainerNavigationUrl(QUrl());
        setStatus(ImageDocumentStatus::Null);
        return;
    }

    if (!hasDisplayedImage() && m_state.loadingContainerNavigationUrl().isEmpty()) {
        setContainerNavigationUrl(QUrl());
    }

    setLoading(true);
    if (!hasDisplayedImage()) {
        clearImage();
        resetZoom();
        setStatus(ImageDocumentStatus::Loading);
    } else {
        setStatus(ImageDocumentStatus::Ready);
    }

    m_imageLoader->start(m_state.sourceUrl(), m_state.displayedComicBookRootUrl(),
        m_state.loadingContainerNavigationUrl());
}

void ImageDocumentController::cancelLoad() { m_imageLoader->cancel(); }

void ImageDocumentController::setSourceUrlFromResolvedLoad(const QUrl &sourceUrl)
{
    m_state.setSourceUrl(sourceUrl);
}

void ImageDocumentController::openAdjacentImage(NavigationDirection direction)
{
    m_navigationService->openAdjacentImage(
        ImageNavigationService::DisplayContext {
            hasDisplayedImage(), m_state.displayedUrl(), m_state.displayedComicBookRootUrl() },
        direction);
}

void ImageDocumentController::cancelNavigation() { m_navigationService->cancelNavigation(); }

void ImageDocumentController::openAdjacentContainer(NavigationDirection direction)
{
    m_navigationService->openAdjacentContainer(m_state.containerNavigationUrl(), direction);
}

void ImageDocumentController::cancelContainerNavigation()
{
    m_navigationService->cancelContainerNavigation();
}

void ImageDocumentController::openImageFromContainerNavigation(
    const QUrl &imageUrl, const QUrl &containerUrl)
{
    setSourceUrlForLoad(imageUrl, containerUrl);
}

void ImageDocumentController::finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl)
{
    finishContainerNavigationLoadWithError(containerUrl,
        imageViewText("The selected container does not contain any supported images."));
}

void ImageDocumentController::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, const QString &errorString)
{
    cancelLoad();
    m_state.clearLoadingContainerNavigationUrl();

    clearImage();
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.clearContainer();
    m_zoomState.prepareImageContainer(containerUrl);
    m_zoomState.resetZoom(displayDevicePixelRatio());
    applyZoomStateChanges(previous);
    setLoading(false);
    setContainerNavigationUrl(containerUrl);

    m_state.setSourceUrl(containerUrl);

    const QString message = errorString.isEmpty()
        ? imageViewText("Could not open the selected container.")
        : errorString;
    setErrorString(message);
    setStatus(ImageDocumentStatus::Error);
}

void ImageDocumentController::setContainerNavigationUrl(const QUrl &containerUrl)
{
    m_state.setContainerNavigationUrl(containerUrl);
}

void ImageDocumentController::updateContainerNavigationFromDisplayedImage()
{
    if (!hasDisplayedImage() || m_state.displayedUrl().isEmpty()) {
        setContainerNavigationUrl(QUrl());
        return;
    }

    setContainerNavigationUrl(containerNavigationUrlForImage(
        m_state.displayedUrl(), m_state.displayedComicBookRootUrl()));
}

void ImageDocumentController::updatePageNavigation()
{
    m_navigationService->updatePageNavigation(ImageNavigationService::DisplayContext {
        hasDisplayedImage(), m_state.displayedUrl(), m_state.displayedComicBookRootUrl() });
}

void ImageDocumentController::cancelPageNavigationUpdate()
{
    m_navigationService->cancelPageNavigationUpdate();
}

void ImageDocumentController::clearPageNavigation() { m_navigationService->clearPageNavigation(); }

void ImageDocumentController::scheduleAdjacentImagePredecode()
{
    if (!hasDisplayedImage() || m_state.displayedUrl().isEmpty()) {
        cancelPredecode();
        return;
    }

    m_predecodeCoordinator->schedule(ImagePredecodeCoordinator::Context { m_state.displayedUrl(),
        m_state.displayedComicBookRootUrl(), m_displayedImageState->isPredecodeCacheable(),
        m_displayedImageState->image() });
}

void ImageDocumentController::cancelPredecode()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->cancel();
    }
}

std::optional<PredecodedImage> ImageDocumentController::takePredecodedImage(const QUrl &url) const
{
    QImage image;
    QUrl comicBookRootUrl;
    if (m_predecodeCoordinator == nullptr
        || !m_predecodeCoordinator->tryTake(url, &image, &comicBookRootUrl)) {
        return std::nullopt;
    }

    return PredecodedImage { image, comicBookRootUrl };
}

void ImageDocumentController::finishPredecodedImageLoad(
    ImageLoadSession session, const QImage &image)
{
    finishLoadSuccessfully(session, image, true);
    scheduleAdjacentImagePredecode();
}

void ImageDocumentController::finishDecodedImageLoad(
    ImageLoadSession session, std::shared_ptr<DecodedImageResult> result)
{
    if (result->isSvg) {
        finishSvgLoadSuccessfully(
            std::move(session), std::move(result->svgData), result->svgIntrinsicSize);
        scheduleAdjacentImagePredecode();
        return;
    }

    const bool predecodeCacheable
        = decodedImageResultIsPredecodeCacheable(*result, PredecodeCache::byteBudget());
    finishLoadSuccessfully(session, result->image, predecodeCacheable);
    if (!result->decodedAnimationFrames.empty()) {
        m_displayedImageState->startDecodedAnimation(
            std::move(result->decodedAnimationFrames), result->animationLoopCount);
    } else if (result->hasAnimationReaderFrames) {
        m_displayedImageState->startAnimation(result->animationData, result->animationFormat,
            result->animationLoopCount, result->firstFrameDelay);
    }
    scheduleAdjacentImagePredecode();
}

void ImageDocumentController::finishLoadWithError(
    const ImageLoadSession &session, ImageLoadError error, const QString &errorString)
{
    const QUrl containerNavigationUrl = session.containerNavigationUrl;
    m_state.clearLoadingContainerNavigationUrl();

    const QString message = error == ImageLoadError::EmptyComicBookArchive
        ? imageViewText("The selected comic book archive does not contain any supported images.")
        : errorString;
    if (!containerNavigationUrl.isEmpty()) {
        finishContainerNavigationLoadWithError(containerNavigationUrl, message);
        return;
    }

    setLoading(false);

    if (hasDisplayedImage()) {
        setErrorString(message);
        setStatus(ImageDocumentStatus::Ready);

        if (!m_state.displayedUrl().isEmpty()) {
            m_state.setSourceUrl(m_state.displayedUrl());
        }
        updatePageNavigation();
        scheduleAdjacentImagePredecode();
        return;
    }

    clearImage();
    m_zoomState.clearContainer();
    setContainerNavigationUrl(QUrl());
    setErrorString(message);
    setStatus(ImageDocumentStatus::Error);
}

void ImageDocumentController::finishLoadSuccessfully(
    const ImageLoadSession &session, const QImage &image, bool predecodeCacheable)
{
    prepareSuccessfulImageLoad(session);
    m_displayedImageState->setPredecodeCacheable(predecodeCacheable);
    setDisplayedImage(image);
    updateRenderContext();
    finishSuccessfulImageLoad(session);
}

void ImageDocumentController::finishSvgLoadSuccessfully(
    ImageLoadSession session, QByteArray data, const QSize &intrinsicSize)
{
    if (data.isEmpty() || intrinsicSize.isEmpty()) {
        finishLoadWithError(session, ImageLoadError::Generic,
            imageViewText("Could not decode the selected SVG image."));
        return;
    }

    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
    const LoadedImageZoom loadedZoom
        = m_zoomState.loadedImageZoom(loadedContainerUrl, intrinsicSize, displayDevicePixelRatio());
    const QSize rasterSize
        = svgRasterSize(loadedZoom.displaySize, displayDevicePixelRatio(), maximumTextureSize());
    const QImage image = renderSvgImage(data, rasterSize);
    if (image.isNull()) {
        finishLoadWithError(session, ImageLoadError::Generic,
            imageViewText("Could not render the selected SVG image."));
        return;
    }

    stopAnimation();
    m_displayedImageState->setPredecodeCacheable(false);
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.setLoadedSvgZoom(loadedContainerUrl, loadedZoom);
    applyZoomStateChanges(previous);
    setDisplayedSvgImage(std::move(data), intrinsicSize, image, rasterSize);
    finishSuccessfulImageLoad(session);
}

void ImageDocumentController::prepareSuccessfulImageLoad(const ImageLoadSession &session)
{
    stopAnimation();
    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    m_zoomState.prepareImageContainer(loadedContainerUrl);
    applyZoomStateChanges(previous);
}

void ImageDocumentController::finishSuccessfulImageLoad(const ImageLoadSession &session)
{
    setSourceUrlFromResolvedLoad(session.imageUrl);
    m_state.setDisplayedImageUrls(session.imageUrl, session.comicBookRootUrl);
    updateWindowTitleFileName();
    if (!session.containerNavigationUrl.isEmpty()) {
        setContainerNavigationUrl(session.containerNavigationUrl);
    } else {
        updateContainerNavigationFromDisplayedImage();
    }
    m_state.clearLoadingContainerNavigationUrl();
    setErrorString(QString());
    setLoading(false);
    setStatus(ImageDocumentStatus::Ready);
    updatePageNavigation();
}

bool ImageDocumentController::hasDisplayedImage() const
{
    return m_displayedImageState->hasImage();
}

void ImageDocumentController::stopAnimation() { m_displayedImageState->stopAnimation(); }

void ImageDocumentController::finishWithAnimationError(const QString &errorString)
{
    setLoading(false);
    clearImage();
    resetZoom();
    setContainerNavigationUrl(QUrl());
    clearPageNavigation();
    const QString message = errorString.isEmpty()
        ? imageViewText("Could not decode the selected image animation.")
        : errorString;
    setErrorString(message);
    setStatus(ImageDocumentStatus::Error);
}

void ImageDocumentController::setDisplayedImage(const QImage &image)
{
    m_displayedImageState->setImage(image);
}

void ImageDocumentController::setDisplayedSvgImage(
    QByteArray data, const QSize &intrinsicSize, const QImage &image, const QSize &rasterSize)
{
    m_displayedImageState->setSvgImage(std::move(data), intrinsicSize, image, rasterSize);
}

bool ImageDocumentController::updateDisplayedSvgRaster()
{
    return m_displayedImageState->updateSvgRaster(
        m_zoomState.displaySize(), displayDevicePixelRatio(), maximumTextureSize());
}

void ImageDocumentController::setLoading(bool loading) { m_state.setLoading(loading); }

void ImageDocumentController::setStatus(ImageDocumentStatus status) { m_state.setStatus(status); }

void ImageDocumentController::setErrorString(const QString &errorString)
{
    m_state.setErrorString(errorString);
}

void ImageDocumentController::setWindowTitleFileName(const QString &fileName)
{
    m_state.setWindowTitleFileName(fileName);
}

void ImageDocumentController::updateWindowTitleFileName()
{
    setWindowTitleFileName(windowTitleFileNameForDisplayedUrl(
        m_state.displayedUrl(), m_state.displayedComicBookRootUrl()));
}

void ImageDocumentController::setImageSize(const QSize &imageSize)
{
    const ImageZoomSnapshot previous = m_zoomState.snapshot();
    if (!m_zoomState.setImageSize(imageSize, displayDevicePixelRatio())) {
        return;
    }

    applyZoomStateChanges(previous);
}

void ImageDocumentController::applyZoomStateChanges(const ImageZoomSnapshot &previous)
{
    const ImageZoomSnapshot current = m_zoomState.snapshot();
    if (previous.imageSize != current.imageSize) {
        notify(ImageDocumentChange::ImageSize);
    }
    if (!imageZoomApproximatelyEqual(previous.viewportSize, current.viewportSize)) {
        notify(ImageDocumentChange::ViewportSize);
    }
    if (previous.zoomMode != current.zoomMode) {
        notify(ImageDocumentChange::ZoomMode);
    }
    if (!imageZoomApproximatelyEqual(previous.zoomPercent, current.zoomPercent)) {
        notify(ImageDocumentChange::ZoomPercent);
    }

    if (!imageZoomApproximatelyEqual(previous.displaySize, current.displaySize)) {
        notify(ImageDocumentChange::DisplaySize);
        updateDisplayedSvgRaster();
        notify(ImageDocumentChange::Repaint);
    }
}

qreal ImageDocumentController::displayDevicePixelRatio() const
{
    return renderContext().devicePixelRatio;
}

int ImageDocumentController::maximumTextureSize() const
{
    return renderContext().maximumTextureSize;
}

ImageDocumentRenderContext ImageDocumentController::renderContext() const
{
    ImageDocumentRenderContext context
        = m_renderContextProvider ? m_renderContextProvider() : ImageDocumentRenderContext {};
    if (!std::isfinite(context.devicePixelRatio) || context.devicePixelRatio <= 0.0) {
        context.devicePixelRatio = 1.0;
    }
    if (context.maximumTextureSize <= 0) {
        context.maximumTextureSize = fallbackTextureSizeMax;
    }
    return context;
}

void ImageDocumentController::notify(ImageDocumentChange change)
{
    if (m_changeCallback) {
        m_changeCallback(change);
    }
}

void ImageDocumentController::clearImage()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->clear();
    }
    cancelPageNavigationUpdate();
    m_state.clearDisplayedImageUrls();
    m_zoomState.clearContainer();
    m_displayedImageState->clear();
    updateWindowTitleFileName();
    clearPageNavigation();

    setImageSize(QSize());
}
}
