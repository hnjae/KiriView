// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagesecondarypagecontroller.h"

#include "decodedimagepresentation.h"
#include "imagecallback.h"
#include "imagecontainer.h"
#include "imageloader.h"
#include "imagepresentationcontroller.h"
#include "imagespreadgeometry.h"

#include <utility>
#include <variant>

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
                    const bool wasVisible = visible();
                    clear();
                    if (wasVisible) {
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

bool ImageSecondaryPageController::visible() const { return m_visible; }

const DisplayedImageLocation &ImageSecondaryPageController::displayedImageLocation() const
{
    return m_displayedImageLocation;
}

QSize ImageSecondaryPageController::imageSize() const
{
    return m_visible ? m_presentationController->imageSize() : QSize();
}

std::shared_ptr<DisplayedImageSurface> ImageSecondaryPageController::imageSurface() const
{
    return m_visible ? m_presentationController->imageSurface() : nullptr;
}

quint64 ImageSecondaryPageController::imageRevision() const
{
    return m_visible ? m_presentationController->imageRevision() : 0;
}

void ImageSecondaryPageController::cachePageSize(const QUrl &url, const QSize &imageSize)
{
    m_pageCache.cachePageSize(url, imageSize);
}

std::optional<bool> ImageSecondaryPageController::cachedPageIsWide(const QUrl &url) const
{
    return m_pageCache.cachedPageIsWide(url);
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
    m_displayedImageLocation = DisplayedImageLocation {};
    m_visible = false;
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
    finishStaticImageLoad(session, std::move(image.staticImage), true);
}

void ImageSecondaryPageController::finishDecodedImageLoad(
    ImageLoadSession session, DecodedImage image)
{
    DecodedImagePresentation presentation = decodedImagePresentationForImage(std::move(image));
    auto finishPresentation = [this, &session](auto &decodedPresentation) {
        return finishDecodedImagePresentation(session, decodedPresentation);
    };
    std::visit(finishPresentation, presentation);
}

bool ImageSecondaryPageController::finishDecodedImagePresentation(
    const ImageLoadSession &session, DecodedStaticImagePresentation &presentation)
{
    finishStaticImageLoad(
        session, std::move(presentation.staticImage), presentation.predecodeCacheable);
    return true;
}

bool ImageSecondaryPageController::finishDecodedImagePresentation(
    const ImageLoadSession &session, const DecodedAnimationImagePresentation &presentation)
{
    finishImageLoad(session, presentation.firstFrame, false);
    return true;
}

bool ImageSecondaryPageController::finishDecodedImagePresentation(
    const ImageLoadSession &session, const UnpresentableDecodedImage &)
{
    finishLoadWithError(session);
    return false;
}

void ImageSecondaryPageController::finishImageLoad(
    const ImageLoadSession &session, const QImage &image, bool predecodeCacheable)
{
    prepareImagePresentation(session);
    m_presentationController->setImage(image, predecodeCacheable);
    finishImagePresentation(session);
}

void ImageSecondaryPageController::finishStaticImageLoad(
    const ImageLoadSession &session, StaticImagePayload staticImage, bool predecodeCacheable)
{
    prepareImagePresentation(session);
    m_presentationController->setStaticImage(std::move(staticImage), predecodeCacheable);
    finishImagePresentation(session);
}

void ImageSecondaryPageController::prepareImagePresentation(const ImageLoadSession &session)
{
    m_presentationController->prepareImageContainer(zoomScopeUrlForLocation(session.location));
}

void ImageSecondaryPageController::finishImagePresentation(const ImageLoadSession &session)
{
    m_displayedImageLocation = session.location;
    const QSize loadedImageSize = m_presentationController->imageSize();
    if (imageSpreadPageIsWide(loadedImageSize)) {
        reportLoadFinished(
            ImageSecondaryPageLoadResult::PrimaryOnly, session.location, loadedImageSize);
        return;
    }

    m_visible = true;
    reportLoadFinished(ImageSecondaryPageLoadResult::Visible, session.location, loadedImageSize);
}

void ImageSecondaryPageController::finishLoadWithError(const ImageLoadSession &session)
{
    reportLoadFinished(ImageSecondaryPageLoadResult::Failed, session.location, QSize());
}

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
