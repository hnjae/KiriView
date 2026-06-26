// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGESPREADSECONDARYPAGEREFRESHPOLICY_H
#define KIRIVIEW_IMAGESPREADSECONDARYPAGEREFRESHPOLICY_H

namespace kiriview {
enum class ImageSpreadSecondaryPageDecision {
    PrimaryOnly,
    LoadNext,
    KeepCurrentSecondary,
};

struct ImageSpreadSecondaryPageRefreshState
{
    bool twoPageModeActive = false;
    int currentPageNumber = 0;
    int pageCount = 0;
    bool primaryPageIsWide = false;
    bool nextPageAvailable = false;
    bool nextPageIsWide = false;
    bool currentSecondaryMatchesNext = false;
};

struct ImageSpreadSecondaryPageRefreshPlan
{
    ImageSpreadSecondaryPageDecision decision = ImageSpreadSecondaryPageDecision::PrimaryOnly;
    int targetPageNumber = 0;
};

ImageSpreadSecondaryPageRefreshPlan imageSpreadSecondaryPageRefreshPlan(
    const ImageSpreadSecondaryPageRefreshState& state);
}

#endif
