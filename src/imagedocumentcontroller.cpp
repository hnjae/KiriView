// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "imagenavigationservice.h"
#include "imagepredecodecoordinator.h"
#include "imagepresentationcontroller.h"
#include "kiriimagedecoder.h"
#include "predecodecache.h"

#include <QCoreApplication>
#include <memory>
#include <optional>
#include <utility>

namespace {
using KiriView::containerNavigationUrlForImage;
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::NavigationDirection;
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
    m_presentationController = std::make_unique<ImagePresentationController>(
        this, m_renderContextProvider, [this](ImageDocumentChange change) { notify(change); },
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

QSize ImageDocumentController::imageSize() const { return m_presentationController->imageSize(); }

QSizeF ImageDocumentController::viewportSize() const
{
    return m_presentationController->viewportSize();
}

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    m_presentationController->setViewportSize(viewportSize);
}

QSizeF ImageDocumentController::displaySize() const
{
    return m_presentationController->displaySize();
}

qreal ImageDocumentController::zoomPercent() const
{
    return m_presentationController->zoomPercent();
}

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    m_presentationController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentController::zoomMode() const
{
    return m_presentationController->zoomMode();
}

int ImageDocumentController::currentPageNumber() const
{
    return m_navigationService->currentPageNumber();
}

int ImageDocumentController::imageCount() const { return m_navigationService->imageCount(); }

bool ImageDocumentController::containerNavigationAvailable() const
{
    return m_state.containerNavigationAvailable();
}

const QImage &ImageDocumentController::image() const { return m_presentationController->image(); }

quint64 ImageDocumentController::imageRevision() const
{
    return m_presentationController->imageRevision();
}

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

void ImageDocumentController::resetZoom() { m_presentationController->resetZoom(); }

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    m_presentationController->setFitMode(zoomMode);
}

void ImageDocumentController::updateRenderContext()
{
    m_presentationController->updateRenderContext();
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
    m_presentationController->prepareFailedContainer(containerUrl);
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
        m_state.displayedComicBookRootUrl(), m_presentationController->isPredecodeCacheable(),
        m_presentationController->image() });
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
        m_presentationController->startDecodedAnimation(
            std::move(result->decodedAnimationFrames), result->animationLoopCount);
    } else if (result->hasAnimationReaderFrames) {
        m_presentationController->startAnimation(result->animationData, result->animationFormat,
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
    setContainerNavigationUrl(QUrl());
    setErrorString(message);
    setStatus(ImageDocumentStatus::Error);
}

void ImageDocumentController::finishLoadSuccessfully(
    const ImageLoadSession &session, const QImage &image, bool predecodeCacheable)
{
    prepareSuccessfulImageLoad(session);
    m_presentationController->setPredecodeCacheable(predecodeCacheable);
    m_presentationController->setImage(image);
    updateRenderContext();
    finishSuccessfulImageLoad(session);
}

void ImageDocumentController::finishSvgLoadSuccessfully(
    ImageLoadSession session, QByteArray data, const QSize &intrinsicSize)
{
    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
    const std::optional<QString> errorString = m_presentationController->setLoadedSvgImage(
        std::move(data), intrinsicSize, loadedContainerUrl);
    if (errorString.has_value()) {
        finishLoadWithError(session, ImageLoadError::Generic, *errorString);
        return;
    }

    finishSuccessfulImageLoad(session);
}

void ImageDocumentController::prepareSuccessfulImageLoad(const ImageLoadSession &session)
{
    stopAnimation();
    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
    m_presentationController->prepareImageContainer(loadedContainerUrl);
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
    return m_presentationController->hasImage();
}

void ImageDocumentController::stopAnimation() { m_presentationController->stopAnimation(); }

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
    m_presentationController->clearImage();
    updateWindowTitleFileName();
    clearPageNavigation();
}
}
