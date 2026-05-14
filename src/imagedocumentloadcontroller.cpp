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

namespace {
KiriView::ImageSourceLoadPolicyInput sourceLoadPolicyInput(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImageSpreadPresentationController &spreadController,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    const bool sourceUrlChanged = state.sourceUrl() != request.sourceUrl;
    KiriView::ImageSourceLoadPolicyInput input;
    input.source_url_changed = sourceUrlChanged;
    input.preserve_two_page_spread_transition = request.preserveTwoPageSpreadTransition;
    input.reset_right_to_left_reading = spreadController.shouldResetRightToLeftReadingForLoad(
        state.displayedArchiveDocument(), request.sourceUrl, request.containerNavigationUrl);
    input.right_to_left_reading_enabled = spreadController.rightToLeftReadingEnabled();
    input.container_navigation_url_empty = request.containerNavigationUrl.isEmpty();
    return input;
}

void cancelNavigationAndPredecode(KiriView::ImageDocumentNavigationController &navigationController,
    KiriView::ImageDocumentPredecodeController &predecodeController)
{
    navigationController.cancelNavigation();
    navigationController.cancelContainerNavigation();
    predecodeController.cancel();
}

class ImageSourceLoadPlanApplier final
{
public:
    ImageSourceLoadPlanApplier(KiriView::ImageDocumentState &state,
        KiriView::ImageDocumentNavigationController &navigationController,
        KiriView::ImageDocumentPredecodeController &predecodeController,
        KiriView::ImageOpenController &openController,
        KiriView::ImageSpreadPresentationController &spreadController)
        : m_state(state)
        , m_navigationController(navigationController)
        , m_predecodeController(predecodeController)
        , m_openController(openController)
        , m_spreadController(spreadController)
    {
    }

    void apply(const KiriView::ImageDocumentSourceLoadRequest &request,
        const KiriView::ImageSourceLoadPlan &plan)
    {
        for (KiriView::ImageSourceLoadAction action : plan.actions) {
            applyAction(request, action);
        }
    }

private:
    void applyAction(const KiriView::ImageDocumentSourceLoadRequest &request,
        KiriView::ImageSourceLoadAction action)
    {
        switch (action) {
        case KiriView::ImageSourceLoadAction::CancelNavigationAndPredecode:
            ::cancelNavigationAndPredecode(m_navigationController, m_predecodeController);
            return;
        case KiriView::ImageSourceLoadAction::FinishSpreadTransition:
            m_spreadController.finishTransition();
            return;
        case KiriView::ImageSourceLoadAction::ResetRightToLeftReading:
            m_spreadController.resetRightToLeftReading();
            return;
        case KiriView::ImageSourceLoadAction::NotifyRightToLeftReading:
            m_spreadController.notifyRightToLeftReadingChanged();
            return;
        case KiriView::ImageSourceLoadAction::ClearSecondaryPage:
            m_spreadController.clearSecondaryPage();
            return;
        case KiriView::ImageSourceLoadAction::ClearLoadingContainerNavigationUrl:
            m_state.clearLoadingContainerNavigationUrl();
            return;
        case KiriView::ImageSourceLoadAction::UpdateContainerNavigationUrl:
            m_state.setContainerNavigationUrl(request.containerNavigationUrl);
            return;
        case KiriView::ImageSourceLoadAction::SetLoadingContainerNavigationUrl:
            m_state.setLoadingContainerNavigationUrl(request.containerNavigationUrl);
            return;
        case KiriView::ImageSourceLoadAction::SetSourceUrl:
            m_state.setSourceUrl(request.sourceUrl);
            return;
        case KiriView::ImageSourceLoadAction::BeginOpen:
            m_openController.open();
            return;
        }
    }

    KiriView::ImageDocumentState &m_state;
    KiriView::ImageDocumentNavigationController &m_navigationController;
    KiriView::ImageDocumentPredecodeController &m_predecodeController;
    KiriView::ImageOpenController &m_openController;
    KiriView::ImageSpreadPresentationController &m_spreadController;
};
}

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

    const ImageSourceLoadPlan plan = ImageOpenWorkflow::sourceLoadPlan(
        ::sourceLoadPolicyInput(m_state, m_spreadController, request));
    ImageSourceLoadPlanApplier(m_state, m_navigationController, m_predecodeController,
        m_openController, m_spreadController)
        .apply(request, plan);
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
    ::cancelNavigationAndPredecode(m_navigationController, m_predecodeController);
    m_openController.cancel();
    m_spreadController.finishTransition();
    m_spreadController.clearSecondaryPage();
    m_state.setSourceUrl(QUrl());
    m_state.setErrorString(QString());
    return ImageOpenWorkflow::finishEmptySourceLoad(m_state);
}

}
