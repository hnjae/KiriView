// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopencontroller.h"

#include "imagedocumentstate.h"
#include "imagepresentationcontroller.h"
#include "kiriimagedecoder.h"
#include "kiriimagenavigation.h"
#include "predecodecache.h"

#include <QCoreApplication>
#include <memory>
#include <utility>

namespace {
using KiriView::containerNavigationUrlForImage;
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::windowTitleFileNameForDisplayedUrl;

QString imageViewText(const char *sourceText)
{
    return QCoreApplication::translate("KiriImageView", sourceText);
}
}

namespace KiriView {
ImageOpenController::ImageOpenController(QObject *parent, ImageDocumentState &state,
    ImagePresentationController &presentationController,
    TakePredecodedImageCallback takePredecodedImage, VoidCallback clearImage,
    VoidCallback updatePageNavigation, VoidCallback scheduleAdjacentImagePredecode)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_takePredecodedImage(std::move(takePredecodedImage))
    , m_clearImage(std::move(clearImage))
    , m_updatePageNavigation(std::move(updatePageNavigation))
    , m_scheduleAdjacentImagePredecode(std::move(scheduleAdjacentImagePredecode))
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
        m_clearImage();
        m_presentationController.resetZoom();
        m_state.setLoading(false);
        m_state.clearLoadingContainerNavigationUrl();
        m_state.setContainerNavigationUrl(QUrl());
        m_state.setStatus(ImageDocumentStatus::Null);
        return;
    }

    if (!m_presentationController.hasImage() && m_state.loadingContainerNavigationUrl().isEmpty()) {
        m_state.setContainerNavigationUrl(QUrl());
    }

    m_state.setLoading(true);
    if (!m_presentationController.hasImage()) {
        m_clearImage();
        m_presentationController.resetZoom();
        m_state.setStatus(ImageDocumentStatus::Loading);
    } else {
        m_state.setStatus(ImageDocumentStatus::Ready);
    }

    m_imageLoader->start(m_state.sourceUrl(), m_state.displayedComicBookRootUrl(),
        m_state.loadingContainerNavigationUrl());
}

void ImageOpenController::cancel() { m_imageLoader->cancel(); }

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

    m_clearImage();
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
    m_scheduleAdjacentImagePredecode();
}

void ImageOpenController::finishDecodedImageLoad(
    ImageLoadSession session, std::shared_ptr<DecodedImageResult> result)
{
    if (result->isSvg) {
        finishSvgLoadSuccessfully(
            std::move(session), std::move(result->svgData), result->svgIntrinsicSize);
        m_scheduleAdjacentImagePredecode();
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
    m_scheduleAdjacentImagePredecode();
}

void ImageOpenController::finishLoadWithError(
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

    m_state.setLoading(false);

    if (m_presentationController.hasImage()) {
        m_state.setErrorString(message);
        m_state.setStatus(ImageDocumentStatus::Ready);

        if (!m_state.displayedUrl().isEmpty()) {
            m_state.setSourceUrl(m_state.displayedUrl());
        }
        m_updatePageNavigation();
        m_scheduleAdjacentImagePredecode();
        return;
    }

    m_clearImage();
    m_state.setContainerNavigationUrl(QUrl());
    m_state.setErrorString(message);
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
    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
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
    const QUrl loadedContainerUrl
        = containerNavigationUrlForImage(session.imageUrl, session.comicBookRootUrl);
    m_presentationController.prepareImageContainer(loadedContainerUrl);
}

void ImageOpenController::finishSuccessfulImageLoad(const ImageLoadSession &session)
{
    setSourceUrlFromResolvedLoad(session.imageUrl);
    m_state.setDisplayedImageUrls(session.imageUrl, session.comicBookRootUrl);
    m_state.setWindowTitleFileName(windowTitleFileNameForDisplayedUrl(
        m_state.displayedUrl(), m_state.displayedComicBookRootUrl()));
    if (!session.containerNavigationUrl.isEmpty()) {
        m_state.setContainerNavigationUrl(session.containerNavigationUrl);
    } else {
        updateContainerNavigationFromDisplayedImage();
    }
    m_state.clearLoadingContainerNavigationUrl();
    m_state.setErrorString(QString());
    m_state.setLoading(false);
    m_state.setStatus(ImageDocumentStatus::Ready);
    m_updatePageNavigation();
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
}
