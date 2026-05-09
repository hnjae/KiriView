// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcontroller.h"

#include "imagecallback.h"
#include "imagecontainer.h"
#include "imagedeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imageloader.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepredecodecoordinator.h"
#include "imagepresentationcontroller.h"
#include "imagerendering.h"
#include "imageviewtext.h"
#include "predecodecache.h"

#include <QRectF>
#include <QString>
#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace KiriView {
ImageDocumentController::ImageDocumentController(
    QObject *parent, RenderContextProvider renderContextProvider, ChangeCallback changeCallback)
    : ImageDocumentController(parent, std::move(renderContextProvider), std::move(changeCallback),
          ImageAsyncDependencies {})
{
}

ImageDocumentController::ImageDocumentController(QObject *parent,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    ImageAsyncDependencies dependencies, FileDeletionFailedCallback fileDeletionFailedCallback)
    : QObject(parent)
    , m_changeCallback(std::move(changeCallback))
    , m_renderContextProvider(renderContextProvider)
    , m_state([this](ImageDocumentChange change) { notify(change); })
{
    dependencies = imageAsyncDependenciesWithDefaults(std::move(dependencies));
    RenderContextProvider primaryRenderContextProvider = renderContextProvider;
    RenderContextProvider secondaryRenderContextProvider = std::move(renderContextProvider);
    m_deletionController = std::make_unique<ImageDeletionController>(this,
        dependencies.candidateProvider, std::move(dependencies.fileOperations),
        ImageDeletionController::Callbacks {
            [this]() { notify(ImageDocumentChange::FileDeletionInProgress); },
            [this]() { clearAfterSuccessfulFileDeletion(); },
            [this](const QUrl &url) { setSourceUrl(url); },
            [this](const QUrl &imageUrl, const QUrl &containerUrl) {
                setSourceUrlForLoad(imageUrl, containerUrl);
            },
            std::move(fileDeletionFailedCallback),
        });
    m_presentationController = std::make_unique<ImagePresentationController>(this,
        std::move(primaryRenderContextProvider),
        ImagePresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QString &errorString) {
                dispatchEffect(ImageDocumentEffect::animationFailed(errorString));
            },
        });
    m_secondaryPresentationController = std::make_unique<ImagePresentationController>(this,
        std::move(secondaryRenderContextProvider),
        ImagePresentationController::Callbacks {
            [this](ImageDocumentChange change) {
                if (change == ImageDocumentChange::Repaint) {
                    notify(change);
                }
            },
            [this](const QString &) { clearSecondaryPageAndNotify(); },
        });
    m_openController
        = std::make_unique<ImageOpenController>(this, m_state, *m_presentationController,
            ImageOpenController::Callbacks {
                [this](const QUrl &url) { return takePredecodedImage(url); },
                [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            },
            dependencies.candidateProvider, dependencies.imageDecode);
    m_secondaryImageLoader = std::make_unique<ImageLoader>(this, dependencies.candidateProvider,
        dependencies.imageDecode,
        ImageLoader::Callbacks {
            {},
            [this](ImageLoadSession session, ImageLoadError, const QString &) {
                finishSecondaryLoadWithError(session);
            },
            [this](ImageLoadSession session, DecodedImage image) {
                finishSecondaryDecodedImageLoad(std::move(session), std::move(image));
            },
            [this](ImageLoadSession session, PredecodedImage image) {
                finishSecondaryPredecodedImageLoad(std::move(session), std::move(image));
            },
            [this](const QUrl &url) { return takePredecodedImage(url); },
        });
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(this, m_state,
        *m_presentationController,
        ImageDocumentNavigationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
        },
        dependencies.candidateProvider);
    m_predecodeCoordinator = std::make_unique<ImagePredecodeCoordinator>(
        this, dependencies.candidateProvider, dependencies.imageDecode);
}

ImageDocumentController::~ImageDocumentController()
{
    m_deletionController->cancel();
    m_presentationController->stopAnimation();
    m_secondaryPresentationController->stopAnimation();
    cancelPredecode();
    m_secondaryImageLoader->cancel();
    m_navigationController->cancelPageNavigationUpdate();
    m_navigationController->cancelContainerNavigation();
    m_navigationController->cancelNavigation();
    m_openController->cancel();
}

QUrl ImageDocumentController::sourceUrl() const { return m_state.sourceUrl(); }

void ImageDocumentController::setSourceUrl(const QUrl &sourceUrl)
{
    setSourceUrlForLoad(sourceUrl, QUrl());
}

