// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopencontroller.h"

#include "decodedimagepresentation.h"
#include "imagecallback.h"
#include "imagecontainer.h"
#include "imagedocumentstate.h"
#include "imageloader.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imageviewtext.h"

#include <memory>
#include <utility>
#include <variant>

namespace {
QString emptyArchiveErrorMessage()
{
    return KiriView::imageViewText("The selected archive does not contain any supported images.");
}

QString archiveOpenErrorMessage(const QString &errorString)
{
    return errorString.isEmpty() ? KiriView::imageViewText("Could not open the selected archive.")
                                 : errorString;
}

QString loadErrorMessage(KiriView::ImageLoadError error, const QString &errorString)
{
    return error == KiriView::ImageLoadError::EmptyArchive ? emptyArchiveErrorMessage()
                                                           : errorString;
}
}

namespace KiriView {
ImageOpenController::ImageOpenController(QObject *parent, ImageDocumentState &state,
    ImagePresentationController &presentationController, ImageOpenController::Callbacks callbacks,
    ImageNavigationCandidateProvider candidateProvider, ImageDecodeDependencies decodeDependencies)
    : m_state(state)
    , m_presentationController(presentationController)
    , m_callbacks(std::move(callbacks))
{
    m_imageLoader = std::make_unique<ImageLoader>(parent, std::move(candidateProvider),
        std::move(decodeDependencies),
        ImageLoader::Callbacks {
            [this](const QUrl &sourceUrl) { setSourceUrlFromResolvedLoad(sourceUrl); },
            [this](const ImageLoadSession &session, ImageLoadError error,
                const QString &errorString) { finishLoadWithError(session, error, errorString); },
            [this](ImageLoadSession session, DecodedImage image) {
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

    switch (ImageOpenWorkflow::sourceTargetForOpen(m_state)) {
    case ImageOpenSourceTarget::EmptySource:
        finishEmptySourceLoad();
        return;
    case ImageOpenSourceTarget::LoadSource:
        break;
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
    finishContainerNavigationLoadWithError(containerUrl, emptyArchiveErrorMessage());
}

void ImageOpenController::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, const QString &errorString)
{
    cancel();

    const QString message = archiveOpenErrorMessage(errorString);
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
}

void ImageOpenController::finishDecodedImageLoad(ImageLoadSession session, DecodedImage image)
{
    auto handleDecoded
        = [this, &session](auto &decoded) { return finishDecodedImageResult(session, decoded); };
    std::visit(handleDecoded, image);
}

bool ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, StaticDecodedImage &decoded)
{
    const DecodedImagePresentationPlan plan = decodedImagePresentationPlan(decoded);
    finishStaticImageLoad(session, std::move(decoded.staticImage), plan.predecodeCacheable);
    return true;
}

bool ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, DecodedAnimationImage &decoded)
{
    const DecodedImagePresentationPlan plan = decodedImagePresentationPlan(decoded);
    if (plan.target == DecodedImagePresentationTarget::DecodeError) {
        finishLoadWithError(session, ImageLoadError::Generic,
            imageViewText("Could not decode the selected image animation."));
        return false;
    }

    return finishAnimationImageLoad(session, decoded.frames.front().image, [this, &decoded]() {
        m_presentationController.startDecodedAnimation(
            std::move(decoded.frames), decoded.loopCount);
    });
}

bool ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, ReaderAnimationImage &decoded)
{
    const DecodedImagePresentationPlan plan = decodedImagePresentationPlan(decoded);
    if (plan.target == DecodedImagePresentationTarget::DecodeError) {
        finishLoadWithError(session, ImageLoadError::Generic,
            imageViewText("Could not decode the selected image animation."));
        return false;
    }

    return finishAnimationImageLoad(session, decoded.firstFrame, [this, &decoded]() {
        m_presentationController.startAnimation(
            decoded.data, decoded.format, decoded.loopCount, decoded.firstFrameDelay);
    });
}

bool ImageOpenController::finishDecodedImageResult(
    ImageLoadSession &session, HeifSequenceAnimationImage &decoded)
{
    const DecodedImagePresentationPlan plan = decodedImagePresentationPlan(decoded);
    if (plan.target == DecodedImagePresentationTarget::DecodeError) {
        finishLoadWithError(session, ImageLoadError::Generic,
            imageViewText("Could not decode the selected image animation."));
        return false;
    }

    return finishAnimationImageLoad(session, decoded.firstFrame,
        [this, &decoded]() { m_presentationController.startHeifSequenceAnimation(decoded.data); });
}

bool ImageOpenController::finishAnimationImageLoad(
    const ImageLoadSession &session, const QImage &firstFrame, std::function<void()> start)
{
    finishLoadSuccessfully(session, firstFrame, false);
    start();
    return true;
}

void ImageOpenController::finishLoadWithError(
    const ImageLoadSession &session, ImageLoadError error, const QString &errorString)
{
    const QUrl containerNavigationUrl = session.request.containerNavigationUrl();
    const QString message = loadErrorMessage(error, errorString);

    switch (ImageOpenWorkflow::failureTargetForLoadError(
        session, m_presentationController.hasImage())) {
    case ImageOpenFailureTarget::ContainerNavigation:
        finishContainerNavigationLoadWithError(containerNavigationUrl, message);
        return;
    case ImageOpenFailureTarget::Replacement:
        finishReplacementLoadWithError(message);
        return;
    case ImageOpenFailureTarget::Initial:
        finishInitialLoadWithError(message);
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
    m_presentationController.stopAnimation();
    m_presentationController.setImage(image);
    finishSuccessfulImagePresentation(session);
}

void ImageOpenController::beginSuccessfulImagePresentation(
    const ImageLoadSession &session, bool predecodeCacheable)
{
    const QUrl loadedZoomScopeUrl = zoomScopeUrlForLocation(session.location);
    m_presentationController.prepareImageContainer(loadedZoomScopeUrl);
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

void ImageOpenController::reportEffects(ImageDocumentEffects effects)
{
    for (ImageDocumentEffect &effect : effects) {
        report(std::move(effect));
    }
}

void ImageOpenController::report(ImageDocumentEffect effect)
{
    invokeIfSet(m_callbacks.effect, std::move(effect));
}
}
