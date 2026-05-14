// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadcontroller.h"

#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagepresentationcontroller.h"
#include "imagespreadpresentationcontroller.h"

#include <QString>

namespace KiriView {
ImageDocumentLoadController::ImageDocumentLoadController(ImageDocumentState &state,
    ImageDocumentDeletionController &deletionController,
    ImageDocumentNavigationController &navigationController,
    ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
    ImagePresentationController &presentationController,
    ImageSpreadPresentationController &spreadController)
    : m_state(state)
    , m_deletionController(deletionController)
    , m_navigationController(navigationController)
    , m_predecodeController(predecodeController)
    , m_openController(openController)
    , m_presentationController(presentationController)
    , m_spreadController(spreadController)
{
}

void ImageDocumentLoadController::loadSource(const ImageDocumentSourceLoadRequest &request)
{
    m_deletionController.cancel();

    const ImageOpenSourceLoadPlan plan
        = ImageOpenWorkflow::sourceLoadPlan(sourceLoadWorkflowRequest(request));
    applySourceLoadPlan(request, plan);
}

ImageOpenSourceLoadRequest ImageDocumentLoadController::sourceLoadWorkflowRequest(
    const ImageDocumentSourceLoadRequest &request) const
{
    const bool sourceUrlChanged = m_state.sourceUrl() != request.sourceUrl;
    const bool resetRightToLeftReading = m_spreadController.shouldResetRightToLeftReadingForLoad(
        m_state.displayedArchiveDocument(), request.sourceUrl, request.containerNavigationUrl);

    ImageOpenSourceLoadRequest openRequest;
    openRequest.sourceUrlChanged = sourceUrlChanged;
    openRequest.preserveTwoPageSpreadTransition = request.preserveTwoPageSpreadTransition;
    openRequest.resetRightToLeftReading = resetRightToLeftReading;
    openRequest.rightToLeftReadingEnabled = m_spreadController.rightToLeftReadingEnabled();
    openRequest.containerNavigationUrlEmpty = request.containerNavigationUrl.isEmpty();
    return openRequest;
}

void ImageDocumentLoadController::applySourceLoadPlan(
    const ImageDocumentSourceLoadRequest &request, const ImageOpenSourceLoadPlan &plan)
{
    for (ImageOpenSourceLoadAction action : plan.actions) {
        applySourceLoadAction(request, action);
    }
}

void ImageDocumentLoadController::applySourceLoadAction(
    const ImageDocumentSourceLoadRequest &request, ImageOpenSourceLoadAction action)
{
    switch (action) {
    case ImageOpenSourceLoadAction::CancelNavigationAndPredecode:
        cancelNavigationAndPredecode();
        return;
    case ImageOpenSourceLoadAction::FinishSpreadTransition:
        m_spreadController.finishTransition();
        return;
    case ImageOpenSourceLoadAction::ResetRightToLeftReading:
        m_spreadController.resetRightToLeftReading();
        return;
    case ImageOpenSourceLoadAction::NotifyRightToLeftReadingBeforeOpen:
    case ImageOpenSourceLoadAction::NotifyRightToLeftReadingAfterOpen:
        m_spreadController.notifyRightToLeftReadingChanged();
        return;
    case ImageOpenSourceLoadAction::ClearSecondaryPage:
        m_spreadController.clearSecondaryPage();
        return;
    case ImageOpenSourceLoadAction::ClearLoadingContainerNavigationUrl:
        m_state.clearLoadingContainerNavigationUrl();
        return;
    case ImageOpenSourceLoadAction::UpdateContainerNavigationUrl:
        m_state.setContainerNavigationUrl(request.containerNavigationUrl);
        return;
    case ImageOpenSourceLoadAction::SetLoadingContainerNavigationUrl:
        m_state.setLoadingContainerNavigationUrl(request.containerNavigationUrl);
        return;
    case ImageOpenSourceLoadAction::SetSourceUrl:
        m_state.setSourceUrl(request.sourceUrl);
        return;
    case ImageOpenSourceLoadAction::BeginOpen:
        m_openController.open();
        return;
    }
}

void ImageDocumentLoadController::clearImage()
{
    m_predecodeController.clear();
    m_spreadController.finishTransition();
    m_spreadController.clearSecondaryPage();
    m_navigationController.cancelPageNavigationUpdate();
    m_state.clearDisplayedImageLocation();
    m_presentationController.clearImage();
    m_navigationController.clearPageNavigation();
    m_spreadController.notifyRightToLeftReadingChanged();
}

ImageDocumentEffects ImageDocumentLoadController::clearAfterSuccessfulFileDeletion()
{
    cancelNavigationAndPredecode();
    m_openController.cancel();
    m_spreadController.finishTransition();
    m_spreadController.clearSecondaryPage();
    m_state.setSourceUrl(QUrl());
    m_state.setErrorString(QString());
    return ImageOpenWorkflow::finishEmptySourceLoad(m_state);
}

void ImageDocumentLoadController::cancelNavigationAndPredecode()
{
    m_navigationController.cancelNavigation();
    m_navigationController.cancelContainerNavigation();
    m_predecodeController.cancel();
}
}