ImageDocumentStatus ImageDocumentController::status() const
{
    if (m_twoPageSpreadTransitionInProgress) {
        return ImageDocumentStatus::Loading;
    }

    return m_state.status();
}

bool ImageDocumentController::loading() const
{
    return m_twoPageSpreadTransitionInProgress || m_state.loading();
}

QString ImageDocumentController::errorString() const { return m_state.errorString(); }

QString ImageDocumentController::windowTitleFileName() const
{
    return m_state.windowTitleFileName();
}

QUrl ImageDocumentController::displayedUrl() const { return m_state.displayedUrl(); }

QSize ImageDocumentController::imageSize() const
{
    return secondaryPageVisible() ? spreadImageSize() : m_presentationController->imageSize();
}

QSize ImageDocumentController::primaryImageSize() const
{
    return m_presentationController->imageSize();
}

QSize ImageDocumentController::secondaryImageSize() const
{
    return secondaryPageVisible() ? m_secondaryPresentationController->imageSize() : QSize();
}

QSizeF ImageDocumentController::viewportSize() const
{
    return m_presentationController->viewportSize();
}

void ImageDocumentController::setViewportSize(const QSizeF &viewportSize)
{
    m_presentationController->setViewportSize(viewportSize);
    m_secondaryPresentationController->setViewportSize(viewportSize);
    if (secondaryPageVisible()) {
        updateSpreadZoomState();
    }
}

QRectF ImageDocumentController::visibleItemRect() const
{
    return secondaryPageVisible() ? m_visibleItemRect : m_presentationController->visibleItemRect();
}

void ImageDocumentController::setVisibleItemRect(const QRectF &visibleItemRect)
{
    m_visibleItemRect = visibleItemRect;
    if (secondaryPageVisible()) {
        applySpreadVisibleItemRects();
        notify(ImageDocumentChange::VisibleItemRect);
        return;
    }

    m_presentationController->setVisibleItemRect(visibleItemRect);
}

QSizeF ImageDocumentController::displaySize() const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomState.displaySize();
    }

    return m_presentationController->displaySize();
}

QSizeF ImageDocumentController::primaryDisplaySize() const
{
    if (!secondaryPageVisible()) {
        return m_presentationController->displaySize();
    }

    const QSize imageSize = m_presentationController->imageSize();
    if (imageSize.isEmpty() || spreadImageSize().isEmpty()) {
        return {};
    }

    const qreal scale = displaySize().width() / spreadImageSize().width();
    return QSizeF(imageSize.width() * scale, imageSize.height() * scale);
}

QSizeF ImageDocumentController::secondaryDisplaySize() const
{
    if (!secondaryPageVisible()) {
        return {};
    }

    const QSize imageSize = m_secondaryPresentationController->imageSize();
    if (imageSize.isEmpty() || spreadImageSize().isEmpty()) {
        return {};
    }

    const qreal scale = displaySize().width() / spreadImageSize().width();
    return QSizeF(imageSize.width() * scale, imageSize.height() * scale);
}

qreal ImageDocumentController::zoomPercent() const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomState.zoomPercent();
    }

    return m_presentationController->zoomPercent();
}

void ImageDocumentController::setZoomPercent(qreal zoomPercent)
{
    if (secondaryPageVisible()) {
        m_spreadZoomState.setManualZoomPercent(zoomPercent, spreadDevicePixelRatio());
        m_presentationController->setZoomPercent(m_spreadZoomState.zoomPercent());
        m_secondaryPresentationController->setZoomPercent(m_spreadZoomState.zoomPercent());
        applySpreadVisibleItemRects();
        notify(ImageDocumentChange::ZoomMode);
        notify(ImageDocumentChange::ZoomPercent);
        notify(ImageDocumentChange::DisplaySize);
        notify(ImageDocumentChange::MaximumManualZoomPercent);
        notify(ImageDocumentChange::Repaint);
        notify(ImageDocumentChange::TwoPageMode);
        return;
    }

    m_presentationController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentController::zoomMode() const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomState.zoomMode();
    }

    return m_presentationController->zoomMode();
}

qreal ImageDocumentController::maximumManualZoomPercent() const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomState.maximumManualZoomPercent(spreadDevicePixelRatio());
    }

    return m_presentationController->maximumManualZoomPercent();
}

qreal ImageDocumentController::clampedManualZoomPercent(qreal zoomPercent) const
{
    if (secondaryPageVisible()) {
        return m_spreadZoomState.clampedManualZoomPercent(zoomPercent, spreadDevicePixelRatio());
    }

    return m_presentationController->clampedManualZoomPercent(zoomPercent);
}

