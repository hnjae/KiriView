// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "presentation/imagespreadsecondarypagerefreshpolicy.h"

#include "kiriview/src/policy/imagespreadgeometry.cxx.h"

namespace {
KiriView::ImageSpreadSecondaryPageDecision imageSpreadSecondaryPageDecision(
    KiriView::RustImageSpreadSecondaryPageDecision decision)
{
    switch (decision) {
    case KiriView::RustImageSpreadSecondaryPageDecision::PrimaryOnly:
        return KiriView::ImageSpreadSecondaryPageDecision::PrimaryOnly;
    case KiriView::RustImageSpreadSecondaryPageDecision::LoadNext:
        return KiriView::ImageSpreadSecondaryPageDecision::LoadNext;
    case KiriView::RustImageSpreadSecondaryPageDecision::KeepCurrentSecondary:
        return KiriView::ImageSpreadSecondaryPageDecision::KeepCurrentSecondary;
    }

    return KiriView::ImageSpreadSecondaryPageDecision::PrimaryOnly;
}

KiriView::RustImageSpreadSecondaryPageRefreshState rustSecondaryPageRefreshState(
    const KiriView::ImageSpreadSecondaryPageRefreshState &state)
{
    return KiriView::RustImageSpreadSecondaryPageRefreshState { state.twoPageModeActive,
        state.currentPageNumber, state.imageCount, state.primaryPageIsWide, state.nextPageAvailable,
        state.nextPageIsWide, state.currentSecondaryMatchesNext };
}
}

namespace KiriView {
ImageSpreadSecondaryPageRefreshPlan imageSpreadSecondaryPageRefreshPlan(
    const ImageSpreadSecondaryPageRefreshState &state)
{
    const RustImageSpreadSecondaryPageRefreshPlan plan
        = rustImageSpreadSecondaryPageRefreshPlan(rustSecondaryPageRefreshState(state));
    return ImageSpreadSecondaryPageRefreshPlan {
        ::imageSpreadSecondaryPageDecision(plan.decision),
        plan.target_page_number,
    };
}
}
