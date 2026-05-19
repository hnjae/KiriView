// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadcontroller.h"

#include "archivedocumentsessionstore.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentsourceloadexecutor.h"
#include "imagedocumentsourceloadpolicy.h"
#include "imagedocumentstate.h"
#include "imagenavigationservice.h"
#include "imageopencontroller.h"
#include "imagespreadpresentationcontroller.h"

namespace {
KiriView::ImageDocumentSourceLoadSnapshot sourceLoadSnapshot(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImageSpreadPresentationController &spreadController)
{
    return KiriView::ImageDocumentSourceLoadSnapshot {
        state.sourceUrl(),
        state.displayedArchiveDocument(),
        spreadController.rightToLeftReadingEnabled(),
    };
}

}

namespace KiriView {
ImageDocumentLoadController::ImageDocumentLoadController(ImageDocumentState &state,
    ImageDocumentDeletionController &deletionController, ImageNavigationService &navigationService,
    ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
    ImageSpreadPresentationController &spreadController,
    ArchiveDocumentSessionStore *archiveSessionStore)
    : m_state(state)
    , m_deletionController(deletionController)
    , m_navigationService(navigationService)
    , m_predecodeController(predecodeController)
    , m_openController(openController)
    , m_spreadController(spreadController)
    , m_archiveSessionStore(archiveSessionStore)
{
}

void ImageDocumentLoadController::loadSource(const ImageDocumentSourceLoadRequest &request)
{
    const ImageDocumentSourceLoadPlan plan
        = ImageDocumentSourceLoadPolicy::plan(imageDocumentSourceLoadPolicyInput(
            sourceLoadSnapshot(m_state, m_spreadController), request));
    executeImageDocumentSourceLoadPlan(request, plan, sourceLoadOperations());
}

ImageDocumentSourceLoadOperations ImageDocumentLoadController::sourceLoadOperations()
{
    ImageDocumentSourceLoadOperations operations;
    operations.cancelFileDeletion = [this]() { m_deletionController.cancel(); };
    operations.cancelNavigationAndPredecode = [this]() {
        m_navigationService.cancelAllNavigation();
        m_predecodeController.cancel();
    };
    operations.finishSpreadTransition = [this]() { m_spreadController.finishTransition(); };
    operations.resetRightToLeftReading = [this]() { m_spreadController.resetRightToLeftReading(); };
    operations.notifyRightToLeftReadingChanged
        = [this]() { m_spreadController.notifyRightToLeftReadingChanged(); };
    operations.clearSecondaryPage = [this]() { m_spreadController.clearSecondaryPage(); };
    operations.clearLoadingContainerNavigationUrl
        = [this]() { m_state.clearLoadingContainerNavigationUrl(); };
    operations.setLoadingContainerNavigationUrl
        = [this](const QUrl &url) { m_state.setLoadingContainerNavigationUrl(url); };
    operations.setContainerNavigationUrl
        = [this](const QUrl &url) { m_state.setContainerNavigationUrl(url); };
    operations.prepareSourceLoad = [this](const ImageDocumentSourceLoadRequest &request) {
        if (m_archiveSessionStore != nullptr) {
            m_archiveSessionStore->prepareForSourceLoad(
                request, m_state.displayedArchiveDocument());
        }
    };
    operations.setSourceUrl = [this](const QUrl &url) { m_state.setSourceUrl(url); };
    operations.beginOpen = [this]() { m_openController.open(); };
    return operations;
}

}
