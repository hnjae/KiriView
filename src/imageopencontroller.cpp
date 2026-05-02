// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopencontroller.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"
#include "imagepresentationcontroller.h"
#include "imageviewtext.h"
#include "kiriimagedecoder.h"
#include "predecodecache.h"

#include <memory>
#include <utility>

namespace {
using KiriView::containerNavigationUrlForImage;
using KiriView::decodedImageResultIsPredecodeCacheable;
}

namespace KiriView {
ImageOpenController::ImageOpenController(QObject *parent, ImageDocumentState &state,
    ImagePresentationController &presentationController,
    TakePredecodedImageCallback takePredecodedImage, EventCallback eventCallback)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_takePredecodedImage(std::move(takePredecodedImage))
    , m_eventCallback(std::move(eventCallback))
{
    m_imageLoader = std::make_unique<ImageLoader>(parent);
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
        [this](const QUrl &url) { return m_takePredecodedImage(url); });
}

ImageOpenController::~ImageOpenController() { cancel(); }

void ImageOpenController::open()
{
    cancel();
    m_state.setErrorString(QString());

    if (m_state.sourceUrl().isEmpty()) {
        finishEmptySourceLoad();
        return;
    }

    beginSourceLoad();
    m_imageLoader->start(ImageLoadRequest { m_state.sourceUrl(),
        m_state.displayedComicBookRootUrl(), m_state.loadingContainerNavigationUrl() });
}

void ImageOpenController::cancel() { m_imageLoader->cancel(); }

void ImageOpenController::finishEmptySourceLoad()
{
    report(DocumentEvent::clearImageRequested());
    m_presentationController.resetZoom();
    m_state.setLoading(false);
    m_state.clearLoadingContainerNavigationUrl();
    m_state.setContainerNavigationUrl(QUrl());
    m_state.setStatus(ImageDocumentStatus::Null);
}

void ImageOpenController::beginSourceLoad()
{
    if (!m_presentationController.hasImage() && m_state.loadingContainerNavigationUrl().isEmpty()) {
        m_state.setContainerNavigationUrl(QUrl());
    }

    m_state.setLoading(true);
    if (!m_presentationController.hasImage()) {
        report(DocumentEvent::clearImageRequested());
        m_presentationController.resetZoom();
        m_state.setStatus(ImageDocumentStatus::Loading);
    } else {
        m_state.setStatus(ImageDocumentStatus::Ready);
    }
}

void ImageOpenController::finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl)
{
    finishContainerNavigationLoadWithError(containerUrl,
        imageViewText("The selected container does not contain any supported images."));
}

void ImageOpenController::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, const QString &errorString)
{
    cancel();
    m_state.clearLoadingContainerNavigationUrl();

    report(DocumentEvent::clearImageRequested());
    m_presentationController.prepareFailedContainer(containerUrl);
    m_state.setLoading(false);
    m_state.setContainerNavigationUrl(containerUrl);
    m_state.setSourceUrl(containerUrl);

    const QString message = errorString.isEmpty()
        ? imageViewText("Could not open the selected container.")
        : errorString;
    m_state.setErrorString(message);
    m_state.setStatus(ImageDocumentStatus::Error);
}

void ImageOpenController::setSourceUrlFromResolvedLoad(const QUrl &sourceUrl)
{
    m_state.setSourceUrl(sourceUrl);
}

void ImageOpenController::finishPredecodedImageLoad(ImageLoadSession session, const QImage &image)
{
    finishLoadSuccessfully(session, image, true);
    report(DocumentEvent::adjacentImagePredecodeRequested());
}

void ImageOpenController::finishDecodedImageLoad(
    ImageLoadSession session, std::shared_ptr<DecodedImageResult> result)
{
    if (result->isSvg) {
        finishSvgLoadSuccessfully(
            std::move(session), std::move(result->svgData), result->svgIntrinsicSize);
        report(DocumentEvent::adjacentImagePredecodeRequested());
        return;
    }

    const bool predecodeCacheable
        = decodedImageResultIsPredecodeCacheable(*result, PredecodeCache::byteBudget());
    finishLoadSuccessfully(session, result->image, predecodeCacheable);
    if (!result->decodedAnimationFrames.empty()) {
        m_presentationController.startDecodedAnimation(
            std::move(result->decodedAnimationFrames), result->animationLoopCount);
    } else if (result->hasAnimationReaderFrames) {
        m_presentationController.startAnimation(result->animationData, result->animationFormat,
            result->animationLoopCount, result->firstFrameDelay);
    }
    report(DocumentEvent::adjacentImagePredecodeRequested());
}

