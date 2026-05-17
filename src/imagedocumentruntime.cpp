// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntime.h"

#include "archivedocumentsessionstore.h"
#include "imagecallback.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumenteffectexecutor.h"
#include "imagedocumentloadcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagenavigationservice.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"
#include "imageviewtext.h"

#include <QObject>
#include <QUrl>
#include <optional>
#include <utility>

namespace {
KiriView::ImageNavigationService::DisplayContext navigationDisplayContext(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImagePresentationController &presentationController)
{
    return KiriView::ImageNavigationService::DisplayContext { presentationController.hasImage(),
        state.displayedImageLocation() };
}
}

namespace KiriView {
ImageDocumentRuntime::ImageDocumentRuntime(QObject *documentObject,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    ImageAsyncDependencies dependencies, FileDeletionFailedCallback fileDeletionFailedCallback)
    : state([this](ImageDocumentChange change) { notify(change); })
    , changeCallback(std::move(changeCallback))
{
    const bool shouldUseArchiveSessionStore = dependencies.archiveDocumentSessions
        || (!dependencies.candidateProvider.archiveImages && !dependencies.imageDecode.dataLoader);
    ArchiveDocumentSessionFactory archiveDocumentSessions
        = std::move(dependencies.archiveDocumentSessions);
    dependencies.archiveDocumentSessions = {};
    dependencies = imageAsyncDependenciesWithDefaults(std::move(dependencies));
    if (shouldUseArchiveSessionStore) {
        archiveSessionStore = std::make_unique<ArchiveDocumentSessionStore>(
            std::move(archiveDocumentSessions), documentObject);
        dependencies = archiveSessionStore->wrapDependencies(std::move(dependencies));
    }
    RenderContextProvider primaryRenderContextProvider = renderContextProvider;
    RenderContextProvider spreadRenderContextProvider = std::move(renderContextProvider);
    presentationController = std::make_unique<ImagePresentationController>(documentObject,
        std::move(primaryRenderContextProvider),
        ImagePresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QString &errorString) {
                openController->finishAnimationLoadWithError(errorString);
            },
        });
    documentDeletionController = std::make_unique<ImageDocumentDeletionController>(documentObject,
        state, *presentationController, dependencies.candidateProvider,
        std::move(dependencies.fileOperations),
        ImageDocumentDeletionController::Callbacks {
            [this]() { notify(ImageDocumentChange::FileDeletionInProgress); },
            [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            std::move(fileDeletionFailedCallback),
        });
    openController
        = std::make_unique<ImageOpenController>(documentObject, state, *presentationController,
            ImageOpenController::Callbacks {
                [this](const QUrl &url) { return predecodeController->tryTake(url); },
                [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            },
            dependencies.candidateProvider, dependencies.imageDecode);
    navigationService = std::make_unique<ImageNavigationService>(documentObject,
        dependencies.candidateProvider,
        ImageNavigationService::Callbacks {
            [this](const QUrl &url) { dispatchEffect(ImageDocumentEffect::openUrl(url)); },
            [this](const QUrl &imageUrl, const QUrl &containerUrl) {
                dispatchEffect(ImageDocumentEffect::containerImageSelected(imageUrl, containerUrl));
            },
            [this](const QUrl &url, ContainerNavigationError error, const QString &message) {
                if (error == ContainerNavigationError::EmptyContainer) {
                    dispatchEffect(ImageDocumentEffect::emptyContainerSelected(url));
                    return;
                }

                if (error == ContainerNavigationError::InvalidComicBookArchive) {
                    dispatchEffect(ImageDocumentEffect::containerNavigationFailed(
                        url, imageViewText("Could not open the selected comic book archive.")));
                    return;
                }

                dispatchEffect(ImageDocumentEffect::containerNavigationFailed(url, message));
            },
            [this]() { notify(ImageDocumentChange::PageNavigation); },
            [this]() { dispatchEffect(ImageDocumentEffect::clearDeletedImage()); },
            [this]() { return documentDeletionController->inProgress(); },
        });
    predecodeController = std::make_unique<ImageDocumentPredecodeController>(
        documentObject, state, *presentationController, dependencies.candidateProvider,
        dependencies.imageDecode, [this]() { return navigationService->currentPageNumber(); },
        std::move(dependencies.powerSaver));
    spreadController = std::make_unique<ImageSpreadPresentationController>(documentObject,
        std::move(spreadRenderContextProvider), state, *presentationController,
        ImageSpreadPresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QUrl &url) { return predecodeController->tryTake(url); },
            [this]() { return navigationService->currentPageNumber(); },
            [this]() { return navigationService->imageCount(); },
            [this](int pageNumber) { return navigationService->urlAtPage(pageNumber); },
            [this]() { dispatchEffect(ImageDocumentEffect::scheduleAdjacentImagePredecode()); },
        },
        dependencies.candidateProvider, dependencies.imageDecode);
    loadController = std::make_unique<ImageDocumentLoadController>(state,
        *documentDeletionController, *navigationService, *predecodeController, *openController,
        *spreadController, archiveSessionStore.get());
    effectExecutor = std::make_unique<ImageDocumentEffectExecutor>(effectOperations());
}

