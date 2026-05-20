// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimecontrollers.h"

#include "archive/archivedocumentsessionstore.h"
#include "async/imagecallback.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumenteffectexecutor.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentruntimedependencies.h"
#include "imagedocumentruntimeeffectbinding.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentstate.h"
#include "imageerrortext.h"
#include "imageopencontroller.h"
#include "navigation/imagenavigationservice.h"
#include "presentation/imagepresentationcontroller.h"
#include "presentation/imagespreadpresentationcontroller.h"

#include <QObject>
#include <QUrl>
#include <utility>

namespace KiriView {
namespace {
    ImageDocumentRenderContext renderContextOrDefault(
        const std::function<ImageDocumentRenderContext()> &renderContext)
    {
        return renderContext ? renderContext() : ImageDocumentRenderContext {};
    }
}

ImageDocumentRuntimeControllers::ImageDocumentRuntimeControllers(QObject *documentObject,
    ImageDocumentState &state, ImageDocumentRuntimeDependencyOverrides dependencies,
    ImageDocumentRuntimeControllerCallbacks callbacks)
    : m_callbacks(std::move(callbacks))
{
    ImageDocumentRuntimeDependencies runtimeDependencies
        = resolveImageDocumentRuntimeDependencies(std::move(dependencies), documentObject);
    m_archiveSessionStore = std::move(runtimeDependencies.archiveSessionStore);
    m_presentationController = std::make_unique<ImagePresentationController>(
        documentObject, [this]() { return renderContextOrDefault(m_callbacks.renderContext); },
        ImagePresentationController::Callbacks {
            [this](ImageDocumentChange change) { invokeIfSet(m_callbacks.notify, change); },
            [this](const QString &errorString) {
                m_openController->finishAnimationLoadWithError(errorString);
            },
        });
    m_deletionController = std::make_unique<ImageDocumentDeletionController>(documentObject, state,
        *m_presentationController, runtimeDependencies.candidateProvider,
        std::move(runtimeDependencies.fileOperations),
        ImageDocumentDeletionController::Callbacks {
            [this]() {
                invokeIfSet(m_callbacks.notify, ImageDocumentChange::FileDeletionInProgress);
            },
            [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            std::move(m_callbacks.fileDeletionFailed),
        });
    m_openController
        = std::make_unique<ImageOpenController>(documentObject, state, *m_presentationController,
            ImageOpenController::Callbacks {
                [this](const QUrl &url) { return m_predecodeController->findPredecodedImage(url); },
                [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); },
            },
            runtimeDependencies.candidateProvider, runtimeDependencies.imageDecode);
    m_navigationService = std::make_unique<ImageNavigationService>(documentObject,
        runtimeDependencies.candidateProvider,
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
                        url, imageErrorText(ImageErrorTextId::OpenComicBookArchive)));
                    return;
                }

                dispatchEffect(ImageDocumentEffect::containerNavigationFailed(url, message));
            },
            [this]() { invokeIfSet(m_callbacks.notify, ImageDocumentChange::PageNavigation); },
            [this]() { dispatchEffect(ImageDocumentEffect::clearDeletedImage()); },
            [this]() { return m_deletionController->inProgress(); },
        });
    m_predecodeController = std::make_unique<ImageDocumentPredecodeController>(
        documentObject, state, *m_presentationController, runtimeDependencies.candidateProvider,
        runtimeDependencies.imageDecode,
        [this]() { return m_navigationService->currentPageNumber(); },
        std::move(runtimeDependencies.powerSaver));
    m_spreadController = std::make_unique<ImageSpreadPresentationController>(
        documentObject, [this]() { return renderContextOrDefault(m_callbacks.renderContext); },
        state, *m_presentationController,
        ImageSpreadPresentationController::Callbacks {
            [this](ImageDocumentChange change) { invokeIfSet(m_callbacks.notify, change); },
            [this](const QUrl &url) { return m_predecodeController->findPredecodedImage(url); },
            [this]() { return m_navigationController->pageNavigationSnapshot(); },
            [this]() { dispatchEffect(ImageDocumentEffect::scheduleAdjacentImagePredecode()); },
        },
        runtimeDependencies.candidateProvider, runtimeDependencies.imageDecode);
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(state,
        *m_presentationController, *m_navigationService, *m_spreadController,
        [this](ImageDocumentEffect effect) { dispatchEffect(std::move(effect)); });
    m_effectExecutor = std::make_unique<ImageDocumentEffectExecutor>(
        imageDocumentRuntimeEffectOperations(ImageDocumentRuntimeEffectBinding {
            m_archiveSessionStore.get(),
            state,
            *m_deletionController,
            *m_presentationController,
            *m_openController,
            *m_navigationController,
            *m_predecodeController,
            *m_spreadController,
            [this](const ImageDocumentSourceLoadRequest &request) {
                invokeIfSet(m_callbacks.loadSource, request);
            },
        }));
}

ImageDocumentRuntimeControllers::~ImageDocumentRuntimeControllers() = default;

ImageDocumentDeletionController &ImageDocumentRuntimeControllers::deletionController() const
{
    return *m_deletionController;
}

ImagePresentationController &ImageDocumentRuntimeControllers::presentationController() const
{
    return *m_presentationController;
}

ImageDocumentNavigationController &ImageDocumentRuntimeControllers::navigationController() const
{
    return *m_navigationController;
}

ImageSpreadPresentationController &ImageDocumentRuntimeControllers::spreadController() const
{
    return *m_spreadController;
}

void ImageDocumentRuntimeControllers::dispatchEffect(ImageDocumentEffect effect)
{
    if (m_effectExecutor != nullptr) {
        m_effectExecutor->dispatch(std::move(effect));
    }
}

void ImageDocumentRuntimeControllers::dispatchPlan(const ImageDocumentRuntimePlan &plan)
{
    if (m_effectExecutor != nullptr) {
        m_effectExecutor->dispatchPlan(plan);
    }
}

void ImageDocumentRuntimeControllers::shutdownRuntime()
{
    if (m_effectExecutor != nullptr) {
        m_effectExecutor->shutdownRuntime();
    }
}
}