void ImageOpenController::finishLoadWithError(
    const ImageLoadSession &session, ImageLoadError error, const QString &errorString)
{
    const QUrl containerNavigationUrl = session.request.containerNavigationUrl;
    m_state.clearLoadingContainerNavigationUrl();

    const QString message = error == ImageLoadError::EmptyComicBookArchive
        ? imageViewText("The selected comic book archive does not contain any supported images.")
        : errorString;
    if (!containerNavigationUrl.isEmpty()) {
        finishContainerNavigationLoadWithError(containerNavigationUrl, message);
        return;
    }

    m_state.setLoading(false);
    if (m_presentationController.hasImage()) {
        finishReplacementLoadWithError(message);
        return;
    }

    finishInitialLoadWithError(message);
}

void ImageOpenController::finishReplacementLoadWithError(const QString &errorString)
{
    m_state.setErrorString(errorString);
    m_state.setStatus(ImageDocumentStatus::Ready);

    if (!m_state.displayedUrl().isEmpty()) {
        m_state.setSourceUrl(m_state.displayedUrl());
    }
    report(DocumentEvent::pageNavigationUpdateRequested());
    report(DocumentEvent::adjacentImagePredecodeRequested());
}

void ImageOpenController::finishInitialLoadWithError(const QString &errorString)
{
    report(DocumentEvent::clearImageRequested());
    m_state.setContainerNavigationUrl(QUrl());
    m_state.setErrorString(errorString);
    m_state.setStatus(ImageDocumentStatus::Error);
}

void ImageOpenController::finishLoadSuccessfully(
    const ImageLoadSession &session, const QImage &image, bool predecodeCacheable)
{
    prepareSuccessfulImageLoad(session);
    m_presentationController.setPredecodeCacheable(predecodeCacheable);
    m_presentationController.setImage(image);
    m_presentationController.updateRenderContext();
    finishSuccessfulImageLoad(session);
}

void ImageOpenController::finishSvgLoadSuccessfully(
    ImageLoadSession session, QByteArray data, const QSize &intrinsicSize)
{
    const QUrl loadedContainerUrl = containerNavigationUrlForImage(
        session.location.imageUrl, session.location.comicBookRootUrl);
    const std::optional<QString> errorString = m_presentationController.setLoadedSvgImage(
        std::move(data), intrinsicSize, loadedContainerUrl);
    if (errorString.has_value()) {
        finishLoadWithError(session, ImageLoadError::Generic, *errorString);
        return;
    }

    finishSuccessfulImageLoad(session);
}

void ImageOpenController::prepareSuccessfulImageLoad(const ImageLoadSession &session)
{
    m_presentationController.stopAnimation();
    const QUrl loadedContainerUrl = containerNavigationUrlForImage(
        session.location.imageUrl, session.location.comicBookRootUrl);
    m_presentationController.prepareImageContainer(loadedContainerUrl);
}

void ImageOpenController::finishSuccessfulImageLoad(const ImageLoadSession &session)
{
    setSourceUrlFromResolvedLoad(session.location.imageUrl);
    m_state.setDisplayedImageLocation(session.location);
    if (!session.request.containerNavigationUrl.isEmpty()) {
        m_state.setContainerNavigationUrl(session.request.containerNavigationUrl);
    } else {
        updateContainerNavigationFromDisplayedImage();
    }
    m_state.clearLoadingContainerNavigationUrl();
    m_state.setErrorString(QString());
    m_state.setLoading(false);
    m_state.setStatus(ImageDocumentStatus::Ready);
    report(DocumentEvent::pageNavigationUpdateRequested());
}

void ImageOpenController::updateContainerNavigationFromDisplayedImage()
{
    if (!m_presentationController.hasImage() || m_state.displayedUrl().isEmpty()) {
        m_state.setContainerNavigationUrl(QUrl());
        return;
    }

    m_state.setContainerNavigationUrl(containerNavigationUrlForImage(
        m_state.displayedUrl(), m_state.displayedComicBookRootUrl()));
}

void ImageOpenController::report(DocumentEvent event)
{
    if (m_eventCallback) {
        m_eventCallback(std::move(event));
    }
}
}