ImageDocumentRuntime::~ImageDocumentRuntime() { shutdown(); }

QUrl ImageDocumentRuntime::sourceUrl() const { return state.sourceUrl(); }

void ImageDocumentRuntime::setSourceUrl(const QUrl &sourceUrl)
{
    loadController->loadSource(ImageDocumentSourceLoadRequest::fromUrl(sourceUrl));
}

ImageDocumentStatus ImageDocumentRuntime::status() const
{
    return spreadController->status(state.status());
}

bool ImageDocumentRuntime::loading() const { return spreadController->loading(state.loading()); }

QString ImageDocumentRuntime::errorString() const { return state.errorString(); }

QString ImageDocumentRuntime::windowTitleFileName() const { return state.windowTitleFileName(); }

QUrl ImageDocumentRuntime::displayedUrl() const { return state.displayedUrl(); }

QSize ImageDocumentRuntime::imageSize() const { return spreadController->imageSize(); }

QSize ImageDocumentRuntime::primaryImageSize() const { return presentationController->imageSize(); }

QSize ImageDocumentRuntime::secondaryImageSize() const
{
    return spreadController->secondaryImageSize();
}

QSizeF ImageDocumentRuntime::viewportSize() const { return presentationController->viewportSize(); }

void ImageDocumentRuntime::setViewportSize(const QSizeF &viewportSize)
{
    spreadController->setViewportSize(viewportSize);
}

QRectF ImageDocumentRuntime::visibleItemRect() const { return spreadController->visibleItemRect(); }

void ImageDocumentRuntime::setVisibleItemRect(const QRectF &visibleItemRect)
{
    spreadController->setVisibleItemRect(visibleItemRect);
}

QSizeF ImageDocumentRuntime::displaySize() const { return spreadController->displaySize(); }

QSizeF ImageDocumentRuntime::primaryDisplaySize() const
{
    return spreadController->primaryDisplaySize();
}

QSizeF ImageDocumentRuntime::secondaryDisplaySize() const
{
    return spreadController->secondaryDisplaySize();
}

qreal ImageDocumentRuntime::zoomPercent() const { return spreadController->zoomPercent(); }

void ImageDocumentRuntime::setZoomPercent(qreal zoomPercent)
{
    spreadController->setZoomPercent(zoomPercent);
}

ImageZoomMode ImageDocumentRuntime::zoomMode() const { return spreadController->zoomMode(); }

qreal ImageDocumentRuntime::maximumManualZoomPercent() const
{
    return spreadController->maximumManualZoomPercent();
}

qreal ImageDocumentRuntime::clampedManualZoomPercent(qreal zoomPercent) const
{
    return spreadController->clampedManualZoomPercent(zoomPercent);
}

qreal ImageDocumentRuntime::steppedManualZoomPercent(qreal stepCount) const
{
    return spreadController->steppedManualZoomPercent(stepCount);
}

int ImageDocumentRuntime::rotationDegrees() const { return spreadController->rotationDegrees(); }

int ImageDocumentRuntime::currentPageNumber() const
{
    return navigationService->currentPageNumber();
}

int ImageDocumentRuntime::currentLastPageNumber() const
{
    return spreadController->currentLastPageNumber();
}

int ImageDocumentRuntime::imageCount() const { return navigationService->imageCount(); }

