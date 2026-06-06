// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageopencontroller.h"

#include "async/imagecallback.h"
#include "imagedocumentstate.h"
#include "imageloader.h"
#include "imageopentransitionapplier.h"
#include "imageopenworkflow.h"
#include "localization/imageerrortext.h"
#include "location/imagedocumentlocation.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagepresentationload.h"
#include "presentation/imagepresentationruntime.h"

#include <KLocalizedString>
#include <memory>
#include <utility>

namespace {
QString emptyOpenedCollectionErrorMessage()
{
    return KiriView::imageErrorText(KiriView::ImageErrorTextId::EmptyOpenedCollection);
}

QString openedCollectionOpenErrorMessage(const QString &errorString)
{
    return errorString.isEmpty()
        ? KiriView::imageErrorText(KiriView::ImageErrorTextId::OpenOpenedCollection)
        : errorString;
}

QString loadErrorMessage(KiriView::ImageLoadError error, const QString &errorString)
{
    return error == KiriView::ImageLoadError::EmptyOpenedCollection
        ? emptyOpenedCollectionErrorMessage()
        : errorString;
}

QString animationLoadErrorMessage(const QString &errorString)
{
    return errorString.isEmpty()
        ? KiriView::imageErrorText(KiriView::ImageErrorTextId::DecodeImageAnimation)
        : errorString;
}

QString unsupportedOpenedCollectionVideoMessage()
{
    return i18nc("@info:status",
        "KiriView can’t play videos inside directly opened archives or directories yet.");
}
}

namespace KiriView {
ImageOpenController::ImageOpenController(QObject *parent, ImageDocumentState &state,
    ImagePageSurfaceController &pageSurfaceController,
    ImagePresentationRuntime &presentationRuntime, ImageOpenController::Callbacks callbacks,
    ImageDocumentPageCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies)
    : m_state(state)
    , m_pageSurfaceController(pageSurfaceController)
    , m_presentationRuntime(presentationRuntime)
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
            [this](ImageLoadSession session, StaticDisplayImagePayload preview) {
                finishThumbnailPreviewLoad(std::move(session), std::move(preview));
            },
            [this](ImageLoadSession session) {
                finishUnsupportedOpenedCollectionVideoLoad(std::move(session));
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

    if (m_state.sourceUrl().isEmpty()) {
        finishEmptySourceLoad();
        return;
    }

    const ImageLoadRequest request = ImageLoadRequest::fromTarget(
        ImageDocumentPageTarget { m_state.sourceUrl(), m_state.sourceKind() },
        m_state.displayedOpenedCollectionScope(), m_state.loadingContainerNavigationUrl());
    beginSourceLoad();
    m_imageLoader->start(request, m_presentationRuntime.firstDisplayDecodeContext());
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
    m_pageSurfaceController.clearShadowDisplayImage();
    reportRuntimePlan(applyImageOpenApplicationPlan(m_state,
        ImageOpenWorkflow::beginSourceLoadPlan(ImageOpenBeginSourceLoadSnapshot {
            m_pageSurfaceController.hasImage(),
            !m_state.loadingContainerNavigationUrl().isEmpty(),
        })));
}

void ImageOpenController::finishContainerNavigationWithEmptyContainer(const QUrl &containerUrl)
{
    finishContainerNavigationLoadWithError(containerUrl, emptyOpenedCollectionErrorMessage());
}

void ImageOpenController::finishContainerNavigationLoadWithError(
    const QUrl &containerUrl, const QString &errorString)
{
    cancel();

    const QString message = openedCollectionOpenErrorMessage(errorString);
    reportRuntimePlan(applyImageOpenApplicationPlan(m_state,
        ImageOpenWorkflow::finishContainerNavigationLoadWithErrorPlan(containerUrl, message)));
}

void ImageOpenController::finishSourceResolved(ImageLoadSession session)
{
    reportRuntimePlan(
        applyImageOpenApplicationPlan(m_state, ImageOpenWorkflow::resolveSourceImagePlan(session)));
}

void ImageOpenController::finishUnsupportedOpenedCollectionVideoLoad(ImageLoadSession session)
{
    const QString message = unsupportedOpenedCollectionVideoMessage();
    m_pageSurfaceController.clearImage();
    invokeIfSet(m_callbacks.clearPrimaryPageSlot);
    const ImageDocumentRuntimePlan plan = applyImageOpenApplicationPlan(
        m_state, ImageOpenWorkflow::finishUnsupportedOpenedCollectionVideoLoadPlan(session));
    invokeIfSet(m_callbacks.unsupportedOpenedCollectionVideoEntered, message);
    reportRuntimePlan(plan);
}

void ImageOpenController::finishThumbnailPreviewLoad(
    ImageLoadSession session, StaticDisplayImagePayload preview)
{
    Q_UNUSED(session);
    m_pageSurfaceController.publishShadowDisplayImage(std::move(preview));
}

void ImageOpenController::finishPredecodedImageLoad(ImageLoadSession session, PredecodedImage image)
{
    EmbeddedMetadata metadata = image.embeddedMetadata;
    finishPresentedImageLoad(session,
        presentPredecodedImageLoad(
            m_pageSurfaceController, std::move(image), m_presentationRuntime.renderContext()),
        std::move(metadata));
}

void ImageOpenController::finishDecodedImageLoad(ImageLoadSession session, DecodedImage image)
{
    EmbeddedMetadata metadata = decodedImageEmbeddedMetadata(image);
    finishPresentedImageLoad(session,
        presentDecodedImageLoad(m_pageSurfaceController, std::move(image),
            ImagePresentationAnimationHandling::StartAnimation,
            m_presentationRuntime.renderContext()),
        std::move(metadata));
}

void ImageOpenController::finishPresentedImageLoad(const ImageLoadSession &session,
    const ImagePresentationLoadResult &result, EmbeddedMetadata metadata)
{
    if (!result.presented) {
        finishLoadWithError(session, ImageLoadError::Generic,
            imageErrorText(ImageErrorTextId::DecodeImageAnimation));
        return;
    }

    invokeIfSet(m_callbacks.commitPrimaryPageSlot, session.location());
    m_state.setEmbeddedMetadata(std::move(metadata));
    finishSuccessfulImageLoad(session);
}

void ImageOpenController::finishLoadWithError(
    const ImageLoadSession &session, ImageLoadError error, const QString &errorString)
{
    m_pageSurfaceController.clearShadowDisplayImage();
    const QString message = loadErrorMessage(error, errorString);
    const QUrl displayedUrl = m_state.displayedUrl();
    reportRuntimePlan(applyImageOpenApplicationPlan(m_state,
        ImageOpenWorkflow::finishLoadWithErrorPlan(
            ImageOpenLoadErrorSnapshot {
                session.hasContainerNavigationTarget(),
                m_pageSurfaceController.hasImage(),
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
