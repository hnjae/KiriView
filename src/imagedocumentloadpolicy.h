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

struct ImageDocumentSourceLoadPolicyInput {
    bool sourceUrlChanged = false;
    bool preserveTwoPageSpreadTransition = false;
    bool resetRightToLeftReading = false;
    bool rightToLeftReadingWasEnabled = false;
    bool requestedContainerNavigationUrlEmpty = false;
};

struct ImageDocumentSourceLoadPlan {
    std::vector<ImageDocumentSourceLoadAction> actions;
};

namespace ImageDocumentLoadPolicy {
    ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentSourceLoadPolicyInput &input);
}
}

#endif