bool ImageDocumentRuntime::containerNavigationAvailable() const
{
    return state.containerNavigationAvailable();
}

bool ImageDocumentRuntime::fileDeletionInProgress() const
{
    return documentDeletionController->inProgress();
}

bool ImageDocumentRuntime::twoPageModeEnabled() const
{
    return spreadController->twoPageModeEnabled();
}

void ImageDocumentRuntime::setTwoPageModeEnabled(bool enabled)
{
    spreadController->setTwoPageModeEnabled(enabled);
}

bool ImageDocumentRuntime::twoPageModeAvailable() const
{
    return spreadController->twoPageModeAvailable();
}

bool ImageDocumentRuntime::rightToLeftReadingEnabled() const
{
    return spreadController->rightToLeftReadingEnabled();
}

void ImageDocumentRuntime::setRightToLeftReadingEnabled(bool enabled)
{
    spreadController->setRightToLeftReadingEnabled(enabled);
}

bool ImageDocumentRuntime::rightToLeftReadingAvailable() const
{
    return spreadController->rightToLeftReadingAvailable();
}

bool ImageDocumentRuntime::secondaryPageVisible() const
{
    return spreadController->secondaryPageVisible();
}

std::shared_ptr<DisplayedImageSurface> ImageDocumentRuntime::imageSurface(
    DisplayedPageRole role) const
{
    return spreadController->imageSurface(role);
}

const QImage &ImageDocumentRuntime::image() const { return presentationController->image(); }

quint64 ImageDocumentRuntime::imageRevision(DisplayedPageRole role) const
{
    return spreadController->imageRevision(role);
}

void ImageDocumentRuntime::dispatchEffect(ImageDocumentEffect effect)
{
    effectExecutor->dispatch(std::move(effect));
}

void ImageDocumentRuntime::notify(ImageDocumentChange change)
{
    spreadController->handleDocumentChange(change);
    invokeIfSet(changeCallback, change);
}

ImageDocumentEffectOperations ImageDocumentRuntime::effectOperations()
{
    ImageDocumentEffectOperations operations;
    operations.clearArchiveSession = [this]() {
        if (archiveSessionStore != nullptr) {
            archiveSessionStore->clear();
        }
    };
    operations.clearPredecode = [this]() { predecodeController->clear(); };
    operations.cancelPredecode = [this]() { predecodeController->cancel(); };
    operations.finishSpreadTransition = [this]() { spreadController->finishTransition(); };
    operations.clearSecondaryPage = [this]() { spreadController->clearSecondaryPage(); };
    operations.cancelPageNavigationUpdate
        = [this]() { navigationService->cancelPageNavigationUpdate(); };
    operations.cancelNavigation = [this]() { navigationService->cancelNavigation(); };
    operations.cancelContainerNavigation
        = [this]() { navigationService->cancelContainerNavigation(); };
    operations.cancelOpen = [this]() { openController->cancel(); };
    operations.clearDisplayedImageLocation = [this]() { state.clearDisplayedImageLocation(); };
    operations.clearPresentationImage = [this]() { presentationController->clearImage(); };
    operations.clearPageNavigation = [this]() { navigationService->clearPageNavigation(); };
    operations.notifyRightToLeftReadingChanged
        = [this]() { spreadController->notifyRightToLeftReadingChanged(); };
    operations.resetZoom = [this]() { spreadController->resetZoom(); };
    operations.updatePageNavigation = [this]() {
        navigationService->updatePageNavigation(
            navigationDisplayContext(state, *presentationController));
    };
    operations.scheduleAdjacentImagePredecode = [this]() {
        predecodeController->scheduleAdjacentImagePredecode(
            spreadController->secondaryDisplayedPredecodeImage());
    };
    operations.loadUrl = [this](const QUrl &url) {
        loadController->loadSource(ImageDocumentSourceLoadRequest::fromUrl(url));
    };
    operations.loadContainerImage = [this](const QUrl &imageUrl, const QUrl &containerUrl) {
        loadController->loadSource(
            ImageDocumentSourceLoadRequest::fromContainerImage(imageUrl, containerUrl));
    };
    operations.finishEmptyContainerNavigation = [this](const QUrl &containerUrl) {
        openController->finishContainerNavigationWithEmptyContainer(containerUrl);
    };
    operations.finishContainerNavigationLoadWithError
        = [this](const QUrl &containerUrl, const QString &errorString) {
              openController->finishContainerNavigationLoadWithError(containerUrl, errorString);
          };
    operations.loadPageNavigationUrl
        = [this](const QUrl &url, bool preserveTwoPageSpreadTransition) {
              loadController->loadSource(ImageDocumentSourceLoadRequest::fromPageNavigation(
                  url, preserveTwoPageSpreadTransition));
          };
    operations.prepareFailedContainer = [this](const QUrl &containerUrl) {
        presentationController->prepareFailedContainer(containerUrl);
    };
    operations.setSourceUrl = [this](const QUrl &url) { state.setSourceUrl(url); };
    operations.setErrorString
        = [this](const QString &errorString) { state.setErrorString(errorString); };
    operations.finishEmptySourceLoad
        = [this]() { return ImageOpenWorkflow::finishEmptySourceLoad(state); };
    return operations;
}

