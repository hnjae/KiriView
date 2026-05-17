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
