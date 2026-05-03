// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopencontroller.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imageviewtext.h"
#include "kiriimagedecoder.h"
#include "predecodecache.h"

#include <memory>
#include <utility>
#include <variant>

namespace {
using KiriView::decodedImageResultIsPredecodeCacheable;
using KiriView::imageContainerUrlForLocation;
}

namespace KiriView {
ImageOpenController::ImageOpenController(QObject *parent, ImageDocumentState &state,
    ImagePresentationController &presentationController, ImageOpenController::Callbacks callbacks,
    const ImageAsyncDependencies &dependencies)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_callbacks(std::move(callbacks))
{
    m_imageLoader = std::make_unique<ImageLoader>(parent, dependencies);
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
    m_imageLoader->setTakePredecodedImageCallback([this](const QUrl &url) {
        if (!m_callbacks.takePredecodedImage) {
            return std::optional<PredecodedImage>();
        }

        return m_callbacks.takePredecodedImage(url);
    });
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
    m_imageLoader->start(ImageLoadRequest::fromLocation(m_state.sourceUrl(),
        m_state.displayedArchiveDocument(), m_state.loadingContainerNavigationUrl()));
}

void ImageOpenController::cancel() { m_imageLoader->cancel(); }

void ImageOpenController::finishEmptySourceLoad()
{
    reportEffects(ImageOpenWorkflow::finishEmptySourceLoad(m_state));
}

void ImageOpenController::beginSourceLoad()
{
    reportEffects(ImageOpenWorkflow::beginSourceLoad(m_state, m_presentationController.hasImage()));
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

    const QString message = errorString.isEmpty()
        ? imageViewText("Could not open the selected container.")
        : errorString;
    reportEffects(
        ImageOpenWorkflow::finishContainerNavigationLoadWithError(m_state, containerUrl, message));
}

void ImageOpenController::setSourceUrlFromResolvedLoad(const QUrl &sourceUrl)
{
    m_state.setSourceUrl(sourceUrl);
}

void ImageOpenController::finishPredecodedImageLoad(ImageLoadSession session, const QImage &image)
{
    finishLoadSuccessfully(session, image, true);
    report(ImageDocumentEffect::scheduleAdjacentImagePredecode());
}

void ImageOpenController::finishDecodedImageLoad(
    ImageLoadSession session, std::shared_ptr<DecodedImageResult> result)
{
    auto handleDecoded = [this, &session, &result](auto &decoded) {
        finishDecodedImageResult(session, decoded, *result);
    };
    std::visit(handleDecoded, *result);
    report(ImageDocumentEffect::scheduleAdjacentImagePredecode());
}

void ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, DecodedImageFailure &decoded, const DecodedImageResult &)
{
    finishLoadWithError(session, ImageLoadError::Generic, decoded.errorString);
}

void ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, SvgDecodedImage &decoded, const DecodedImageResult &)
{
    finishSvgLoadSuccessfully(session, std::move(decoded.data), decoded.svgIntrinsicSize);
}

void ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, StaticDecodedImage &decoded, const DecodedImageResult &result)
{
    const bool predecodeCacheable
        = decodedImageResultIsPredecodeCacheable(result, PredecodeCache::byteBudget());
    finishLoadSuccessfully(session, decoded.image, predecodeCacheable);
}

void ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, DecodedAnimationImage &decoded, const DecodedImageResult &)
{
    if (decoded.frames.empty()) {
        finishLoadWithError(session, ImageLoadError::Generic,
            imageViewText("Could not decode the selected image animation."));
        return;
    }

    finishLoadSuccessfully(session, decoded.frames.front().image, false);
    m_presentationController.startDecodedAnimation(std::move(decoded.frames), decoded.loopCount);
}

void ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, ReaderAnimationImage &decoded, const DecodedImageResult &)
{
    finishLoadSuccessfully(session, decoded.firstFrame, false);
    m_presentationController.startAnimation(
        decoded.data, decoded.format, decoded.loopCount, decoded.firstFrameDelay);
}

void ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, HeifSequenceAnimationImage &decoded, const DecodedImageResult &)
{
    finishLoadSuccessfully(session, decoded.firstFrame, false);
    m_presentationController.startHeifSequenceAnimation(decoded.data, decoded.firstFrameDelay);
}

void ImageOpenController::finishLoadWithError(
    const ImageLoadSession &session, ImageLoadError error, const QString &errorString)
{
    const QUrl containerNavigationUrl = session.request.containerNavigationUrl();
    const QString message = error == ImageLoadError::EmptyArchive
        ? imageViewText("The selected archive does not contain any supported images.")
        : errorString;
    if (!containerNavigationUrl.isEmpty()) {
        finishContainerNavigationLoadWithError(containerNavigationUrl, message);
        return;
    }

    if (m_presentationController.hasImage()) {
        finishReplacementLoadWithError(message);
        return;
    }

    finishInitialLoadWithError(message);
}

void ImageOpenController::finishReplacementLoadWithError(const QString &errorString)
{
    reportEffects(ImageOpenWorkflow::finishReplacementLoadWithError(m_state, errorString));
}

void ImageOpenController::finishInitialLoadWithError(const QString &errorString)
{
    reportEffects(ImageOpenWorkflow::finishInitialLoadWithError(m_state, errorString));
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
    const QUrl loadedContainerUrl = imageContainerUrlForLocation(session.location);
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
    const QUrl loadedContainerUrl = imageContainerUrlForLocation(session.location);
    m_presentationController.prepareImageContainer(loadedContainerUrl);
}

void ImageOpenController::finishSuccessfulImageLoad(const ImageLoadSession &session)
{
    reportEffects(ImageOpenWorkflow::finishSuccessfulImageLoad(m_state, session));
}

void ImageOpenController::reportEffects(const ImageDocumentEffects &effects)
{
    for (const ImageDocumentEffect &effect : effects.items()) {
        report(effect);
    }
}

void ImageOpenController::report(ImageDocumentEffect effect)
{
    if (m_callbacks.effect) {
        m_callbacks.effect(std::move(effect));
    }
}
}
