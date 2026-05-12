// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntime.h"

#include "imagecallback.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumenteffectexecutor.h"
#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentnavigator.h"
#include "imagedocumentpredecodecontroller.h"
#include "imageopencontroller.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <QObject>
#include <utility>

namespace KiriView {
ImageDocumentRuntime::ImageDocumentRuntime(QObject *documentObject,
    RenderContextProvider renderContextProvider, ChangeCallback changeCallback,
    ImageAsyncDependencies dependencies, FileDeletionFailedCallback fileDeletionFailedCallback)
    : state([this](ImageDocumentChange change) { notify(change); })
    , changeCallback(std::move(changeCallback))
{
    dependencies = imageAsyncDependenciesWithDefaults(std::move(dependencies));
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
            [this]() {
                effectExecutor->dispatchAll(loadController->clearAfterSuccessfulFileDeletion());
            },
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
        },
        dependencies.candidateProvider);
    predecodeController = std::make_unique<ImageDocumentPredecodeController>(documentObject, state,
        *presentationController, dependencies.candidateProvider, dependencies.imageDecode);
    spreadController = std::make_unique<ImageSpreadPresentationController>(documentObject,
        std::move(spreadRenderContextProvider), state, *presentationController,
        ImageSpreadPresentationController::Callbacks {
            [this](ImageDocumentChange change) { notify(change); },
            [this](const QUrl &url) { return predecodeController->tryTake(url); },
            [this]() { return navigationController->currentPageNumber(); },
            [this]() { return navigationController->imageCount(); },
            [this](int pageNumber) { return navigationController->urlAtPage(pageNumber); },
        },
        dependencies.candidateProvider, dependencies.imageDecode);
    loadController = std::make_unique<ImageDocumentLoadController>(state,
        *documentDeletionController, *navigationController, *predecodeController, *openController,
        *presentationController, *spreadController);
    effectExecutor
        = std::make_unique<ImageDocumentEffectExecutor>(*navigationController, *predecodeController,
            *openController, *presentationController, *spreadController, *loadController);
    navigator = std::make_unique<ImageDocumentNavigator>(
        *navigationController, *spreadController, *loadController);
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
}
}