void ImageDocumentRuntime::shutdown()
{
    documentDeletionController->cancel();
    presentationController->stopAnimation();
    spreadController->shutdown();
    predecodeController->cancel();
    navigationService->cancelPageNavigationUpdate();
    navigationService->cancelContainerNavigation();
    navigationService->cancelNavigation();
    openController->cancel();
    if (archiveSessionStore != nullptr) {
        archiveSessionStore->clear();
    }
}

void ImageDocumentRuntime::openPreviousImage() { openAdjacentImage(NavigationDirection::Previous); }

void ImageDocumentRuntime::openNextImage() { openAdjacentImage(NavigationDirection::Next); }

void ImageDocumentRuntime::openPreviousSinglePage() { openImageAtRelativePageOffset(-1); }

void ImageDocumentRuntime::openNextSinglePage() { openImageAtRelativePageOffset(1); }

void ImageDocumentRuntime::openPreviousContainer()
{
    openAdjacentContainer(NavigationDirection::Previous);
}

void ImageDocumentRuntime::openNextContainer() { openAdjacentContainer(NavigationDirection::Next); }

void ImageDocumentRuntime::deleteDisplayedFile(FileDeletionMode mode)
{
    documentDeletionController->deleteDisplayedFile(mode);
}

void ImageDocumentRuntime::resetZoom() { spreadController->resetZoom(); }

void ImageDocumentRuntime::setFitMode(ImageZoomMode zoomMode)
{
    spreadController->setFitMode(zoomMode);
}

void ImageDocumentRuntime::rotateClockwise() { spreadController->rotateClockwise(); }

void ImageDocumentRuntime::rotateCounterclockwise() { spreadController->rotateCounterclockwise(); }

void ImageDocumentRuntime::updateRenderContext() { spreadController->updateRenderContext(); }

void ImageDocumentRuntime::openImageAtPage(int pageNumber)
{
    const bool spreadTransition = spreadController->shouldBeginTransition(pageNumber);
    const std::optional<QUrl> pageUrl = navigationService->selectPage(pageNumber);
    if (!pageUrl.has_value()) {
        return;
    }

    if (spreadTransition) {
        spreadController->beginTransition();
    }

    dispatchEffect(ImageDocumentEffect::pageNavigationSelected(*pageUrl, spreadTransition));
}

void ImageDocumentRuntime::openAdjacentImage(NavigationDirection direction)
{
    const ImageSpreadPageNavigationTarget target
        = spreadController->imageNavigationTarget(direction);
    if (!target.handledBySpread) {
        navigationService->openAdjacentImage(
            navigationDisplayContext(state, *presentationController), direction);
        return;
    }

    if (target.pageNumber <= 0) {
        return;
    }

    openImageAtPage(target.pageNumber);
}

void ImageDocumentRuntime::openAdjacentContainer(NavigationDirection direction)
{
    navigationService->openAdjacentContainer(state.containerNavigationUrl(), direction);
}

void ImageDocumentRuntime::openImageAtRelativePageOffset(int offset)
{
    const int targetPage = spreadController->relativePageNavigationTarget(offset);
    if (targetPage <= 0) {
        return;
    }

    openImageAtPage(targetPage);
}
}