int ImageDocumentController::currentPageNumber() const
{
    return m_navigationController->currentPageNumber();
}

int ImageDocumentController::currentLastPageNumber() const
{
    const int current = currentPageNumber();
    if (current <= 0) {
        return 0;
    }

    return secondaryPageVisible() ? current + 1 : current;
}

int ImageDocumentController::imageCount() const { return m_navigationController->imageCount(); }

bool ImageDocumentController::containerNavigationAvailable() const
{
    return m_state.containerNavigationAvailable();
}

bool ImageDocumentController::fileDeletionInProgress() const
{
    return m_deletionController->inProgress();
}

bool ImageDocumentController::twoPageModeEnabled() const { return m_twoPageModeEnabled; }

void ImageDocumentController::setTwoPageModeEnabled(bool enabled)
{
    if (m_twoPageModeEnabled == enabled) {
        return;
    }

    const bool wasSecondaryVisible = secondaryPageVisible();
    const ImageZoomMode previousZoomMode = zoomMode();
    const qreal previousZoomPercent = zoomPercent();
    m_twoPageModeEnabled = enabled;
    if (m_twoPageModeEnabled) {
        m_spreadZoomState = ImageZoomState {};
    }
    if (!m_twoPageModeEnabled) {
        finishTwoPageSpreadTransition();
        clearSecondaryPage();
        if (wasSecondaryVisible) {
            if (previousZoomMode == ImageZoomMode::Manual) {
                m_presentationController->setZoomPercent(previousZoomPercent);
            } else if (previousZoomMode == ImageZoomMode::Fit) {
                m_presentationController->resetZoom();
            } else {
                m_presentationController->setFitMode(previousZoomMode);
            }
        }
    }
    refreshSecondaryPage();
    notifyTwoPageModeChanged();
}

bool ImageDocumentController::twoPageModeAvailable() const
{
    return m_presentationController->hasImage() && !m_state.displayedUrl().isEmpty()
        && m_state.displayedArchiveDocument().isComicBook();
}

bool ImageDocumentController::secondaryPageVisible() const
{
    return twoPageModeActive() && m_secondaryPageVisible;
}

std::shared_ptr<DisplayedImageSurface> ImageDocumentController::imageSurface(
    DisplayedPageRole role) const
{
    if (m_twoPageSpreadTransitionInProgress) {
        return nullptr;
    }

    if (role == DisplayedPageRole::Secondary) {
        return secondaryPageVisible() ? m_secondaryPresentationController->imageSurface() : nullptr;
    }

    return m_presentationController->imageSurface();
}

const QImage &ImageDocumentController::image() const { return m_presentationController->image(); }

quint64 ImageDocumentController::imageRevision(DisplayedPageRole role) const
{
    if (m_twoPageSpreadTransitionInProgress) {
        return 0;
    }

    if (role == DisplayedPageRole::Secondary) {
        return secondaryPageVisible() ? m_secondaryPresentationController->imageRevision() : 0;
    }

    return m_presentationController->imageRevision();
}

void ImageDocumentController::openPreviousImage()
{
    if (!twoPageModeActive() || currentPageNumber() <= 0) {
        m_navigationController->openPreviousImage();
        return;
    }

    if (currentPageNumber() <= 2) {
        openImageAtPage(1);
        return;
    }

    int offset = secondaryPageVisible() ? -2 : -1;
    if (secondaryPageVisible()) {
        const std::optional<QUrl> previousUrl
            = m_navigationController->urlAtPage(currentPageNumber() - 1);
        if (previousUrl.has_value() && cachedPageIsWide(*previousUrl).value_or(false)) {
            offset = -1;
        }
    }
    openImageAtRelativePageOffset(offset);
}

void ImageDocumentController::openNextImage()
{
    if (!twoPageModeActive() || currentPageNumber() <= 0) {
        m_navigationController->openNextImage();
        return;
    }

    const int nextPage = currentLastPageNumber() + 1;
    if (nextPage > imageCount()) {
        return;
    }

    openImageAtPage(nextPage);
}

void ImageDocumentController::openPreviousSinglePage() { openImageAtRelativePageOffset(-1); }

void ImageDocumentController::openNextSinglePage() { openImageAtRelativePageOffset(1); }

void ImageDocumentController::openPreviousContainer()
{
    m_navigationController->openPreviousContainer();
}

void ImageDocumentController::openNextContainer() { m_navigationController->openNextContainer(); }

