// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagesecondarypagecontroller.h"

#include "async/imagecallback.h"
#include "document/imageloader.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagepresentationload.h"
#include "presentation/imagespreadgeometry.h"

#include <utility>

namespace KiriView {
ImageSecondaryPageController::ImageSecondaryPageController(QObject *parent,
    RenderContextProvider renderContextProvider, ImageSecondaryPageController::Callbacks callbacks,
    ImageDocumentPageCandidateProvider candidateProvider,
    ImageDecodeDependencies decodeDependencies, ImageCacheBudgets cacheBudgets)
    : m_callbacks(std::move(callbacks))
    , m_renderContextProvider(std::move(renderContextProvider))
{
    m_pageSurfaceController = std::make_unique<ImagePageSurfaceController>(parent,
        ImagePageSurfaceController::Callbacks {
            [this](ImageDocumentChange change) {
                if (change == ImageDocumentChange::DisplaySource) {
                    notify(change);
                }
            },
            [this](const QString &) {
                const bool hadDisplayedPage = visible();
                clear();
                if (hadDisplayedPage) {
                    invokeIfSet(m_callbacks.visibilityChanged);
                }
            },
        },
        cacheBudgets, std::shared_ptr<DisplayImageStore> {}, DisplayedPageRole::Secondary);
    m_imageLoader = std::make_unique<ImageLoader>(parent, std::move(candidateProvider),
        std::move(decodeDependencies),
        ImageLoader::Callbacks {
            [this](ImageLoadSession session, ImageLoadError, const QString &) {
                finishLoadWithError(session);
            },
            [this](ImageLoadSession session, DecodedImage image) {
                finishDecodedImageLoad(std::move(session), std::move(image));
            },
            [this](ImageLoadSession session, PredecodedImage image) {
                finishPredecodedImageLoad(std::move(session), std::move(image));
            },
            {},
            {},
            [this](const QUrl &url) {
                if (!m_callbacks.findPredecodedImage) {
                    return std::optional<PredecodedImage>();
                }

                return m_callbacks.findPredecodedImage(url);
            },
            {},
        });
}

ImageSecondaryPageController::~ImageSecondaryPageController()
{
    cancel();
    stopAnimation();
}

ImagePageSurfaceController &ImageSecondaryPageController::pageSurfaceController()
{
    return *m_pageSurfaceController;
}

const ImagePageSurfaceController &ImageSecondaryPageController::pageSurfaceController() const
{
    return *m_pageSurfaceController;
}

bool ImageSecondaryPageController::visible() const { return m_displayState.visible(); }

DisplayedImageLocation ImageSecondaryPageController::displayedImageLocation() const
{
    return m_displayState.displayedImageLocation();
}

QSize ImageSecondaryPageController::imageSize() const { return m_displayState.imageSize(); }

ImagePresentationPageSlotSnapshot ImageSecondaryPageController::pageSlotSnapshot() const
{
    return visible() ? m_pageSurfaceController->snapshot() : ImagePresentationPageSlotSnapshot {};
}

void ImageSecondaryPageController::startLoad(const QUrl &url,
    const OpenedCollectionScopeLocation &displayedOpenedCollectionScope,
    const ImageFirstDisplayDecodeContext &firstDisplayContext)
{
    cancel();
    stopAnimation();
    m_imageLoader->start(
        ImageLoadRequest::fromLocation(url, displayedOpenedCollectionScope), firstDisplayContext);
}

void ImageSecondaryPageController::clear()
{
    cancel();
    stopAnimation();
    m_pageSurfaceController->clearImage();
    m_displayState.clear();
}

void ImageSecondaryPageController::cancel()
{
    if (m_imageLoader != nullptr) {
        m_imageLoader->cancel();
    }
}

void ImageSecondaryPageController::stopAnimation()
{
    if (m_pageSurfaceController != nullptr) {
        m_pageSurfaceController->stopAnimation();
    }
}

void ImageSecondaryPageController::finishPredecodedImageLoad(
    ImageLoadSession session, PredecodedImage image)
{
    finishImagePresentation(session,
        presentPredecodedImageLoad(*m_pageSurfaceController, std::move(image), renderContext()));
}

void ImageSecondaryPageController::finishDecodedImageLoad(
    ImageLoadSession session, DecodedImage image)
{
    finishImagePresentation(session,
        presentDecodedImageLoad(*m_pageSurfaceController, std::move(image),
            ImagePresentationAnimationHandling::FirstFrameOnly, renderContext()));
}

void ImageSecondaryPageController::finishImagePresentation(
    const ImageLoadSession &session, const ImagePresentationLoadResult &result)
{
    if (!result.presented) {
        finishLoadWithError(session);
        return;
    }

    applyLoadCompletion(m_displayState.finishPresentedLoad(
        session.location(), result.imageSize, imageSpreadPageIsWide(result.imageSize)));
}

void ImageSecondaryPageController::finishLoadWithError(const ImageLoadSession &session)
{
    applyLoadCompletion(m_displayState.finishFailedLoad(session.location()));
}

void ImageSecondaryPageController::applyLoadCompletion(
    const ImageSecondaryPageLoadCompletion &completion)
{
    if (completion.clearPresentation) {
        m_pageSurfaceController->clearImage();
    }

    invokeIfSet(
        m_callbacks.loadFinished, completion.result, completion.location, completion.imageSize);
}

ImageDocumentRenderContext ImageSecondaryPageController::renderContext() const
{
    return m_renderContextProvider ? m_renderContextProvider() : ImageDocumentRenderContext {};
}

void ImageSecondaryPageController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_callbacks.change, change);
}
}
