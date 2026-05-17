// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumenteffectexecutor.h"

#include "archivedocumentsessionstore.h"
#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <QString>
#include <QUrl>
#include <utility>
#include <variant>

namespace KiriView {
namespace {
    template <typename Operation, typename... Args>
    void run(const Operation &operation, Args &&...args)
    {
        if (operation) {
            operation(std::forward<Args>(args)...);
        }
    }

    ImageDocumentEffects generatedEffects(const std::function<ImageDocumentEffects()> &operation)
    {
        return operation ? operation() : ImageDocumentEffects {};
    }

    ImageDocumentEffectOperations effectOperations(ImageDocumentState &state,
        ImageDocumentNavigationController &navigationController,
        ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
        ImagePresentationController &presentationController,
        ImageSpreadPresentationController &spreadController,
        ImageDocumentLoadController &loadController,
        ArchiveDocumentSessionStore *archiveSessionStore)
    {
        ImageDocumentEffectOperations operations;
        operations.clearArchiveSession = [archiveSessionStore]() {
            if (archiveSessionStore != nullptr) {
                archiveSessionStore->clear();
            }
        };
        operations.clearPredecode = [&predecodeController]() { predecodeController.clear(); };
        operations.cancelPredecode = [&predecodeController]() { predecodeController.cancel(); };
        operations.finishSpreadTransition
            = [&spreadController]() { spreadController.finishTransition(); };
        operations.clearSecondaryPage
            = [&spreadController]() { spreadController.clearSecondaryPage(); };
        operations.cancelPageNavigationUpdate
            = [&navigationController]() { navigationController.cancelPageNavigationUpdate(); };
        operations.cancelNavigation
            = [&navigationController]() { navigationController.cancelNavigation(); };
        operations.cancelContainerNavigation
            = [&navigationController]() { navigationController.cancelContainerNavigation(); };
        operations.cancelOpen = [&openController]() { openController.cancel(); };
        operations.clearDisplayedImageLocation
            = [&state]() { state.clearDisplayedImageLocation(); };
        operations.clearPresentationImage
            = [&presentationController]() { presentationController.clearImage(); };
        operations.clearPageNavigation
            = [&navigationController]() { navigationController.clearPageNavigation(); };
        operations.notifyRightToLeftReadingChanged
            = [&spreadController]() { spreadController.notifyRightToLeftReadingChanged(); };
        operations.resetZoom = [&spreadController]() { spreadController.resetZoom(); };
        operations.updatePageNavigation
            = [&navigationController]() { navigationController.updatePageNavigation(); };
        operations.scheduleAdjacentImagePredecode = [&predecodeController, &spreadController]() {
            predecodeController.scheduleAdjacentImagePredecode(
                spreadController.secondaryDisplayedPredecodeImage());
        };
        operations.loadUrl = [&loadController](const QUrl &url) {
            loadController.loadSource(ImageDocumentSourceLoadRequest::fromUrl(url));
        };
        operations.loadContainerImage
            = [&loadController](const QUrl &imageUrl, const QUrl &containerUrl) {
                  loadController.loadSource(
                      ImageDocumentSourceLoadRequest::fromContainerImage(imageUrl, containerUrl));
              };
        operations.finishEmptyContainerNavigation = [&openController](const QUrl &containerUrl) {
            openController.finishContainerNavigationWithEmptyContainer(containerUrl);
        };
        operations.finishContainerNavigationLoadWithError
            = [&openController](const QUrl &containerUrl, const QString &errorString) {
                  openController.finishContainerNavigationLoadWithError(containerUrl, errorString);
              };
        operations.loadPageNavigationUrl
            = [&loadController](const QUrl &url, bool preserveTwoPageSpreadTransition) {
                  loadController.loadSource(ImageDocumentSourceLoadRequest::fromPageNavigation(
                      url, preserveTwoPageSpreadTransition));
              };
        operations.prepareFailedContainer = [&presentationController](const QUrl &containerUrl) {
            presentationController.prepareFailedContainer(containerUrl);
        };
        operations.setSourceUrl = [&state](const QUrl &url) { state.setSourceUrl(url); };
        operations.setErrorString
            = [&state](const QString &errorString) { state.setErrorString(errorString); };
        operations.finishEmptySourceLoad
            = [&state]() { return ImageOpenWorkflow::finishEmptySourceLoad(state); };
        return operations;
    }
}

ImageDocumentEffectExecutor::ImageDocumentEffectExecutor(ImageDocumentState &state,
    ImageDocumentNavigationController &navigationController,
    ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
    ImagePresentationController &presentationController,
    ImageSpreadPresentationController &spreadController,
    ImageDocumentLoadController &loadController, ArchiveDocumentSessionStore *archiveSessionStore)
    : ImageDocumentEffectExecutor(
          effectOperations(state, navigationController, predecodeController, openController,
              presentationController, spreadController, loadController, archiveSessionStore))
{
}

ImageDocumentEffectExecutor::ImageDocumentEffectExecutor(ImageDocumentEffectOperations operations)
    : m_operations(std::move(operations))
{
}

void ImageDocumentEffectExecutor::dispatch(ImageDocumentEffect effect)
{
    std::visit([this](const auto &payload) { dispatchPayload(payload); }, effect.payload);
}

void ImageDocumentEffectExecutor::dispatchGeneratedEffects(ImageDocumentEffects effects)
{
    for (ImageDocumentEffect &effect : effects) {
        dispatch(std::move(effect));
    }
}

void ImageDocumentEffectExecutor::clearImage()
{
    run(m_operations.clearArchiveSession);
    run(m_operations.clearPredecode);
    run(m_operations.finishSpreadTransition);
    run(m_operations.clearSecondaryPage);
    run(m_operations.cancelPageNavigationUpdate);
    run(m_operations.clearDisplayedImageLocation);
    run(m_operations.clearPresentationImage);
    run(m_operations.clearPageNavigation);
    run(m_operations.notifyRightToLeftReadingChanged);
}

ImageDocumentEffects ImageDocumentEffectExecutor::clearAfterSuccessfulFileDeletion()
{
    run(m_operations.clearArchiveSession);
    run(m_operations.cancelNavigation);
    run(m_operations.cancelContainerNavigation);
    run(m_operations.cancelPredecode);
    run(m_operations.cancelOpen);
    run(m_operations.finishSpreadTransition);
    run(m_operations.clearSecondaryPage);
    run(m_operations.setSourceUrl, QUrl());
    run(m_operations.setErrorString, QString());
    return generatedEffects(m_operations.finishEmptySourceLoad);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ClearImageEffect &) { clearImage(); }

void ImageDocumentEffectExecutor::dispatchPayload(const ClearDeletedImageEffect &)
{
    dispatchGeneratedEffects(clearAfterSuccessfulFileDeletion());
}

void ImageDocumentEffectExecutor::dispatchPayload(const ResetZoomEffect &)
{
    run(m_operations.resetZoom);
}

void ImageDocumentEffectExecutor::dispatchPayload(const UpdatePageNavigationEffect &)
{
    run(m_operations.updatePageNavigation);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ScheduleAdjacentImagePredecodeEffect &)
{
    run(m_operations.scheduleAdjacentImagePredecode);
}

void ImageDocumentEffectExecutor::dispatchPayload(const OpenUrlEffect &payload)
{
    run(m_operations.loadUrl, payload.url);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ContainerImageSelectedEffect &payload)
{
    run(m_operations.loadContainerImage, payload.imageUrl, payload.containerUrl);
}

void ImageDocumentEffectExecutor::dispatchPayload(const EmptyContainerSelectedEffect &payload)
{
    run(m_operations.finishEmptyContainerNavigation, payload.containerUrl);
}

void ImageDocumentEffectExecutor::dispatchPayload(const ContainerNavigationFailedEffect &payload)
{
    run(m_operations.finishContainerNavigationLoadWithError, payload.containerUrl,
        payload.errorString);
}

void ImageDocumentEffectExecutor::dispatchPayload(const PageNavigationSelectedEffect &payload)
{
    run(m_operations.loadPageNavigationUrl, payload.url, payload.preserveTwoPageSpreadTransition);
}

void ImageDocumentEffectExecutor::dispatchPayload(const PrepareFailedContainerEffect &payload)
{
    run(m_operations.prepareFailedContainer, payload.containerUrl);
}
}
