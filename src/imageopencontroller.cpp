// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopencontroller.h"

#include "imagecontainer.h"
#include "imagedocumentstate.h"
#include "imageloader.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imageviewtext.h"
#include "predecodecache.h"

#include <memory>
#include <utility>
#include <variant>

namespace {
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
    m_imageLoader = std::make_unique<ImageLoader>(parent, dependencies,
        ImageLoader::Callbacks {
            [this](const QUrl &sourceUrl) { setSourceUrlFromResolvedLoad(sourceUrl); },
            [this](const ImageLoadSession &session, ImageLoadError error,
                const QString &errorString) { finishLoadWithError(session, error, errorString); },
            [this](ImageLoadSession session, std::shared_ptr<DecodedImage> image) {
                finishDecodedImageLoad(std::move(session), std::move(image));
            },
            [this](ImageLoadSession session, PredecodedImage image) {
                finishPredecodedImageLoad(std::move(session), std::move(image));
            },
            [this](const QUrl &url) {
                if (!m_callbacks.takePredecodedImage) {
                    return std::optional<PredecodedImage>();
                }

                return m_callbacks.takePredecodedImage(url);
            },
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
    m_imageLoader->start(
        ImageLoadRequest::fromLocation(m_state.sourceUrl(), m_state.displayedArchiveDocument(),
            m_state.loadingContainerNavigationUrl()),
        m_presentationController.firstDisplayDecodeContext());
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
    finishContainerNavigationLoadWithError(
        containerUrl, imageViewText("The selected archive does not contain any supported images."));
}

void ImageOpenController::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, const QString &errorString)
{
    cancel();

    const QString message = errorString.isEmpty()
        ? imageViewText("Could not open the selected archive.")
        : errorString;
    reportEffects(
        ImageOpenWorkflow::finishContainerNavigationLoadWithError(m_state, containerUrl, message));
}

void ImageOpenController::setSourceUrlFromResolvedLoad(const QUrl &sourceUrl)
{
    m_state.setSourceUrl(sourceUrl);
}

void ImageOpenController::finishPredecodedImageLoad(ImageLoadSession session, PredecodedImage image)
{
    finishStaticImageLoad(session, std::move(image.staticImage), true);
    report(ImageDocumentEffect::scheduleAdjacentImagePredecode());
}

void ImageOpenController::finishDecodedImageLoad(
    ImageLoadSession session, std::shared_ptr<DecodedImage> image)
{
    auto handleDecoded
        = [this, &session](auto &decoded) { return finishDecodedImageResult(session, decoded); };
    const bool displayedImage = std::visit(handleDecoded, *image);
    if (displayedImage) {
        report(ImageDocumentEffect::scheduleAdjacentImagePredecode());
    }
}

bool ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, StaticDecodedImage &decoded)
{
    const bool predecodeCacheable
        = decoded.staticImage.byteCostWithinBudget(PredecodeCache::byteBudget()).has_value();
    finishStaticImageLoad(session, std::move(decoded.staticImage), predecodeCacheable);
    return true;
}

bool ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, DecodedAnimationImage &decoded)
{
    if (decoded.frames.empty()) {
        finishLoadWithError(session, ImageLoadError::Generic,
            imageViewText("Could not decode the selected image animation."));
        return false;
    }

    finishLoadSuccessfully(session, decoded.frames.front().image, false);
    m_presentationController.startDecodedAnimation(std::move(decoded.frames), decoded.loopCount);
    return true;
}

bool ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, ReaderAnimationImage &decoded)
{
    finishLoadSuccessfully(session, decoded.firstFrame, false);
    m_presentationController.startAnimation(
        decoded.data, decoded.format, decoded.loopCount, decoded.firstFrameDelay);
    return true;
}

bool ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, HeifSequenceAnimationImage &decoded)
{
    finishLoadSuccessfully(session, decoded.firstFrame, false);
    m_presentationController.startHeifSequenceAnimation(decoded.data, decoded.firstFrameDelay);
    return true;
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

void ImageOpenController::finishStaticImageLoad(
    const ImageLoadSession &session, StaticImagePayload staticImage, bool predecodeCacheable)
{
    beginSuccessfulImagePresentation(session, predecodeCacheable);
    m_presentationController.setStaticImage(std::move(staticImage));
    finishSuccessfulImagePresentation(session);
}

void ImageOpenController::finishLoadSuccessfully(
    const ImageLoadSession &session, const QImage &image, bool predecodeCacheable)
{
    beginSuccessfulImagePresentation(session, predecodeCacheable);
    m_presentationController.setImage(image);
    finishSuccessfulImagePresentation(session);
}

void ImageOpenController::beginSuccessfulImagePresentation(
    const ImageLoadSession &session, bool predecodeCacheable)
{
    m_presentationController.stopAnimation();
    const QUrl loadedContainerUrl = imageContainerUrlForLocation(session.location);
    m_presentationController.prepareImageContainer(loadedContainerUrl);
    m_presentationController.setPredecodeCacheable(predecodeCacheable);
}

void ImageOpenController::finishSuccessfulImagePresentation(const ImageLoadSession &session)
{
    m_presentationController.updateRenderContext();
    finishSuccessfulImageLoad(session);
}

void ImageOpenController::finishSuccessfulImageLoad(const ImageLoadSession &session)
{
    reportEffects(ImageOpenWorkflow::finishSuccessfulImageLoad(m_state, session));
}

void ImageOpenController::reportEffects(const ImageDocumentEffects &effects)
{
    for (const ImageDocumentEffect &effect : effects) {
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