void ImageDocumentController::deleteDisplayedFile(FileDeletionMode mode)
{
    if (!m_presentationController->hasImage()) {
        return;
    }

    m_deletionController->deleteDisplayedFile(m_state.displayedImageLocation(), mode);
}

void ImageDocumentController::openImageAtPage(int pageNumber)
{
    const std::optional<QUrl> pageUrl = m_navigationController->urlAtPage(pageNumber);
    if (!pageUrl.has_value()) {
        return;
    }

    const bool spreadTransition = shouldBeginTwoPageSpreadTransition(pageNumber);
    if (spreadTransition) {
        beginTwoPageSpreadTransition();
    }
    setSourceUrlForLoad(*pageUrl, QUrl(), spreadTransition);
}

void ImageDocumentController::resetZoom()
{
    if (secondaryPageVisible()) {
        m_spreadZoomState.resetZoom(spreadDevicePixelRatio());
        m_presentationController->setZoomPercent(m_spreadZoomState.zoomPercent());
        m_secondaryPresentationController->setZoomPercent(m_spreadZoomState.zoomPercent());
        applySpreadVisibleItemRects();
        notify(ImageDocumentChange::ZoomMode);
        notify(ImageDocumentChange::ZoomPercent);
        notify(ImageDocumentChange::DisplaySize);
        notify(ImageDocumentChange::MaximumManualZoomPercent);
        notify(ImageDocumentChange::Repaint);
        notify(ImageDocumentChange::TwoPageMode);
        return;
    }

    m_presentationController->resetZoom();
}

void ImageDocumentController::setFitMode(ImageZoomMode zoomMode)
{
    if (secondaryPageVisible()) {
        if (zoomMode == ImageZoomMode::Manual) {
            return;
        }

        m_spreadZoomState.setFitMode(zoomMode, spreadDevicePixelRatio());
        m_presentationController->setZoomPercent(m_spreadZoomState.zoomPercent());
        m_secondaryPresentationController->setZoomPercent(m_spreadZoomState.zoomPercent());
        applySpreadVisibleItemRects();
        notify(ImageDocumentChange::ZoomMode);
        notify(ImageDocumentChange::ZoomPercent);
        notify(ImageDocumentChange::DisplaySize);
        notify(ImageDocumentChange::MaximumManualZoomPercent);
        notify(ImageDocumentChange::Repaint);
        notify(ImageDocumentChange::TwoPageMode);
        return;
    }

    m_presentationController->setFitMode(zoomMode);
}

void ImageDocumentController::updateRenderContext()
{
    m_presentationController->updateRenderContext();
    m_secondaryPresentationController->updateRenderContext();
    if (secondaryPageVisible()) {
        m_spreadZoomState.update(spreadDevicePixelRatio());
        applySpreadVisibleItemRects();
        notify(ImageDocumentChange::DisplaySize);
        notify(ImageDocumentChange::MaximumManualZoomPercent);
        notify(ImageDocumentChange::Repaint);
        notify(ImageDocumentChange::TwoPageMode);
    }
}

void ImageDocumentController::dispatchEffect(ImageDocumentEffect effect)
{
    std::visit([this](const auto &payload) { dispatchEffectPayload(payload); }, effect.payload);
}

void ImageDocumentController::dispatchEffectPayload(const ClearImageEffect &) { clearImage(); }

void ImageDocumentController::dispatchEffectPayload(const ResetZoomEffect &) { resetZoom(); }

void ImageDocumentController::dispatchEffectPayload(const UpdatePageNavigationEffect &)
{
    m_navigationController->updatePageNavigation();
}

void ImageDocumentController::dispatchEffectPayload(const ScheduleAdjacentImagePredecodeEffect &)
{
    scheduleAdjacentImagePredecode();
}

void ImageDocumentController::dispatchEffectPayload(const OpenUrlEffect &payload)
{
    setSourceUrl(payload.url);
}

void ImageDocumentController::dispatchEffectPayload(const ContainerImageSelectedEffect &payload)
{
    setSourceUrlForLoad(payload.imageUrl, payload.containerUrl);
}

void ImageDocumentController::dispatchEffectPayload(const EmptyContainerSelectedEffect &payload)
{
    m_openController->finishContainerNavigationWithEmptyContainer(payload.containerUrl);
}

void ImageDocumentController::dispatchEffectPayload(const ContainerNavigationFailedEffect &payload)
{
    m_openController->finishContainerNavigationLoadWithError(
        payload.containerUrl, payload.errorString);
}

