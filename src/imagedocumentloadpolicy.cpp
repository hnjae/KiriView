// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentloadpolicy.h"

#include "imagedocumentsourceloadrequest.h"
#include "imagedocumentstate.h"
#include "imagespreadpresentationcontroller.h"
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

KiriView::ImageDocumentSourceLoadKind sourceLoadKind(const KiriView::ImageDocumentState &state,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    if (state.sourceUrl() == request.sourceUrl) {
        return KiriView::ImageDocumentSourceLoadKind::CurrentSource;
    }

    return KiriView::ImageDocumentSourceLoadKind::ReplacementSource;
}

KiriView::ImageDocumentRightToLeftReadingReset rightToLeftReadingReset(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImageSpreadPresentationController &spreadController,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    if (!spreadController.shouldResetRightToLeftReadingForLoad(
            state.displayedArchiveDocument(), request.sourceUrl, request.containerNavigationUrl)) {
        return KiriView::ImageDocumentRightToLeftReadingReset::Keep;
    }

    return spreadController.rightToLeftReadingEnabled()
        ? KiriView::ImageDocumentRightToLeftReadingReset::ResetActive
        : KiriView::ImageDocumentRightToLeftReadingReset::ResetInactive;
}

KiriView::ImageDocumentSourceLoadPolicyInput sourceLoadPolicyInput(
    const KiriView::ImageDocumentState &state,
    const KiriView::ImageSpreadPresentationController &spreadController,
    const KiriView::ImageDocumentSourceLoadRequest &request)
{
    KiriView::ImageDocumentSourceLoadPolicyInput input;
    input.loadKind = sourceLoadKind(state, request);
    input.preserveTwoPageSpreadTransition = request.preserveTwoPageSpreadTransition;
    input.rightToLeftReadingReset = rightToLeftReadingReset(state, spreadController, request);
    input.hasRequestedContainerNavigationUrl = !request.containerNavigationUrl.isEmpty();
    return input;
}
}

namespace KiriView::ImageDocumentLoadPolicy {
ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentSourceLoadPolicyInput &input)
{
    return imageDocumentSourceLoadPlan(
        rustImageDocumentSourceLoadPlan(rustSourceLoadPolicyInput(input)));
}

ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentState &state,
    const ImageSpreadPresentationController &spreadController,
    const ImageDocumentSourceLoadRequest &request)
{
    return sourceLoadPlan(::sourceLoadPolicyInput(state, spreadController, request));
}
}
