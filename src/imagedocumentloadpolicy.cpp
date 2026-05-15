// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadpolicy.h"

#include "kiriview/src/imagedocumentloadpolicy.cxx.h"

namespace {
KiriView::RustImageDocumentSourceLoadPolicyInput rustSourceLoadPolicyInput(
    const KiriView::ImageDocumentSourceLoadPolicyInput &input)
{
    KiriView::RustImageDocumentSourceLoadPolicyInput rustInput {};
    rustInput.replace_source = input.replaceSource;
    rustInput.preserve_two_page_spread_transition = input.preserveTwoPageSpreadTransition;
    rustInput.reset_right_to_left_reading = input.resetRightToLeftReading;
    rustInput.right_to_left_reading_was_enabled = input.rightToLeftReadingWasEnabled;
    rustInput.has_requested_container_navigation_url = input.hasRequestedContainerNavigationUrl;
    return rustInput;
}

KiriView::ImageDocumentSourceLoadAction imageDocumentSourceLoadAction(
    KiriView::RustImageDocumentSourceLoadAction action)
{
    switch (action) {
    case KiriView::RustImageDocumentSourceLoadAction::CancelNavigationAndPredecode:
        return KiriView::ImageDocumentSourceLoadAction::CancelNavigationAndPredecode;
    case KiriView::RustImageDocumentSourceLoadAction::FinishSpreadTransition:
        return KiriView::ImageDocumentSourceLoadAction::FinishSpreadTransition;
    case KiriView::RustImageDocumentSourceLoadAction::ResetRightToLeftReading:
        return KiriView::ImageDocumentSourceLoadAction::ResetRightToLeftReading;
    case KiriView::RustImageDocumentSourceLoadAction::NotifyRightToLeftReading:
        return KiriView::ImageDocumentSourceLoadAction::NotifyRightToLeftReading;
    case KiriView::RustImageDocumentSourceLoadAction::ClearSecondaryPage:
        return KiriView::ImageDocumentSourceLoadAction::ClearSecondaryPage;
    case KiriView::RustImageDocumentSourceLoadAction::ClearLoadingContainerNavigationUrl:
        return KiriView::ImageDocumentSourceLoadAction::ClearLoadingContainerNavigationUrl;
    case KiriView::RustImageDocumentSourceLoadAction::UpdateContainerNavigationUrl:
        return KiriView::ImageDocumentSourceLoadAction::UpdateContainerNavigationUrl;
    case KiriView::RustImageDocumentSourceLoadAction::SetLoadingContainerNavigationUrl:
        return KiriView::ImageDocumentSourceLoadAction::SetLoadingContainerNavigationUrl;
    case KiriView::RustImageDocumentSourceLoadAction::SetSourceUrl:
        return KiriView::ImageDocumentSourceLoadAction::SetSourceUrl;
    case KiriView::RustImageDocumentSourceLoadAction::BeginOpen:
        return KiriView::ImageDocumentSourceLoadAction::BeginOpen;
    }

    return KiriView::ImageDocumentSourceLoadAction::BeginOpen;
}

KiriView::ImageDocumentSourceLoadPlan imageDocumentSourceLoadPlan(
    const KiriView::RustImageDocumentSourceLoadPlan &rustPlan)
{
    KiriView::ImageDocumentSourceLoadPlan plan;
    plan.actions.reserve(rustPlan.actions.size());
    for (KiriView::RustImageDocumentSourceLoadAction action : rustPlan.actions) {
        plan.actions.push_back(imageDocumentSourceLoadAction(action));
    }
    return plan;
}
}

namespace KiriView::ImageDocumentLoadPolicy {
ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentSourceLoadPolicyInput &input)
{
    return imageDocumentSourceLoadPlan(
        rustImageDocumentSourceLoadPlan(rustSourceLoadPolicyInput(input)));
}
}