void ImageDocumentController::dispatchEffectPayload(const PrepareFailedContainerEffect &payload)
{
    m_presentationController->prepareFailedContainer(payload.containerUrl);
}

void ImageDocumentController::dispatchEffectPayload(const AnimationFailedEffect &payload)
{
    finishWithAnimationError(payload.errorString);
}

void ImageDocumentController::dispatchEffects(ImageDocumentEffects effects)
{
    for (ImageDocumentEffect &effect : effects) {
        dispatchEffect(std::move(effect));
    }
}

void ImageDocumentController::setSourceUrlForLoad(
    const QUrl &sourceUrl, const QUrl &containerNavigationUrl, bool preserveTwoPageSpreadTransition)
{
    m_deletionController->cancel();

    if (m_state.sourceUrl() == sourceUrl) {
        if (!preserveTwoPageSpreadTransition) {
            finishTwoPageSpreadTransition();
        }
        m_state.clearLoadingContainerNavigationUrl();
        if (!containerNavigationUrl.isEmpty()) {
            m_state.setContainerNavigationUrl(containerNavigationUrl);
        }
        return;
    }

    cancelNavigationAndPredecode();
    if (!preserveTwoPageSpreadTransition) {
        finishTwoPageSpreadTransition();
    }
    clearSecondaryPage();
    m_state.setLoadingContainerNavigationUrl(containerNavigationUrl);
    m_state.setSourceUrl(sourceUrl);
    m_openController->open();
}

void ImageDocumentController::clearAfterSuccessfulFileDeletion()
{
    cancelNavigationAndPredecode();
    m_openController->cancel();
    finishTwoPageSpreadTransition();
    clearSecondaryPage();
    m_state.setSourceUrl(QUrl());
    m_state.setErrorString(QString());
    dispatchEffects(ImageOpenWorkflow::finishEmptySourceLoad(m_state));
}

void ImageDocumentController::scheduleAdjacentImagePredecode()
{
    if (!m_presentationController->hasImage() || m_state.displayedUrl().isEmpty()) {
        cancelPredecode();
        return;
    }

    std::optional<StaticImagePayload> staticImage = m_presentationController->staticImage();
    if (!staticImage.has_value()) {
        cancelPredecode();
        return;
    }

    m_predecodeCoordinator->schedule(ImagePredecodeCoordinator::Context {
        m_state.displayedImageLocation(), m_presentationController->isPredecodeCacheable(),
        std::move(*staticImage), m_presentationController->firstDisplayDecodeContext() });
}

void ImageDocumentController::cancelNavigationAndPredecode()
{
    m_navigationController->cancelNavigation();
    m_navigationController->cancelContainerNavigation();
    cancelPredecode();
}

void ImageDocumentController::cancelPredecode()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->cancel();
    }
}

std::optional<PredecodedImage> ImageDocumentController::takePredecodedImage(const QUrl &url) const
{
    if (m_predecodeCoordinator == nullptr) {
        return std::nullopt;
    }

    return m_predecodeCoordinator->tryTake(url);
}

void ImageDocumentController::finishWithAnimationError(const QString &errorString)
{
    const QString message = errorString.isEmpty()
        ? imageViewText("Could not decode the selected image animation.")
        : errorString;
    ImageDocumentEffects effects
        = ImageOpenWorkflow::finishAnimationLoadWithError(m_state, message);
    dispatchEffects(std::move(effects));
}

void ImageDocumentController::clearSecondaryPage()
{
    if (m_secondaryImageLoader != nullptr) {
        m_secondaryImageLoader->cancel();
    }
    if (m_secondaryPresentationController != nullptr) {
        m_secondaryPresentationController->stopAnimation();
        m_secondaryPresentationController->clearImage();
    }
    m_secondaryDisplayedImageLocation = DisplayedImageLocation {};
    m_secondaryPageVisible = false;
}

void ImageDocumentController::clearSecondaryPageAndNotify()
{
    const bool wasVisible = secondaryPageVisible();
    clearSecondaryPage();
    if (wasVisible) {
        notifyTwoPageModeChanged();
    }
}

