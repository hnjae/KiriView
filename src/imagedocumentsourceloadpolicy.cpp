// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentsourceloadpolicy.h"

#include "kiriview/src/imagedocumentsourceloadpolicy.cxx.h"

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

KiriView::ImageDocumentRightToLeftReadingTransition rightToLeftReadingTransition(
    KiriView::RustImageDocumentRightToLeftReadingTransition transition)
{
    using ReadingTransition = KiriView::ImageDocumentRightToLeftReadingTransition;
    using RustReadingTransition = KiriView::RustImageDocumentRightToLeftReadingTransition;

    switch (transition) {
    case RustReadingTransition::Keep:
        return ReadingTransition::Keep;
    case RustReadingTransition::Reset:
        return ReadingTransition::Reset;
    case RustReadingTransition::ResetAndNotifyBeforeSourceState:
        return ReadingTransition::ResetAndNotifyBeforeSourceState;
    case RustReadingTransition::ResetAndNotifyAfterOpen:
        return ReadingTransition::ResetAndNotifyAfterOpen;
    }

    return ReadingTransition::Keep;
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

KiriView::RustImageDocumentSourceLoadPolicyInput rustSourceLoadPolicyInput(
    const KiriView::ImageDocumentSourceLoadPolicyInput &input)
{
    return KiriView::RustImageDocumentSourceLoadPolicyInput {
        rustSourceLoadKind(input.loadKind),
        input.preserveTwoPageSpreadTransition,
        rustRightToLeftReadingReset(input.rightToLeftReadingReset),
        input.hasRequestedContainerNavigationUrl,
    };
}

KiriView::ImageDocumentSourceLoadPlan sourceLoadPlan(
    const KiriView::RustImageDocumentSourceLoadPlan &plan)
{
    return KiriView::ImageDocumentSourceLoadPlan {
        plan.cancel_navigation_and_predecode,
        plan.finish_spread_transition,
        rightToLeftReadingTransition(plan.right_to_left_reading_transition),
        plan.clear_secondary_page,
        sourceLoadUrlTarget(plan.loading_container_navigation_url),
        sourceLoadUrlTarget(plan.container_navigation_url),
        sourceLoadUrlTarget(plan.source_url),
        plan.begin_open,
    };
}

}

namespace KiriView::ImageDocumentSourceLoadPolicy {
ImageDocumentSourceLoadPlan plan(const ImageDocumentSourceLoadPolicyInput &input)
{
    return sourceLoadPlan(rustImageDocumentSourceLoadPlan(rustSourceLoadPolicyInput(input)));
}
}
