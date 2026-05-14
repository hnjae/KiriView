// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagesourceloadworkflow.h"

namespace {
void appendInitialLoadActions(
    KiriView::ImageSourceLoadPlan *plan, const KiriView::ImageSourceLoadRequest &request)
{
    if (request.sourceUrlChanged) {
        plan->actions.push_back(KiriView::ImageSourceLoadAction::CancelNavigationAndPredecode);
    }
    if (!request.preserveTwoPageSpreadTransition) {
        plan->actions.push_back(KiriView::ImageSourceLoadAction::FinishSpreadTransition);
    }
    if (request.resetRightToLeftReading) {
        plan->actions.push_back(KiriView::ImageSourceLoadAction::ResetRightToLeftReading);
    }
}

void appendUnchangedSourceActions(
    KiriView::ImageSourceLoadPlan *plan, const KiriView::ImageSourceLoadRequest &request)
{
    if (request.resetRightToLeftReading && request.rightToLeftReadingEnabled) {
        plan->actions.push_back(KiriView::ImageSourceLoadAction::NotifyRightToLeftReading);
    }
    plan->actions.push_back(KiriView::ImageSourceLoadAction::ClearLoadingContainerNavigationUrl);
    if (!request.containerNavigationUrlEmpty) {
        plan->actions.push_back(KiriView::ImageSourceLoadAction::UpdateContainerNavigationUrl);
    }
}

void appendChangedSourceActions(
    KiriView::ImageSourceLoadPlan *plan, const KiriView::ImageSourceLoadRequest &request)
{
    plan->actions.push_back(KiriView::ImageSourceLoadAction::ClearSecondaryPage);
    plan->actions.push_back(KiriView::ImageSourceLoadAction::SetLoadingContainerNavigationUrl);
    plan->actions.push_back(KiriView::ImageSourceLoadAction::SetSourceUrl);
    plan->actions.push_back(KiriView::ImageSourceLoadAction::BeginOpen);
    if (request.resetRightToLeftReading && request.rightToLeftReadingEnabled) {
        plan->actions.push_back(KiriView::ImageSourceLoadAction::NotifyRightToLeftReading);
    }
}
}

namespace KiriView::ImageSourceLoadWorkflow {
ImageSourceLoadPlan plan(const ImageSourceLoadRequest &request)
{
    ImageSourceLoadPlan plan;
    appendInitialLoadActions(&plan, request);
    if (request.sourceUrlChanged) {
        appendChangedSourceActions(&plan, request);
    } else {
        appendUnchangedSourceActions(&plan, request);
    }
    return plan;
}
}
