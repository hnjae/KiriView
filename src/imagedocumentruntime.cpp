// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntime.h"

#include "archivedocumentsessionstore.h"
#include "imagecallback.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumenteffectexecutor.h"
#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentsourceloadrequest.h"
#include "imageopencontroller.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <QObject>
#include <QUrl>
#include <optional>
#include <utility>

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
    navigationController = std::make_unique<ImageDocumentNavigationController>(documentObject,
        state, *presentationController,
        ImageDocumentNavigationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            [this]() { return documentDeletionController->inProgress(); },
        },
        dependencies.candidateProvider);
    predecodeController = std::make_unique<ImageDocumentPredecodeController>(
        documentObject, state, *presentationController, dependencies.candidateProvider,
        dependencies.imageDecode, [this]() { return navigationController->currentPageNumber(); },
        std::move(dependencies.powerSaver));
    spreadController = std::make_unique<ImageSpreadPresentationController>(documentObject,
        std::move(spreadRenderContextProvider), state, *presentationController,
        ImageSpreadPresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QUrl &url) { return predecodeController->tryTake(url); },
            [this]() { return navigationController->currentPageNumber(); },
            [this]() { return navigationController->imageCount(); },
            [this](int pageNumber) { return navigationController->urlAtPage(pageNumber); },
            [this]() { dispatchEffect(ImageDocumentEffect::scheduleAdjacentImagePredecode()); },
        },
        dependencies.candidateProvider, dependencies.imageDecode);
    loadController = std::make_unique<ImageDocumentLoadController>(state,
        *documentDeletionController, *navigationController, *predecodeController, *openController,
        *spreadController, archiveSessionStore.get());
    effectExecutor = std::make_unique<ImageDocumentEffectExecutor>(state, *navigationController,
        *predecodeController, *openController, *presentationController, *spreadController,
        *loadController, archiveSessionStore.get());
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
    return navigationController->currentPageNumber();
}

int ImageDocumentRuntime::currentLastPageNumber() const
{
    return spreadController->currentLastPageNumber();
}

int ImageDocumentRuntime::imageCount() const { return navigationController->imageCount(); }

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

void ImageDocumentRuntime::shutdown()
{
    documentDeletionController->cancel();
    presentationController->stopAnimation();
    spreadController->shutdown();
    predecodeController->cancel();
    navigationController->cancelPageNavigationUpdate();
    navigationController->cancelContainerNavigation();
    navigationController->cancelNavigation();
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
    const std::optional<QUrl> pageUrl = navigationController->selectPage(pageNumber);
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
        if (direction == NavigationDirection::Previous) {
            navigationController->openPreviousImage();
            return;
        }

        navigationController->openNextImage();
        return;
    }

    if (target.pageNumber <= 0) {
        return;
    }

    openImageAtPage(target.pageNumber);
}

void ImageDocumentRuntime::openAdjacentContainer(NavigationDirection direction)
{
    if (direction == NavigationDirection::Previous) {
        navigationController->openPreviousContainer();
        return;
    }

    navigationController->openNextContainer();
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
