// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimecontrollers.h"

#include "archive/archivedocumentsessionstore.h"
#include "async/imagecallback.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentnavigationruntimeplan.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentruntimedependencies.h"
#include "imagedocumentruntimeplanexecutor.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
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
        },
        runtimeDependencies.predecodeCacheByteBudget);
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
            std::move(m_callbacks.unsupportedDocumentVideoEntered),
        },
        runtimeDependencies.candidateProvider, runtimeDependencies.imageDecode);
    m_navigationService = std::make_unique<ImageNavigationService>(documentObject,
        runtimeDependencies.candidateProvider,
        ImageNavigationService::Callbacks {
            [this](ImageNavigationPlan plan) {
                dispatchPlan(imageDocumentRuntimePlanForNavigationPlan(plan));
            },
            [this]() { invokeIfSet(m_callbacks.notify, ImageDocumentChange::PageNavigation); },
            [this]() { return m_deletionController->inProgress(); },
        });
    m_predecodeController = std::make_unique<ImageDocumentPredecodeController>(
        documentObject, state, *m_presentationController, runtimeDependencies.candidateProvider,
        runtimeDependencies.imageDecode, runtimeDependencies.predecodeCacheByteBudget,
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
        runtimeDependencies.candidateProvider, runtimeDependencies.imageDecode,
        runtimeDependencies.predecodeCacheByteBudget);
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(state,
        *m_presentationController, *m_navigationService, *m_spreadController,
        [this](ImageDocumentRuntimePlan plan) { dispatchPlan(plan); });
    m_runtimePlanExecutor
        = std::make_unique<ImageDocumentRuntimePlanExecutor>(runtimeOperations(state));
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

ImageDocumentRuntimeOperations ImageDocumentRuntimeControllers::runtimeOperations(
    ImageDocumentState &state)
{
    ImageDocumentState *stateOwner = &state;

    ImageDocumentRuntimeOperations operations;
    operations.lifecycle.cancelFileDeletion = [this]() { m_deletionController->cancel(); };
    operations.lifecycle.stopPresentationAnimation
        = [this]() { m_presentationController->stopAnimation(); };
    operations.lifecycle.shutdownSpread = [this]() { m_spreadController->shutdown(); };
    operations.archive.clearSession = [this]() {
        if (m_archiveSessionStore != nullptr) {
            m_archiveSessionStore->clear();
        }
    };
    operations.predecode.clearPredecode = [this]() { m_predecodeController->clear(); };
    operations.predecode.cancelPredecode = [this]() { m_predecodeController->cancel(); };
    operations.predecode.scheduleAdjacentImagePredecode = [this]() {
        m_predecodeController->scheduleAdjacentImagePredecode(
            m_spreadController->secondaryDisplayedPredecodeImage());
    };
    operations.spread.finishSpreadTransition = [this]() { m_spreadController->finishTransition(); };
    operations.spread.resetRightToLeftReading
        = [this]() { m_spreadController->resetRightToLeftReading(); };
    operations.spread.clearSecondaryPage = [this]() { m_spreadController->clearSecondaryPage(); };
    operations.spread.notifyRightToLeftReadingChanged
        = [this]() { m_spreadController->notifyRightToLeftReadingChanged(); };
    operations.spread.resetZoom = [this]() { m_spreadController->resetZoom(); };
    operations.spread.prepareFailedContainer = [this](const QUrl &containerUrl) {
        m_presentationController->prepareFailedContainer(containerUrl);
    };
    operations.navigation.cancelPageNavigationUpdate
        = [this]() { m_navigationController->cancelPageNavigationUpdate(); };
    operations.navigation.cancelNavigation
        = [this]() { m_navigationController->cancelNavigation(); };
    operations.navigation.cancelContainerNavigation
        = [this]() { m_navigationController->cancelContainerNavigation(); };
    operations.navigation.cancelAllNavigation
        = [this]() { m_navigationController->cancelAllNavigation(); };
    operations.navigation.clearPageNavigation
        = [this]() { m_navigationController->clearPageNavigation(); };
    operations.navigation.updatePageNavigation
        = [this]() { m_navigationController->updatePageNavigation(); };
    operations.navigation.loadUrl = [this](const ImageNavigationTarget &target) {
        invokeIfSet(m_callbacks.loadSource, ImageDocumentSourceLoadRequest::fromTarget(target));
    };
    operations.navigation.loadContainerImage
        = [this](const ImageNavigationTarget &target, const QUrl &containerUrl) {
              invokeIfSet(m_callbacks.loadSource,
                  ImageDocumentSourceLoadRequest::fromContainerTarget(target, containerUrl));
          };
    operations.navigation.finishEmptyContainerNavigation = [this](const QUrl &containerUrl) {
        m_openController->finishContainerNavigationWithEmptyContainer(containerUrl);
    };
    operations.navigation.finishContainerNavigationLoadWithError
        = [this](const QUrl &containerUrl, const QString &errorString) {
              m_openController->finishContainerNavigationLoadWithError(containerUrl, errorString);
          };
    operations.navigation.loadPageNavigationUrl
        = [this](const ImageNavigationTarget &target, bool preserveTwoPageSpreadTransition) {
              invokeIfSet(m_callbacks.loadSource,
                  ImageDocumentSourceLoadRequest::fromPageNavigationTarget(
                      target, preserveTwoPageSpreadTransition));
          };
    operations.open.cancelOpen = [this]() { m_openController->cancel(); };
    operations.open.clearDisplayedImageLocation
        = [stateOwner]() { stateOwner->clearDisplayedImageLocation(); };
    operations.open.clearPresentationImage = [this]() { m_presentationController->clearImage(); };
    operations.sourceLoad.clearLoadingContainerNavigationUrl
        = [stateOwner]() { stateOwner->clearLoadingContainerNavigationUrl(); };
    operations.sourceLoad.setLoadingContainerNavigationUrl
        = [stateOwner](const QUrl &url) { stateOwner->setLoadingContainerNavigationUrl(url); };
    operations.sourceLoad.setContainerNavigationUrl
        = [stateOwner](const QUrl &url) { stateOwner->setContainerNavigationUrl(url); };
    operations.sourceLoad.prepareSourceLoad
        = [this, stateOwner](const ImageDocumentSourceLoadRequest &request) {
              if (m_archiveSessionStore != nullptr) {
                  m_archiveSessionStore->prepareForSourceLoad(
                      request, stateOwner->displayedImagePageScope());
              }
          };
    operations.open.setSourceUrl = [stateOwner](const ImageNavigationTarget &target) {
        stateOwner->setSourceKind(target.kind);
        stateOwner->setSourceUrl(target.url);
    };
    operations.sourceLoad.beginOpen = [this]() { m_openController->open(); };
    operations.open.setErrorString
        = [stateOwner](const QString &errorString) { stateOwner->setErrorString(errorString); };
    operations.open.finishEmptySourceLoad = [this]() { m_openController->finishEmptySourceLoad(); };
    return operations;
}
}
