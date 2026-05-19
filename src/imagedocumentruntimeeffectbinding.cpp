// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentruntimeeffectbinding.h"

#include "archivedocumentsessionstore.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentloadcontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imageopentransitionapplier.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

namespace KiriView {
ImageDocumentEffectOperations imageDocumentRuntimeEffectOperations(
    ImageDocumentRuntimeEffectBinding binding)
{
    ArchiveDocumentSessionStore *archiveSessionStore = binding.archiveSessionStore;
    ImageDocumentState *state = &binding.state;
    ImageDocumentDeletionController *deletionController = &binding.deletionController;
    ImagePresentationController *presentationController = &binding.presentationController;
    ImageOpenController *openController = &binding.openController;
    ImageDocumentNavigationController *navigationController = &binding.navigationController;
    ImageDocumentPredecodeController *predecodeController = &binding.predecodeController;
    ImageSpreadPresentationController *spreadController = &binding.spreadController;
    ImageDocumentLoadController *loadController = &binding.loadController;

    ImageDocumentEffectOperations operations;
    operations.lifecycle.cancelFileDeletion
        = [deletionController]() { deletionController->cancel(); };
    operations.lifecycle.stopPresentationAnimation
        = [presentationController]() { presentationController->stopAnimation(); };
    operations.lifecycle.shutdownSpread = [spreadController]() { spreadController->shutdown(); };
    operations.archive.clearSession = [archiveSessionStore]() {
        if (archiveSessionStore != nullptr) {
            archiveSessionStore->clear();
        }
    };
    operations.predecode.clearPredecode = [predecodeController]() { predecodeController->clear(); };
    operations.predecode.cancelPredecode
        = [predecodeController]() { predecodeController->cancel(); };
    operations.predecode.scheduleAdjacentImagePredecode
        = [predecodeController, spreadController]() {
              predecodeController->scheduleAdjacentImagePredecode(
                  spreadController->secondaryDisplayedPredecodeImage());
          };
    operations.spread.finishSpreadTransition
        = [spreadController]() { spreadController->finishTransition(); };
    operations.spread.resetRightToLeftReading
        = [spreadController]() { spreadController->resetRightToLeftReading(); };
    operations.spread.clearSecondaryPage
        = [spreadController]() { spreadController->clearSecondaryPage(); };
    operations.spread.notifyRightToLeftReadingChanged
        = [spreadController]() { spreadController->notifyRightToLeftReadingChanged(); };
    operations.spread.resetZoom = [spreadController]() { spreadController->resetZoom(); };
    operations.spread.prepareFailedContainer = [presentationController](const QUrl &containerUrl) {
        presentationController->prepareFailedContainer(containerUrl);
    };
    operations.navigation.cancelPageNavigationUpdate
        = [navigationController]() { navigationController->cancelPageNavigationUpdate(); };
    operations.navigation.cancelNavigation
        = [navigationController]() { navigationController->cancelNavigation(); };
    operations.navigation.cancelContainerNavigation
        = [navigationController]() { navigationController->cancelContainerNavigation(); };
    operations.navigation.cancelAllNavigation
        = [navigationController]() { navigationController->cancelAllNavigation(); };
    operations.navigation.clearPageNavigation
        = [navigationController]() { navigationController->clearPageNavigation(); };
    operations.navigation.updatePageNavigation
        = [navigationController]() { navigationController->updatePageNavigation(); };
    operations.navigation.loadUrl = [loadController](const QUrl &url) {
        loadController->loadSource(ImageDocumentSourceLoadRequest::fromUrl(url));
    };
    operations.navigation.loadContainerImage
        = [loadController](const QUrl &imageUrl, const QUrl &containerUrl) {
              loadController->loadSource(
                  ImageDocumentSourceLoadRequest::fromContainerImage(imageUrl, containerUrl));
          };
    operations.navigation.finishEmptyContainerNavigation
        = [openController](const QUrl &containerUrl) {
              openController->finishContainerNavigationWithEmptyContainer(containerUrl);
          };
    operations.navigation.finishContainerNavigationLoadWithError
        = [openController](const QUrl &containerUrl, const QString &errorString) {
              openController->finishContainerNavigationLoadWithError(containerUrl, errorString);
          };
    operations.navigation.loadPageNavigationUrl
        = [loadController](const QUrl &url, bool preserveTwoPageSpreadTransition) {
              loadController->loadSource(ImageDocumentSourceLoadRequest::fromPageNavigation(
                  url, preserveTwoPageSpreadTransition));
          };
    operations.open.cancelOpen = [openController]() { openController->cancel(); };
    operations.open.clearDisplayedImageLocation
        = [state]() { state->clearDisplayedImageLocation(); };
    operations.open.clearPresentationImage
        = [presentationController]() { presentationController->clearImage(); };
    operations.sourceLoad.clearLoadingContainerNavigationUrl
        = [state]() { state->clearLoadingContainerNavigationUrl(); };
    operations.sourceLoad.setLoadingContainerNavigationUrl
        = [state](const QUrl &url) { state->setLoadingContainerNavigationUrl(url); };
    operations.sourceLoad.setContainerNavigationUrl
        = [state](const QUrl &url) { state->setContainerNavigationUrl(url); };
    operations.sourceLoad.prepareSourceLoad = [archiveSessionStore, state](
                                                  const ImageDocumentSourceLoadRequest &request) {
        if (archiveSessionStore != nullptr) {
            archiveSessionStore->prepareForSourceLoad(request, state->displayedArchiveDocument());
        }
    };
    operations.open.setSourceUrl = [state](const QUrl &url) { state->setSourceUrl(url); };
    operations.sourceLoad.beginOpen = [openController]() { openController->open(); };
    operations.open.setErrorString
        = [state](const QString &errorString) { state->setErrorString(errorString); };
    operations.open.finishEmptySourceLoad = [state]() {
        return applyImageOpenTransition(
            *state, ImageOpenWorkflow::finishEmptySourceLoadTransition());
    };
    return operations;
}
}
