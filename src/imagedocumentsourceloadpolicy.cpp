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

KiriView::ImageDocumentRightToLeftReadingTransition rightToLeftReadingTransition(
    KiriView::RustImageDocumentRightToLeftReadingTransition transition)
{
    using ReadingTransition = KiriView::ImageDocumentRightToLeftReadingTransition;
    using RustReadingTransition = KiriView::RustImageDocumentRightToLeftReadingTransition;

    switch (transition) {
    case RustReadingTransition::Keep:
        return ReadingTransition::Keep;
    case RustReadingTransition::ResetAndNotifyBeforeSourceState:
        return ReadingTransition::ResetAndNotifyBeforeSourceState;
    case RustReadingTransition::ResetAndNotifyAfterOpen:
        return ReadingTransition::ResetAndNotifyAfterOpen;
    }

    return ReadingTransition::Keep;
}

KiriView::ImageDocumentSourceLoadPendingContainerTarget pendingContainerTarget(
    KiriView::RustImageDocumentSourceLoadPendingContainerTarget target)
{
    switch (target) {
    case KiriView::RustImageDocumentSourceLoadPendingContainerTarget::Unchanged:
        return KiriView::ImageDocumentSourceLoadPendingContainerTarget::Unchanged;
    case KiriView::RustImageDocumentSourceLoadPendingContainerTarget::Empty:
        return KiriView::ImageDocumentSourceLoadPendingContainerTarget::Empty;
    case KiriView::RustImageDocumentSourceLoadPendingContainerTarget::RequestedContainerNavigation:
        return KiriView::ImageDocumentSourceLoadPendingContainerTarget::
            RequestedContainerNavigation;
    }

    return KiriView::ImageDocumentSourceLoadPendingContainerTarget::Unchanged;
}

KiriView::ImageDocumentSourceLoadContainerTarget containerTarget(
    KiriView::RustImageDocumentSourceLoadContainerTarget target)
{
    switch (target) {
    case KiriView::RustImageDocumentSourceLoadContainerTarget::Unchanged:
        return KiriView::ImageDocumentSourceLoadContainerTarget::Unchanged;
    case KiriView::RustImageDocumentSourceLoadContainerTarget::RequestedContainerNavigation:
        return KiriView::ImageDocumentSourceLoadContainerTarget::RequestedContainerNavigation;
    }

    return KiriView::ImageDocumentSourceLoadContainerTarget::Unchanged;
}

KiriView::ImageDocumentSourceLoadSourceTarget sourceTarget(
    KiriView::RustImageDocumentSourceLoadSourceTarget target)
{
    switch (target) {
    case KiriView::RustImageDocumentSourceLoadSourceTarget::Unchanged:
        return KiriView::ImageDocumentSourceLoadSourceTarget::Unchanged;
    case KiriView::RustImageDocumentSourceLoadSourceTarget::RequestedSource:
        return KiriView::ImageDocumentSourceLoadSourceTarget::RequestedSource;
    }

    return KiriView::ImageDocumentSourceLoadSourceTarget::Unchanged;
}

KiriView::RustImageDocumentSourceLoadPolicyInput rustSourceLoadPolicyInput(
    const KiriView::ImageDocumentSourceLoadPolicyInput &input)
{
    return KiriView::RustImageDocumentSourceLoadPolicyInput {
        rustSourceLoadKind(input.loadKind),
        input.preserveTwoPageSpreadTransition,
        input.rightToLeftReadingEnabled,
        input.sourceWithinDisplayedComicBookArchive,
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
        pendingContainerTarget(plan.loading_container_navigation_url),
        containerTarget(plan.container_navigation_url),
        sourceTarget(plan.source_url),
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
