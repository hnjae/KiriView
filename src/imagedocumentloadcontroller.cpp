// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadcontroller.h"

#include "imagedocumentdeletioncontroller.h"
#include "imagedocumentnavigationcontroller.h"
#include "imagedocumentpredecodecontroller.h"
#include "imagedocumentstate.h"
#include "imageopencontroller.h"
#include "imageopenworkflow.h"
#include "imagespreadpresentationcontroller.h"

#include <QString>

namespace KiriView {
ImageDocumentLoadController::ImageDocumentLoadController(ImageDocumentState &state,
    ImageDocumentDeletionController &deletionController,
    ImageDocumentNavigationController &navigationController,
    ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
    ImageSpreadPresentationController &spreadController)
    : m_state(state)
    , m_deletionController(deletionController)
    , m_navigationController(navigationController)
    , m_predecodeController(predecodeController)
    , m_openController(openController)
    , m_spreadController(spreadController)
{
}

void ImageDocumentLoadController::loadSource(const ImageDocumentSourceLoadRequest &request)
{
    m_deletionController.cancel();

    const bool sourceUrlChanged = m_state.sourceUrl() != request.sourceUrl;
    const bool resetRightToLeftReading = m_spreadController.shouldResetRightToLeftReadingForLoad(
        m_state.displayedArchiveDocument(), request.sourceUrl, request.containerNavigationUrl);
    const ImageOpenSourceLoadPlan plan = ImageOpenWorkflow::sourceLoadPlan(sourceUrlChanged,
        request.preserveTwoPageSpreadTransition, resetRightToLeftReading,
        request.containerNavigationUrl.isEmpty());
    if (plan.cancelNavigationAndPredecode) {
        cancelNavigationAndPredecode();
    }
    if (plan.finishSpreadTransition) {
        m_spreadController.finishTransition();
    }

    const bool notifyRightToLeftReading
        = plan.resetRightToLeftReading && m_spreadController.rightToLeftReadingEnabled();
    if (plan.resetRightToLeftReading) {
        m_spreadController.resetRightToLeftReading();
    }
    if (!sourceUrlChanged && notifyRightToLeftReading) {
        m_spreadController.notifyRightToLeftReadingChanged();
    }
    if (plan.clearSecondaryPage) {
        m_spreadController.clearSecondaryPage();
    }
    if (plan.clearLoadingContainerNavigationUrl) {
        m_state.clearLoadingContainerNavigationUrl();
    }
    if (plan.updateContainerNavigationUrl) {
        m_state.setContainerNavigationUrl(request.containerNavigationUrl);
    }
    if (plan.setLoadingContainerNavigationUrl) {
        m_state.setLoadingContainerNavigationUrl(request.containerNavigationUrl);
    }
    if (plan.setSourceUrl) {
        m_state.setSourceUrl(request.sourceUrl);
    }
    if (plan.beginOpen) {
        m_openController.open();
    }
    if (sourceUrlChanged && notifyRightToLeftReading) {
        m_spreadController.notifyRightToLeftReadingChanged();
    }
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