void ImageDocumentController::refreshSecondaryPage()
{
    cacheWidePage(m_state.displayedUrl(), m_presentationController->imageSize());

    auto finishWithPrimaryPage = [this]() {
        applyStoredSpreadZoomToPrimaryPage();
        clearSecondaryPageAndNotify();
        finishTwoPageSpreadTransition();
    };

    if (!twoPageModeActive() || currentPageIsCover() || primaryPageIsWide()) {
        finishWithPrimaryPage();
        return;
    }

    const int nextPageNumber = currentPageNumber() + 1;
    if (nextPageNumber <= 1 || nextPageNumber > imageCount()) {
        finishWithPrimaryPage();
        return;
    }

    const std::optional<QUrl> nextUrl = m_navigationController->urlAtPage(nextPageNumber);
    if (!nextUrl.has_value()) {
        finishWithPrimaryPage();
        return;
    }

    if (cachedPageIsWide(*nextUrl).value_or(false)) {
        finishWithPrimaryPage();
        return;
    }

    if (secondaryPageVisible() && m_secondaryDisplayedImageLocation.imageUrl() == *nextUrl) {
        return;
    }

    startSecondaryPageLoad(*nextUrl);
}

void ImageDocumentController::startSecondaryPageLoad(const QUrl &url)
{
    clearSecondaryPage();
    m_secondaryImageLoader->start(
        ImageLoadRequest::fromLocation(url, m_state.displayedArchiveDocument()),
        m_presentationController->firstDisplayDecodeContext());
    notifyTwoPageModeChanged();
}

void ImageDocumentController::finishSecondaryPredecodedImageLoad(
    ImageLoadSession session, PredecodedImage image)
{
    finishSecondaryStaticImageLoad(session, std::move(image.staticImage), true);
}

void ImageDocumentController::finishSecondaryDecodedImageLoad(
    ImageLoadSession session, DecodedImage image)
{
    auto handleDecoded = [this, &session](auto &decoded) {
        return finishSecondaryDecodedImageResult(session, decoded);
    };
    std::visit(handleDecoded, image);
}

bool ImageDocumentController::finishSecondaryDecodedImageResult(
    const ImageLoadSession &session, StaticDecodedImage &decoded)
{
    const bool predecodeCacheable = PredecodeCache::canCacheImage(decoded.staticImage);
    finishSecondaryStaticImageLoad(session, std::move(decoded.staticImage), predecodeCacheable);
    return true;
}

bool ImageDocumentController::finishSecondaryDecodedImageResult(
    const ImageLoadSession &session, DecodedAnimationImage &decoded)
{
    if (decoded.frames.empty()) {
        finishSecondaryLoadWithError(session);
        return false;
    }

    finishSecondaryImageLoad(session, decoded.frames.front().image, false);
    return true;
}

bool ImageDocumentController::finishSecondaryDecodedImageResult(
    const ImageLoadSession &session, ReaderAnimationImage &decoded)
{
    finishSecondaryImageLoad(session, decoded.firstFrame, false);
    return true;
}

bool ImageDocumentController::finishSecondaryDecodedImageResult(
    const ImageLoadSession &session, HeifSequenceAnimationImage &decoded)
{
    finishSecondaryImageLoad(session, decoded.firstFrame, false);
    return true;
}

void ImageDocumentController::finishSecondaryImageLoad(
    const ImageLoadSession &session, const QImage &image, bool predecodeCacheable)
{
    m_secondaryPresentationController->prepareImageContainer(
        zoomScopeUrlForLocation(session.location));
    m_secondaryPresentationController->setPredecodeCacheable(predecodeCacheable);
    m_secondaryPresentationController->setImage(image);
    m_secondaryDisplayedImageLocation = session.location;
    cacheWidePage(session.location.imageUrl(), m_secondaryPresentationController->imageSize());
    m_secondaryPageVisible = !pageIsWide(m_secondaryPresentationController->imageSize());
    if (!m_secondaryPageVisible) {
        clearSecondaryPage();
        applyStoredSpreadZoomToPrimaryPage();
        finishTwoPageSpreadTransition();
        notifyTwoPageModeChanged();
        return;
    }

    updateSpreadZoomState();
    finishTwoPageSpreadTransition();
    notifyTwoPageModeChanged();
}

void ImageDocumentController::finishSecondaryStaticImageLoad(
    const ImageLoadSession &session, StaticImagePayload staticImage, bool predecodeCacheable)
{
    m_secondaryPresentationController->prepareImageContainer(
        zoomScopeUrlForLocation(session.location));
    m_secondaryPresentationController->setPredecodeCacheable(predecodeCacheable);
    m_secondaryPresentationController->setStaticImage(std::move(staticImage));
    m_secondaryDisplayedImageLocation = session.location;
    cacheWidePage(session.location.imageUrl(), m_secondaryPresentationController->imageSize());
    m_secondaryPageVisible = !pageIsWide(m_secondaryPresentationController->imageSize());
    if (!m_secondaryPageVisible) {
        clearSecondaryPage();
        applyStoredSpreadZoomToPrimaryPage();
        finishTwoPageSpreadTransition();
        notifyTwoPageModeChanged();
        return;
    }

    updateSpreadZoomState();
    finishTwoPageSpreadTransition();
    notifyTwoPageModeChanged();
}

