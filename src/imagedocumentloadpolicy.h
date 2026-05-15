// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTLOADPOLICY_H
#define KIRIVIEW_IMAGEDOCUMENTLOADPOLICY_H

#include <vector>

namespace KiriView {
enum class ImageDocumentSourceLoadAction {
    CancelNavigationAndPredecode,
    FinishSpreadTransition,
    ResetRightToLeftReading,
    NotifyRightToLeftReading,
    ClearSecondaryPage,
    ClearLoadingContainerNavigationUrl,
    UpdateContainerNavigationUrl,
    SetLoadingContainerNavigationUrl,
    SetSourceUrl,
    BeginOpen,
};

enum class ImageDocumentSourceLoadKind {
    CurrentSource,
    ReplacementSource,
};

struct ImageDocumentSourceLoadPolicyInput {
    ImageDocumentSourceLoadKind loadKind = ImageDocumentSourceLoadKind::CurrentSource;
    bool preserveTwoPageSpreadTransition = false;
    bool resetRightToLeftReading = false;
    bool rightToLeftReadingWasEnabled = false;
    bool hasRequestedContainerNavigationUrl = false;
};

struct ImageDocumentSourceLoadPlan {
    std::vector<ImageDocumentSourceLoadAction> actions;
};

namespace ImageDocumentLoadPolicy {
    ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentSourceLoadPolicyInput &input);
}
}

#endif
