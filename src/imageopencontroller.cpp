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

QString animationLoadErrorMessage(const QString &errorString)
{
    return errorString.isEmpty()
        ? KiriView::imageViewText("Could not decode the selected image animation.")
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

void ImageOpenController::finishAnimationLoadWithError(const QString &errorString)
{
    const QString message = animationLoadErrorMessage(errorString);
    reportEffects(ImageOpenWorkflow::finishAnimationLoadWithError(m_state, message));
}

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
    DecodedImagePresentation presentation = decodedImagePresentationForImage(std::move(image));
    auto finishPresentation = [this, &session](auto &decodedPresentation) {
        return finishDecodedImagePresentation(session, decodedPresentation);
    };
    std::visit(finishPresentation, presentation);
}

bool ImageOpenController::finishDecodedImagePresentation(
    const ImageLoadSession &session, DecodedStaticImagePresentation &presentation)
{
    finishStaticImageLoad(
        session, std::move(presentation.staticImage), presentation.predecodeCacheable);
    return true;
}

bool ImageOpenController::finishDecodedImagePresentation(
    const ImageLoadSession &session, DecodedAnimationImagePresentation &presentation)
{
    const QImage firstFrame = presentation.firstFrame;
    DecodedAnimationImagePresentation animation = std::move(presentation);
    return finishAnimationImageLoad(session, firstFrame,
        [this, animation = std::move(animation)]() { startDecodedAnimation(animation); });
}

bool ImageOpenController::finishDecodedImagePresentation(
    const ImageLoadSession &session, const UnpresentableDecodedImage &)
{
    finishLoadWithError(session, ImageLoadError::Generic,
        imageViewText("Could not decode the selected image animation."));
    return false;
}

void ImageOpenController::startDecodedAnimation(
    const DecodedAnimationImagePresentation &presentation)
{
    switch (presentation.kind) {
    case DecodedImageAnimationKind::Apng:
        m_presentationController.startApngAnimation(
            presentation.data, presentation.loopCount, presentation.firstFrameDelay);
        return;
    case DecodedImageAnimationKind::Reader:
        m_presentationController.startAnimation(presentation.data, presentation.format,
            presentation.loopCount, presentation.firstFrameDelay);
        return;
    case DecodedImageAnimationKind::HeifSequence:
        m_presentationController.startHeifSequenceAnimation(presentation.data);
        return;
    }
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
    const QString message = loadErrorMessage(error, errorString);
    reportEffects(ImageOpenWorkflow::finishLoadWithError(
        m_state, session, m_presentationController.hasImage(), message));
}

void ImageOpenController::finishStaticImageLoad(
    const ImageLoadSession &session, StaticImagePayload staticImage, bool predecodeCacheable)
{
    beginSuccessfulImagePresentation(session);
    m_presentationController.setStaticImage(std::move(staticImage), predecodeCacheable);
    finishSuccessfulImagePresentation(session);
}

void ImageOpenController::finishLoadSuccessfully(
    const ImageLoadSession &session, const QImage &image, bool predecodeCacheable)
{
    beginSuccessfulImagePresentation(session);
    m_presentationController.stopAnimation();
    m_presentationController.setImage(image, predecodeCacheable);
    finishSuccessfulImagePresentation(session);
}

void ImageOpenController::beginSuccessfulImagePresentation(const ImageLoadSession &session)
{
    const QUrl loadedZoomScopeUrl = zoomScopeUrlForLocation(session.location);
    m_presentationController.prepareImageContainer(loadedZoomScopeUrl);
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