void ImageDocumentController::finishSecondaryLoadWithError(const ImageLoadSession &)
{
    clearSecondaryPage();
    applyStoredSpreadZoomToPrimaryPage();
    finishTwoPageSpreadTransition();
    notifyTwoPageModeChanged();
}

bool ImageDocumentController::shouldBeginTwoPageSpreadTransition(int targetPageNumber) const
{
    return twoPageModeActive() && currentPageNumber() > 0 && targetPageNumber > 0
        && targetPageNumber <= imageCount() && targetPageNumber != currentPageNumber();
}

void ImageDocumentController::beginTwoPageSpreadTransition()
{
    if (m_twoPageSpreadTransitionInProgress) {
        notifyTwoPageSpreadTransitionChanged();
        return;
    }

    m_twoPageSpreadTransitionInProgress = true;
    notifyTwoPageSpreadTransitionChanged();
}

void ImageDocumentController::finishTwoPageSpreadTransition()
{
    if (!m_twoPageSpreadTransitionInProgress) {
        return;
    }

    m_twoPageSpreadTransitionInProgress = false;
    notifyTwoPageSpreadTransitionChanged();
}

void ImageDocumentController::notifyTwoPageSpreadTransitionChanged()
{
    notify(ImageDocumentChange::Status);
    notify(ImageDocumentChange::Loading);
    notify(ImageDocumentChange::Repaint);
}

void ImageDocumentController::updateSpreadZoomState()
{
    if (!secondaryPageVisible()) {
        return;
    }

    const QSize nextSpreadImageSize = spreadImageSize();
    const qreal devicePixelRatio = spreadDevicePixelRatio();
    const ImageZoomMode activeZoomMode = m_spreadZoomState.imageSize().isEmpty()
        ? m_presentationController->zoomMode()
        : m_spreadZoomState.zoomMode();
    const qreal activeZoomPercent = m_spreadZoomState.imageSize().isEmpty()
        ? m_presentationController->zoomPercent()
        : m_spreadZoomState.zoomPercent();

    m_spreadZoomState.setViewportSize(m_presentationController->viewportSize(), devicePixelRatio);
    m_spreadZoomState.setImageSize(nextSpreadImageSize, devicePixelRatio);
    if (activeZoomMode == ImageZoomMode::Manual) {
        m_spreadZoomState.setManualZoomPercent(activeZoomPercent, devicePixelRatio);
    } else {
        m_spreadZoomState.setFitMode(activeZoomMode, devicePixelRatio);
    }

    m_presentationController->setZoomPercent(m_spreadZoomState.zoomPercent());
    m_secondaryPresentationController->setZoomPercent(m_spreadZoomState.zoomPercent());
    applySpreadVisibleItemRects();
}

void ImageDocumentController::applyStoredSpreadZoomToPrimaryPage()
{
    if (m_spreadZoomState.imageSize().isEmpty()) {
        return;
    }

    if (m_spreadZoomState.zoomMode() == ImageZoomMode::Manual) {
        m_presentationController->setZoomPercent(m_spreadZoomState.zoomPercent());
    } else if (m_spreadZoomState.zoomMode() == ImageZoomMode::Fit) {
        m_presentationController->resetZoom();
    } else {
        m_presentationController->setFitMode(m_spreadZoomState.zoomMode());
    }
}

void ImageDocumentController::applySpreadVisibleItemRects()
{
    if (!secondaryPageVisible()) {
        return;
    }

    const QRectF visibleRect = m_visibleItemRect;
    const QRectF primaryRect = primarySpreadPageRect();
    const QRectF secondaryRect = secondarySpreadPageRect();
    m_presentationController->setVisibleItemRect(
        visibleRect.intersected(primaryRect).translated(-primaryRect.x(), -primaryRect.y()));
    m_secondaryPresentationController->setVisibleItemRect(
        visibleRect.intersected(secondaryRect).translated(-secondaryRect.x(), -secondaryRect.y()));
}

QRectF ImageDocumentController::primarySpreadPageRect() const
{
    const QSizeF pageSize = primaryDisplaySize();
    const QSizeF spreadSize = displaySize();
    return QRectF(0.0, std::max<qreal>(0.0, (spreadSize.height() - pageSize.height()) / 2.0),
        pageSize.width(), pageSize.height());
}

