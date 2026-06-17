// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimecontrollers.h"

#include "archive/mediaentrysourcestore.h"
#include "async/imagecallback.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentnavigationruntimeplan.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentpredecodedimagelookup.h"
#include "imagedocumentprimarypageslotport.h"
#include "imagedocumentruntimedependencies.h"
#include "imagedocumentruntimeworkflow.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "navigation/imagedocumentpagenavigationservice.h"
#include "presentation/imagepagesurfacecontroller.h"
#include "presentation/imagepresentationruntime.h"
#include "presentation/imagespreadpresentationcontroller.h"

#include <QObject>
#include <QUrl>
#include <optional>
#include <utility>

namespace kiriview {
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
    m_mediaEntrySourceStore = std::move(runtimeDependencies.mediaEntrySourceStore);
    m_pageSurfaceController = std::make_unique<ImagePageSurfaceController>(documentObject,
        ImagePageSurfaceController::Callbacks {
            [this](ImageDocumentChange change) { invokeIfSet(m_callbacks.notify, change); },
            [this](const QString &errorString) {
                m_openController->finishAnimationLoadWithError(errorString);
            },
        },
        runtimeDependencies.cacheBudgets);
    m_presentationRuntime = std::make_unique<ImagePresentationRuntime>(
        [this]() { return renderContextOrDefault(m_callbacks.renderContext); });
    m_deletionController = std::make_unique<ImageDocumentDeletionController>(documentObject, state,
        *m_pageSurfaceController, runtimeDependencies.candidateProvider,
        std::move(runtimeDependencies.fileDeletionProvider),
        ImageDocumentDeletionController::Callbacks {
            [this]() {
                invokeIfSet(m_callbacks.notify, ImageDocumentChange::FileDeletionInProgress);
            },
            [this](ImageDocumentRuntimePlan plan) { dispatchPlan(plan); },
            std::move(m_callbacks.fileDeletionFailed),
        });
    m_navigationService = std::make_unique<ImageDocumentPageNavigationService>(documentObject,
        runtimeDependencies.candidateProvider,
        ImageDocumentPageNavigationService::Callbacks {
            [this](ImageDocumentPageNavigationPlan plan) {
                dispatchPlan(imageDocumentRuntimePlanForNavigationPlan(plan));
            },
            [this]() { invokeIfSet(m_callbacks.notify, ImageDocumentChange::PageNavigation); },
            [this]() { return m_deletionController->inProgress(); },
        });
    m_predecodeController = std::make_unique<ImageDocumentPredecodeController>(
        documentObject, state, *m_pageSurfaceController, *m_presentationRuntime,
        runtimeDependencies.candidateProvider, runtimeDependencies.imageDecode,
        runtimeDependencies.cacheBudgets.predecodeCacheByteBudget,
        [this]() { return m_navigationService->currentPageNumber(); },
        std::move(runtimeDependencies.powerSaver),
        runtimeDependencies.ordinaryDirectMediaPredecodeEnabled,
        std::move(runtimeDependencies.predecodeTimerScheduler),
        std::move(runtimeDependencies.predecodeThreadCountProvider));
    m_predecodedImageLookup = std::make_unique<ImageDocumentPredecodedImageLookup>(
        std::move(externalPredecodedImageFinder), m_predecodeController.get());
    m_spreadController = std::make_unique<ImageSpreadPresentationController>(
        documentObject, [this]() { return renderContextOrDefault(m_callbacks.renderContext); },
        state, *m_pageSurfaceController, *m_presentationRuntime,
        ImageSpreadPresentationController::Callbacks {
            [this](ImageDocumentChange change) { invokeIfSet(m_callbacks.notify, change); },
            [this](const QUrl &url) { return m_predecodedImageLookup->find(url); },
            [this]() { return m_navigationController->pageNavigationSnapshot(); },
            [this]() {
                dispatchPlan(
                    ImageDocumentRuntimePlan { ScheduleAdjacentImagePredecodeOperation {} });
            },
        },
        runtimeDependencies.candidateProvider, runtimeDependencies.imageDecode,
        runtimeDependencies.cacheBudgets);
    m_primaryPageSlotPort
        = std::make_unique<ImageDocumentPrimaryPageSlotPort>(m_spreadController.get());
    m_openController = std::make_unique<ImageOpenController>(documentObject, state,
        *m_pageSurfaceController, *m_presentationRuntime,
        ImageOpenController::Callbacks {
            [this](const QUrl &url) { return m_predecodedImageLookup->find(url); },
            [this](const ImageDocumentRuntimePlan &plan) { dispatchPlan(plan); },
            std::move(m_callbacks.unsupportedOpenedCollectionVideoEntered),
            [this](const DisplayedImageLocation &location) {
                m_primaryPageSlotPort->commit(location);
            },
            [this]() { m_primaryPageSlotPort->clear(); },
        },
        runtimeDependencies.candidateProvider, runtimeDependencies.imageDecode);
    m_navigationController = std::make_unique<ImageDocumentNavigationController>(state,
        *m_pageSurfaceController, *m_navigationService, *m_spreadController,
        [this](ImageDocumentRuntimePlan plan) { dispatchPlan(plan); });
    m_runtimeWorkflow
        = std::make_unique<ImageDocumentRuntimeWorkflow>(ImageDocumentRuntimeWorkflowPorts {
            &state,
            m_mediaEntrySourceStore.get(),
            m_deletionController.get(),
            m_pageSurfaceController.get(),
            m_openController.get(),
            m_predecodeController.get(),
            m_spreadController.get(),
            m_navigationController.get(),
            m_callbacks.loadSource,
            m_callbacks.containerNavigationBoundaryReached,
        });
}

ImageDocumentRuntimeControllers::~ImageDocumentRuntimeControllers() = default;

ImageDocumentDeletionController &ImageDocumentRuntimeControllers::deletionController() const
{
    return *m_deletionController;
}

ImagePageSurfaceController &ImageDocumentRuntimeControllers::pageSurfaceController() const
{
    return *m_pageSurfaceController;
}

ImagePresentationRuntime &ImageDocumentRuntimeControllers::presentationRuntime() const
{
    return *m_presentationRuntime;
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
    if (m_runtimeWorkflow != nullptr) {
        m_runtimeWorkflow->dispatchPlan(plan);
    }
}

void ImageDocumentRuntimeControllers::shutdownRuntime()
{
    if (m_runtimeWorkflow != nullptr) {
        m_runtimeWorkflow->shutdownRuntime();
    }
}
}
