// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopencontroller.h"

#include "async/imagecallback.h"
#include "imagedocumentstate.h"
#include "imageloader.h"
#include "imageopentransitionapplier.h"
#include "imageopenworkflow.h"
#include "localization/imageerrortext.h"
#include "presentation/imagepresentationcontroller.h"
#include "presentation/imagepresentationload.h"

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
            [this](const ImageLoadSession &session, ImageLoadError error,
                const QString &errorString) { finishLoadWithError(session, error, errorString); },
            [this](ImageLoadSession session, DecodedImage image) {
                finishDecodedImageLoad(std::move(session), std::move(image));
            },
            [this](ImageLoadSession session, PredecodedImage image) {
                finishPredecodedImageLoad(std::move(session), std::move(image));
            },
            [this](const QUrl &url) {
                if (!m_callbacks.findPredecodedImage) {
                    return std::optional<PredecodedImage>();
                }

                return m_callbacks.findPredecodedImage(url);
            },
            [this](ImageLoadSession session) { finishSourceResolved(std::move(session)); },
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
    reportRuntimePlan(applyImageOpenApplicationPlan(
        m_state, ImageOpenWorkflow::finishAnimationLoadWithErrorPlan(message)));
}

void ImageOpenController::finishEmptySourceLoad()
{
    reportRuntimePlan(
        applyImageOpenApplicationPlan(m_state, ImageOpenWorkflow::finishEmptySourceLoadPlan()));
}

void ImageOpenController::beginSourceLoad()
{
    reportRuntimePlan(applyImageOpenApplicationPlan(m_state,
        ImageOpenWorkflow::beginSourceLoadPlan(ImageOpenBeginSourceLoadSnapshot {
            m_presentationController.hasImage(),
            !m_state.loadingContainerNavigationUrl().isEmpty(),
        })));
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
    reportRuntimePlan(applyImageOpenApplicationPlan(m_state,
        ImageOpenWorkflow::finishContainerNavigationLoadWithErrorPlan(containerUrl, message)));
}

void ImageOpenController::finishSourceResolved(ImageLoadSession session)
{
    reportRuntimePlan(
        applyImageOpenApplicationPlan(m_state, ImageOpenWorkflow::resolveSourceImagePlan(session)));
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
    reportRuntimePlan(applyImageOpenApplicationPlan(m_state,
        ImageOpenWorkflow::finishLoadWithErrorPlan(
            ImageOpenLoadErrorSnapshot {
                session.hasContainerNavigationTarget(),
                m_presentationController.hasImage(),
                !displayedUrl.isEmpty(),
            },
            session, displayedUrl, message)));
}

void ImageOpenController::finishSuccessfulImageLoad(const ImageLoadSession &session)
{
    reportRuntimePlan(applyImageOpenApplicationPlan(m_state,
        ImageOpenWorkflow::finishSuccessfulImageLoadPlan(
            ImageOpenSuccessfulImageLoadSnapshot {
                session.hasContainerNavigationTarget(),
            },
            session)));
}

void ImageOpenController::reportRuntimePlan(ImageDocumentRuntimePlan plan)
{
    invokeIfSet(m_callbacks.runtimePlan, plan);
}
}
