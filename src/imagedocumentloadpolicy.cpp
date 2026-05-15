// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadpolicy.h"

#include "kiriview/src/imagedocumentloadpolicy.cxx.h"

namespace {
KiriView::RustImageDocumentSourceLoadKind rustSourceLoadKind(
    KiriView::ImageDocumentSourceLoadKind loadKind)
{
    switch (loadKind) {
    case KiriView::ImageDocumentSourceLoadKind::CurrentSource:
        return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
    case KiriView::ImageDocumentSourceLoadKind::ReplacementSource:
        return KiriView::RustImageDocumentSourceLoadKind::ReplacementSource;
    }

    return KiriView::RustImageDocumentSourceLoadKind::CurrentSource;
}

KiriView::RustImageDocumentRightToLeftReadingReset rustRightToLeftReadingReset(
    KiriView::ImageDocumentRightToLeftReadingReset reset)
{
    switch (reset) {
    case KiriView::ImageDocumentRightToLeftReadingReset::Keep:
        return KiriView::RustImageDocumentRightToLeftReadingReset::Keep;
    case KiriView::ImageDocumentRightToLeftReadingReset::ResetInactive:
        return KiriView::RustImageDocumentRightToLeftReadingReset::ResetInactive;
    case KiriView::ImageDocumentRightToLeftReadingReset::ResetActive:
        return KiriView::RustImageDocumentRightToLeftReadingReset::ResetActive;
    }

    return KiriView::RustImageDocumentRightToLeftReadingReset::Keep;
}

KiriView::RustImageDocumentSourceLoadPolicyInput rustSourceLoadPolicyInput(
    const KiriView::ImageDocumentSourceLoadPolicyInput &input)
{
    KiriView::RustImageDocumentSourceLoadPolicyInput rustInput {};
    rustInput.load_kind = rustSourceLoadKind(input.loadKind);
    rustInput.preserve_two_page_spread_transition = input.preserveTwoPageSpreadTransition;
    rustInput.right_to_left_reading_reset
        = rustRightToLeftReadingReset(input.rightToLeftReadingReset);
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