QRectF ImageDocumentController::secondarySpreadPageRect() const
{
    const QSizeF primarySize = primaryDisplaySize();
    const QSizeF secondarySize = secondaryDisplaySize();
    const QSizeF spreadSize = displaySize();
    return QRectF(primarySize.width(),
        std::max<qreal>(0.0, (spreadSize.height() - secondarySize.height()) / 2.0),
        secondarySize.width(), secondarySize.height());
}

QSize ImageDocumentController::spreadImageSize() const
{
    const QSize primarySize = m_presentationController->imageSize();
    const QSize secondarySize = m_secondaryPresentationController->imageSize();
    if (primarySize.isEmpty() || secondarySize.isEmpty()) {
        return primarySize;
    }

    return QSize(primarySize.width() + secondarySize.width(),
        std::max(primarySize.height(), secondarySize.height()));
}

bool ImageDocumentController::twoPageModeActive() const
{
    return m_twoPageModeEnabled && twoPageModeAvailable();
}

bool ImageDocumentController::currentPageIsCover() const { return currentPageNumber() == 1; }

bool ImageDocumentController::primaryPageIsWide() const
{
    return pageIsWide(m_presentationController->imageSize());
}

bool ImageDocumentController::pageIsWide(const QSize &imageSize)
{
    return !imageSize.isEmpty() && imageSize.width() > imageSize.height();
}

void ImageDocumentController::cacheWidePage(const QUrl &url, const QSize &imageSize)
{
    const QString key = pageCacheKey(url);
    if (key.isEmpty() || imageSize.isEmpty()) {
        return;
    }

    m_widePageByUrl[key] = pageIsWide(imageSize);
}

std::optional<bool> ImageDocumentController::cachedPageIsWide(const QUrl &url) const
{
    const QString key = pageCacheKey(url);
    if (key.isEmpty()) {
        return std::nullopt;
    }

    const auto cached = m_widePageByUrl.find(key);
    if (cached == m_widePageByUrl.cend()) {
        return std::nullopt;
    }

    return cached->second;
}

QString ImageDocumentController::pageCacheKey(const QUrl &url)
{
    return url.adjusted(QUrl::NormalizePathSegments).toString();
}

void ImageDocumentController::openImageAtRelativePageOffset(int offset)
{
    const int targetPage = currentPageNumber() + offset;
    if (targetPage < 1 || targetPage > imageCount()) {
        return;
    }

    openImageAtPage(targetPage);
}

void ImageDocumentController::notifyTwoPageModeChanged()
{
    notify(ImageDocumentChange::TwoPageMode);
    notify(ImageDocumentChange::ImageSize);
    notify(ImageDocumentChange::DisplaySize);
    notify(ImageDocumentChange::ZoomPercent);
    notify(ImageDocumentChange::ZoomMode);
    notify(ImageDocumentChange::MaximumManualZoomPercent);
    notify(ImageDocumentChange::Repaint);
}

ImageDocumentRenderContext ImageDocumentController::renderContext() const
{
    ImageDocumentRenderContext context
        = m_renderContextProvider ? m_renderContextProvider() : ImageDocumentRenderContext {};
    if (!std::isfinite(context.devicePixelRatio) || context.devicePixelRatio <= 0.0) {
        context.devicePixelRatio = 1.0;
    }
    if (context.maximumTextureSize <= 0) {
        context.maximumTextureSize = fallbackTextureSizeMax;
    }
    return context;
}

qreal ImageDocumentController::spreadDevicePixelRatio() const
{
    return renderContext().devicePixelRatio;
}

void ImageDocumentController::notify(ImageDocumentChange change)
{
    if (change == ImageDocumentChange::ErrorString && !m_state.errorString().isEmpty()) {
        finishTwoPageSpreadTransition();
    }

    if (change == ImageDocumentChange::PageNavigation) {
        refreshSecondaryPage();
    }

    invokeIfSet(m_changeCallback, change);
}

void ImageDocumentController::clearImage()
{
    if (m_predecodeCoordinator != nullptr) {
        m_predecodeCoordinator->clear();
    }
    finishTwoPageSpreadTransition();
    clearSecondaryPage();
    m_navigationController->cancelPageNavigationUpdate();
    m_state.clearDisplayedImageUrls();
    m_presentationController->clearImage();
    m_navigationController->clearPageNavigation();
}
}
