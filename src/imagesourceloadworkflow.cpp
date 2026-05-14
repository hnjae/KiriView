// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagesourceloadworkflow.h"

#include "kiriview/src/imagesourceloadworkflow.cxx.h"

#include <QtGlobal>

namespace {
KiriView::RustImageSourceLoadRequest rustImageSourceLoadRequest(
    const KiriView::ImageSourceLoadRequest &request)
{
    KiriView::RustImageSourceLoadRequest rustRequest {};
    rustRequest.source_url_changed = request.sourceUrlChanged;
    rustRequest.preserve_two_page_spread_transition = request.preserveTwoPageSpreadTransition;
    rustRequest.reset_right_to_left_reading = request.resetRightToLeftReading;
    rustRequest.right_to_left_reading_enabled = request.rightToLeftReadingEnabled;
    rustRequest.container_navigation_url_empty = request.containerNavigationUrlEmpty;
    return rustRequest;
}

KiriView::ImageSourceLoadAction imageSourceLoadAction(KiriView::RustImageSourceLoadAction action)
{
    switch (action) {
    case KiriView::RustImageSourceLoadAction::CancelNavigationAndPredecode:
        return KiriView::ImageSourceLoadAction::CancelNavigationAndPredecode;
    case KiriView::RustImageSourceLoadAction::FinishSpreadTransition:
        return KiriView::ImageSourceLoadAction::FinishSpreadTransition;
    case KiriView::RustImageSourceLoadAction::ResetRightToLeftReading:
        return KiriView::ImageSourceLoadAction::ResetRightToLeftReading;
    case KiriView::RustImageSourceLoadAction::NotifyRightToLeftReading:
        return KiriView::ImageSourceLoadAction::NotifyRightToLeftReading;
    case KiriView::RustImageSourceLoadAction::ClearSecondaryPage:
        return KiriView::ImageSourceLoadAction::ClearSecondaryPage;
    case KiriView::RustImageSourceLoadAction::ClearLoadingContainerNavigationUrl:
        return KiriView::ImageSourceLoadAction::ClearLoadingContainerNavigationUrl;
    case KiriView::RustImageSourceLoadAction::UpdateContainerNavigationUrl:
        return KiriView::ImageSourceLoadAction::UpdateContainerNavigationUrl;
    case KiriView::RustImageSourceLoadAction::SetLoadingContainerNavigationUrl:
        return KiriView::ImageSourceLoadAction::SetLoadingContainerNavigationUrl;
    case KiriView::RustImageSourceLoadAction::SetSourceUrl:
        return KiriView::ImageSourceLoadAction::SetSourceUrl;
    case KiriView::RustImageSourceLoadAction::BeginOpen:
        return KiriView::ImageSourceLoadAction::BeginOpen;
    }

    Q_UNREACHABLE_RETURN(KiriView::ImageSourceLoadAction::BeginOpen);
}

KiriView::ImageSourceLoadPlan imageSourceLoadPlan(KiriView::RustImageSourceLoadPlan rustPlan)
{
    KiriView::ImageSourceLoadPlan plan;
    plan.actions.reserve(rustPlan.actions.size());
    for (KiriView::RustImageSourceLoadAction action : rustPlan.actions) {
        plan.actions.push_back(imageSourceLoadAction(action));
    }
    return plan;
}
}

namespace KiriView::ImageSourceLoadWorkflow {
ImageSourceLoadPlan plan(const ImageSourceLoadRequest &request)
{
    return imageSourceLoadPlan(rustImageSourceLoadPlan(rustImageSourceLoadRequest(request)));
}
}
