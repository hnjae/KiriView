// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimecontrollers.h"

#include "archive/archivedocumentsessionstore.h"
#include "async/imagecallback.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentruntimedependencies.h"
#include "imagedocumentruntimeoperationbinding.h"
#include "imagedocumentruntimeplanexecutor.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "localization/imageerrortext.h"
#include "navigation/imagenavigationservice.h"
#include "presentation/imagepresentationcontroller.h"
#include "presentation/imagespreadpresentationcontroller.h"

#include <QObject>
#include <QUrl>
#include <optional>
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
    ExternalPredecodedImageFinder externalPredecodedImageFinder
        = std::move(runtimeDependencies.externalPredecodedImageFinder);
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
            [this](ImageDocumentRuntimePlan plan) { dispatchPlan(plan); },
            std::move(m_callbacks.fileDeletionFailed),
        });
    m_openController = std::make_unique<ImageOpenController>(documentObject, state,
        *m_presentationController,
        ImageOpenController::Callbacks {
            [this, externalPredecodedImageFinder = std::move(externalPredecodedImageFinder)](
                const QUrl &url) {
                if (externalPredecodedImageFinder) {
                    std::optional<PredecodedImage> predecoded = externalPredecodedImageFinder(url);
                    if (predecoded.has_value()) {
                        return predecoded;
                    }
                }

                return m_predecodeController->findPredecodedImage(url);
            },
            [this](const ImageDocumentRuntimePlan &plan) { dispatchPlan(plan); },
        },
        runtimeDependencies.candidateProvider, runtimeDependencies.imageDecode);
    m_navigationService = std::make_unique<ImageNavigationService>(documentObject,
        runtimeDependencies.candidateProvider,
        ImageNavigationService::Callbacks {
            [this](const QUrl &url) {
                dispatchPlan(ImageDocumentRuntimePlan { LoadUrlOperation { url } });
            },
            [this](const QUrl &imageUrl, const QUrl &containerUrl) {
                dispatchPlan(ImageDocumentRuntimePlan {
                    LoadContainerImageOperation { imageUrl, containerUrl },
                });
            },
            [this](const QUrl &url, ContainerNavigationError error, const QString &message) {
                if (error == ContainerNavigationError::EmptyContainer) {
                    dispatchPlan(ImageDocumentRuntimePlan {
                        FinishEmptyContainerNavigationOperation { url },
                    });
                    return;
                }

                if (error == ContainerNavigationError::InvalidComicBookArchive) {
                    dispatchPlan(ImageDocumentRuntimePlan {
                        FinishContainerNavigationLoadWithErrorOperation {
                            url,
                            imageErrorText(ImageErrorTextId::OpenComicBookArchive),
                        },
                    });
                    return;
                }

                dispatchPlan(ImageDocumentRuntimePlan {
                    FinishContainerNavigationLoadWithErrorOperation { url, message },
                });
            },
            [this]() { invokeIfSet(m_callbacks.notify, ImageDocumentChange::PageNavigation); },
            [this]() { dispatchPlan(imageDocumentClearDeletedImagePlan()); },
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
            [this]() {
                dispatchPlan(
                    ImageDocumentRuntimePlan { ScheduleAdjacentImagePredecodeOperation {} });
            },
        },
        runtimeDependencies.candidateProvider, runtimeDependencies.imageDecode);
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(state,
        *m_presentationController, *m_navigationService, *m_spreadController,
        [this](ImageDocumentRuntimePlan plan) { dispatchPlan(plan); });
    m_runtimePlanExecutor = std::make_unique<ImageDocumentRuntimePlanExecutor>(
        imageDocumentRuntimeOperations(ImageDocumentRuntimeOperationBinding {
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

void ImageDocumentRuntimeControllers::dispatchPlan(const ImageDocumentRuntimePlan &plan)
{
    if (m_runtimePlanExecutor != nullptr) {
        m_runtimePlanExecutor->dispatchPlan(plan);
    }
}

void ImageDocumentRuntimeControllers::shutdownRuntime()
{
    if (m_runtimePlanExecutor != nullptr) {
        m_runtimePlanExecutor->shutdownRuntime();
    }
}
}
