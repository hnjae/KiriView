// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopencontroller.h"

#include "imagecallback.h"
#include "imagedocumentstate.h"
#include "imageerrortext.h"
#include "imageloader.h"
#include "imageopentransitionapplier.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imagepresentationload.h"

#include <memory>
#include <utility>

namespace {
QString emptyArchiveErrorMessage()
{
    return KiriView::imageErrorText(KiriView::ImageErrorTextId::EmptyArchive);
}

QString archiveOpenErrorMessage(const QString &errorString)
{
    return errorString.isEmpty() ? KiriView::imageErrorText(KiriView::ImageErrorTextId::OpenArchive)
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
        ? KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeImageAnimation)
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
    reportEffects(applyImageOpenTransition(m_state,
        ImageOpenWorkflow::finishAnimationLoadWithErrorTransition(),
        ImageOpenTransitionContext::animationError(message)));
}

void ImageOpenController::finishEmptySourceLoad()
{
    reportEffects(
        applyImageOpenTransition(m_state, ImageOpenWorkflow::finishEmptySourceLoadTransition()));
}

void ImageOpenController::beginSourceLoad()
{
    reportEffects(applyImageOpenTransition(m_state,
        ImageOpenWorkflow::beginSourceLoadTransition(m_presentationController.hasImage(),
            !m_state.loadingContainerNavigationUrl().isEmpty())));
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
    reportEffects(applyImageOpenTransition(m_state,
        ImageOpenWorkflow::finishContainerNavigationLoadWithErrorTransition(),
        ImageOpenTransitionContext::containerNavigationError(containerUrl, message)));
}

void ImageOpenController::setSourceUrlFromResolvedLoad(const QUrl &sourceUrl)
{
    m_state.setSourceUrl(sourceUrl);
}

void ImageOpenController::finishPredecodedImageLoad(ImageLoadSession session, PredecodedImage image)
{
    finishPresentedImageLoad(
        session, presentPredecodedImageLoad(m_presentationController, session, std::move(image)));
}

void ImageOpenController::finishDecodedImageLoad(ImageLoadSession session, DecodedImage image)
{
    finishPresentedImageLoad(session,
        presentDecodedImageLoad(m_presentationController, session, std::move(image),
            ImagePresentationAnimationHandling::StartAnimation));
}

void ImageOpenController::finishPresentedImageLoad(
    const ImageLoadSession &session, const ImagePresentationLoadResult &result)
{
    if (!result.presented) {
        finishLoadWithError(session, ImageLoadError::Generic,
            imageErrorText(ImageErrorTextId::DecodeImageAnimation));
        return;
    }

    finishSuccessfulImageLoad(session);
}

void ImageOpenController::finishLoadWithError(
    const ImageLoadSession &session, ImageLoadError error, const QString &errorString)
{
    const QString message = loadErrorMessage(error, errorString);
    const QUrl displayedUrl = m_state.displayedUrl();
    reportEffects(applyImageOpenTransition(m_state,
        ImageOpenWorkflow::finishLoadWithErrorTransition(
            session, m_presentationController.hasImage(), !displayedUrl.isEmpty()),
        ImageOpenTransitionContext::sourceLoadError(session, displayedUrl, message)));
}

void ImageOpenController::finishSuccessfulImageLoad(const ImageLoadSession &session)
{
    reportEffects(applyImageOpenTransition(m_state,
        ImageOpenWorkflow::finishSuccessfulImageLoadTransition(session),
        ImageOpenTransitionContext::successfulImageLoad(session)));
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
