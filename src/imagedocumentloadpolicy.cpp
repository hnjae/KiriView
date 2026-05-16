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

KiriView::ImageDocumentRightToLeftReadingTransition rightToLeftReadingTransition(
    KiriView::RustImageDocumentRightToLeftReadingTransition transition)
{
    switch (transition) {
    case KiriView::RustImageDocumentRightToLeftReadingTransition::Keep:
        return KiriView::ImageDocumentRightToLeftReadingTransition::Keep;
    case KiriView::RustImageDocumentRightToLeftReadingTransition::Reset:
        return KiriView::ImageDocumentRightToLeftReadingTransition::Reset;
    case KiriView::RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState:
        return KiriView::ImageDocumentRightToLeftReadingTransition::ResetAndNotifyBeforeSourceState;
    case KiriView::RustImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen:
        return KiriView::ImageDocumentRightToLeftReadingTransition::ResetAndNotifyAfterOpen;
    }

    return KiriView::ImageDocumentRightToLeftReadingTransition::Keep;
}

KiriView::ImageDocumentSourceLoadUrlTarget sourceLoadUrlTarget(
    KiriView::RustImageDocumentSourceLoadUrlTarget target)
{
    switch (target) {
    case KiriView::RustImageDocumentSourceLoadUrlTarget::Unchanged:
        return KiriView::ImageDocumentSourceLoadUrlTarget::Unchanged;
    case KiriView::RustImageDocumentSourceLoadUrlTarget::Empty:
        return KiriView::ImageDocumentSourceLoadUrlTarget::Empty;
    case KiriView::RustImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation:
        return KiriView::ImageDocumentSourceLoadUrlTarget::RequestedContainerNavigation;
    case KiriView::RustImageDocumentSourceLoadUrlTarget::RequestedSource:
        return KiriView::ImageDocumentSourceLoadUrlTarget::RequestedSource;
    }

    return KiriView::ImageDocumentSourceLoadUrlTarget::Unchanged;
}

KiriView::ImageDocumentSourceLoadPlan imageDocumentSourceLoadPlan(
    const KiriView::RustImageDocumentSourceLoadPlan &rustPlan)
{
    return KiriView::ImageDocumentSourceLoadPlan {
        rustPlan.cancel_navigation_and_predecode,
        rustPlan.finish_spread_transition,
        rightToLeftReadingTransition(rustPlan.right_to_left_reading_transition),
        rustPlan.clear_secondary_page,
        sourceLoadUrlTarget(rustPlan.loading_container_navigation_url),
        sourceLoadUrlTarget(rustPlan.container_navigation_url),
        sourceLoadUrlTarget(rustPlan.source_url),
        rustPlan.begin_open,
    };
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
