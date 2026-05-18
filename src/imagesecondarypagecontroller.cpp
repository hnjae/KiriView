// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagesecondarypagecontroller.h"

#include "imagecallback.h"
#include "imageloader.h"
#include "imagepresentationcontroller.h"
#include "imagepresentationload.h"
#include "imagespreadgeometry.h"

#include <utility>

namespace KiriView {
ImageSecondaryPageController::ImageSecondaryPageController(QObject *parent,
    RenderContextProvider renderContextProvider, ImageSecondaryPageController::Callbacks callbacks,
    ImageNavigationCandidateProvider candidateProvider, ImageDecodeDependencies decodeDependencies)
    : m_callbacks(std::move(callbacks))
{
    m_presentationController
        = std::make_unique<ImagePresentationController>(parent, std::move(renderContextProvider),
            ImagePresentationController::Callbacks {
                [this](ImageDocumentChange change) {
                    if (change == ImageDocumentChange::Repaint) {
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
            });
    m_imageLoader = std::make_unique<ImageLoader>(parent, std::move(candidateProvider),
        std::move(decodeDependencies),
        ImageLoader::Callbacks {
            {},
            [this](ImageLoadSession session, ImageLoadError, const QString &) {
                finishLoadWithError(session);
            },
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

ImageSecondaryPageController::~ImageSecondaryPageController()
{
    cancel();
    stopAnimation();
}

ImagePresentationController &ImageSecondaryPageController::presentationController()
{
    return *m_presentationController;
}

const ImagePresentationController &ImageSecondaryPageController::presentationController() const
{
    return *m_presentationController;
}

bool ImageSecondaryPageController::visible() const { return m_displayedPage.has_value(); }

DisplayedImageLocation ImageSecondaryPageController::displayedImageLocation() const
{
    return m_displayedPage.has_value() ? m_displayedPage->location : DisplayedImageLocation {};
}

QSize ImageSecondaryPageController::imageSize() const
{
    return m_displayedPage.has_value() ? m_displayedPage->imageSize : QSize();
}

DisplayedImageRenderSnapshot ImageSecondaryPageController::renderSnapshot() const
{
    return visible() ? m_presentationController->renderSnapshot() : DisplayedImageRenderSnapshot {};
}

void ImageSecondaryPageController::setViewportSize(const QSizeF &viewportSize)
{
    m_presentationController->setViewportSize(viewportSize);
}

void ImageSecondaryPageController::updateRenderContext()
{
    m_presentationController->updateRenderContext();
}

void ImageSecondaryPageController::startLoad(const QUrl &url,
    const ArchiveDocumentLocation &displayedArchiveDocument,
    const ImageFirstDisplayDecodeContext &firstDisplayContext)
{
    clear();
    m_imageLoader->start(
        ImageLoadRequest::fromLocation(url, displayedArchiveDocument), firstDisplayContext);
}

void ImageSecondaryPageController::clear()
{
    cancel();
    stopAnimation();
    m_presentationController->clearImage();
    clearDisplayedPage();
}

void ImageSecondaryPageController::cancel()
{
    if (m_imageLoader != nullptr) {
        m_imageLoader->cancel();
    }
}

void ImageSecondaryPageController::stopAnimation()
{
    if (m_presentationController != nullptr) {
        m_presentationController->stopAnimation();
    }
}

void ImageSecondaryPageController::finishPredecodedImageLoad(
    ImageLoadSession session, PredecodedImage image)
{
    finishImagePresentation(
        session, presentPredecodedImageLoad(*m_presentationController, session, std::move(image)));
}

void ImageSecondaryPageController::finishDecodedImageLoad(
    ImageLoadSession session, DecodedImage image)
{
    finishImagePresentation(session,
        presentDecodedImageLoad(*m_presentationController, session, std::move(image),
            ImagePresentationAnimationHandling::FirstFrameOnly));
}

void ImageSecondaryPageController::finishImagePresentation(
    const ImageLoadSession &session, const ImagePresentationLoadResult &result)
{
    if (!result.presented) {
        finishLoadWithError(session);
        return;
    }

    if (imageSpreadPageIsWide(result.imageSize)) {
        m_presentationController->clearImage();
        clearDisplayedPage();
        reportLoadFinished(
            ImageSecondaryPageLoadResult::PrimaryOnly, session.location, result.imageSize);
        return;
    }

    showDisplayedPage(session.location, result.imageSize);
    reportLoadFinished(ImageSecondaryPageLoadResult::Visible, session.location, result.imageSize);
}

void ImageSecondaryPageController::finishLoadWithError(const ImageLoadSession &session)
{
    reportLoadFinished(ImageSecondaryPageLoadResult::Failed, session.location, QSize());
}

void ImageSecondaryPageController::showDisplayedPage(
    DisplayedImageLocation location, QSize imageSize)
{
    m_displayedPage = ImageSecondaryPageDisplayState { std::move(location), std::move(imageSize) };
}

void ImageSecondaryPageController::clearDisplayedPage() { m_displayedPage.reset(); }

void ImageSecondaryPageController::notify(ImageDocumentChange change)
{
    invokeIfSet(m_callbacks.change, change);
}

void ImageSecondaryPageController::reportLoadFinished(
    ImageSecondaryPageLoadResult result, const DisplayedImageLocation &location, QSize size)
{
    invokeIfSet(m_callbacks.loadFinished, result, location, size);
}
}
