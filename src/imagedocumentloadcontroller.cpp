// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadcontroller.h"

#include "archivedocumentsessionstore.h"
#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagespreadpresentationcontroller.h"

namespace {
void cancelNavigationAndPredecode(KiriView::ImageDocumentNavigationController &navigationController,
    KiriView::ImageDocumentPredecodeController &predecodeController)
{
    navigationController.cancelNavigation();
    navigationController.cancelContainerNavigation();
    predecodeController.cancel();
}
}

namespace KiriView {
ImageDocumentLoadController::ImageDocumentLoadController(ImageDocumentState &state,
    ImageDocumentDeletionController &deletionController,
    ImageDocumentNavigationController &navigationController,
    ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
    ImageSpreadPresentationController &spreadController,
    ArchiveDocumentSessionStore *archiveSessionStore)
    : m_state(state)
    , m_deletionController(deletionController)
    , m_navigationController(navigationController)
    , m_predecodeController(predecodeController)
    , m_openController(openController)
    , m_spreadController(spreadController)
    , m_archiveSessionStore(archiveSessionStore)
{
}

void ImageDocumentLoadController::loadSource(const ImageDocumentSourceLoadRequest &request)
{
    m_deletionController.cancel();

    const ImageDocumentSourceLoadPlan plan
        = ImageOpenWorkflow::sourceLoadPlan(m_state, m_spreadController, request);
    applySourceLoadPlan(request, plan);
}

void ImageDocumentLoadController::applySourceLoadPlan(
    const ImageDocumentSourceLoadRequest &request, const ImageDocumentSourceLoadPlan &plan)
{
    if (plan.cancelNavigationAndPredecode) {
        ::cancelNavigationAndPredecode(m_navigationController, m_predecodeController);
    }
    if (plan.finishSpreadTransition) {
        m_spreadController.finishTransition();
    }
    applyRightToLeftReadingTransition(plan.rightToLeftReadingTransition, true, false);
    if (plan.clearSecondaryPage) {
        m_spreadController.clearSecondaryPage();
    }
    applyLoadingContainerNavigationUrlTarget(plan.loadingContainerNavigationUrl, request);
    applyContainerNavigationUrlTarget(plan.containerNavigationUrl, request);
    applySourceLoadUrlTarget(plan.sourceUrl, request);
    if (plan.beginOpen) {
        m_openController.open();
    }
    applyRightToLeftReadingTransition(plan.rightToLeftReadingTransition, false, true);
}

void ImageDocumentLoadController::applyRightToLeftReadingTransition(
    ImageDocumentRightToLeftReadingTransition transition, bool notifyBeforeSourceState,
    bool notifyAfterOpen)
{
    switch (transition) {
    case ImageDocumentRightToLeftReadingTransition::Keep:
        return;
    case ImageDocumentRightToLeftReadingTransition::Reset:
        if (notifyBeforeSourceState) {
            m_spreadController.resetRightToLeftReading();
        }
        return;
    case ImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState:
        if (notifyBeforeSourceState) {
            m_spreadController.resetRightToLeftReading();
            m_spreadController.notifyRightToLeftReadingChanged();
        }
        return;
    case ImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen:
        if (notifyBeforeSourceState) {
            m_spreadController.resetRightToLeftReading();
        }
        if (notifyAfterOpen) {
            m_spreadController.notifyRightToLeftReadingChanged();
        }
        return;
    }
}

void ImageDocumentLoadController::applyLoadingContainerNavigationUrlTarget(
    ImageDocumentSourceLoadUrlTarget target, const ImageDocumentSourceLoadRequest &request)
{
    switch (target) {
    case ImageDocumentSourceLoadUrlTarget::Unchanged:
        return;
    case ImageDocumentSourceLoadUrlTarget::Empty:
        m_state.clearLoadingContainerNavigationUrl();
        return;
    case ImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation:
        m_state.setLoadingContainerNavigationUrl(request.containerNavigationUrl);
        return;
    case ImageDocumentSourceLoadUrlTarget::RequestedSource:
        return;
    }
}

void ImageDocumentLoadController::applyContainerNavigationUrlTarget(
    ImageDocumentSourceLoadUrlTarget target, const ImageDocumentSourceLoadRequest &request)
{
    switch (target) {
    case ImageDocumentSourceLoadUrlTarget::Unchanged:
    case ImageDocumentSourceLoadUrlTarget::Empty:
    case ImageDocumentSourceLoadUrlTarget::RequestedSource:
        return;
    case ImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation:
        m_state.setContainerNavigationUrl(request.containerNavigationUrl);
        return;
    }
}

void ImageDocumentLoadController::applySourceLoadUrlTarget(
    ImageDocumentSourceLoadUrlTarget target, const ImageDocumentSourceLoadRequest &request)
{
    switch (target) {
    case ImageDocumentSourceLoadUrlTarget::Unchanged:
    case ImageDocumentSourceLoadUrlTarget::Empty:
    case ImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation:
        return;
    case ImageDocumentSourceLoadUrlTarget::RequestedSource:
        if (m_archiveSessionStore != nullptr) {
            m_archiveSessionStore->prepareForSourceLoad(
                request, m_state.displayedArchiveDocument());
        }
        m_state.setSourceUrl(request.sourceUrl);
        return;
    }
}

}
