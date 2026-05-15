// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadpolicy.h"

#include "kiriview/src/imageopenworkflow.cxx.h"

namespace {
KiriView::RustImageSourceLoadPolicyInput rustSourceLoadPolicyInput(
    const KiriView::ImageSourceLoadPolicyInput &input)
{
    KiriView::RustImageSourceLoadPolicyInput rustInput {};
    rustInput.source_url_changed = input.sourceUrlChanged;
    rustInput.preserve_two_page_spread_transition = input.preserveTwoPageSpreadTransition;
    rustInput.reset_right_to_left_reading = input.resetRightToLeftReading;
    rustInput.right_to_left_reading_was_enabled = input.rightToLeftReadingWasEnabled;
    rustInput.requested_container_navigation_url_empty = input.requestedContainerNavigationUrlEmpty;
    return rustInput;
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

    return KiriView::ImageSourceLoadAction::BeginOpen;
}

KiriView::ImageSourceLoadPlan imageSourceLoadPlan(const KiriView::RustImageSourceLoadPlan &rustPlan)
{
    KiriView::ImageSourceLoadPlan plan;
    plan.actions.reserve(rustPlan.actions.size());
    for (KiriView::RustImageSourceLoadAction action : rustPlan.actions) {
        plan.actions.push_back(imageSourceLoadAction(action));
    }
    return plan;
}
}

namespace KiriView::ImageDocumentSourceLoadPolicy {
ImageSourceLoadPlan sourceLoadPlan(const ImageSourceLoadPolicyInput &input)
{
    return imageSourceLoadPlan(rustImageSourceLoadPlan(rustSourceLoadPolicyInput(input)));
}
}
