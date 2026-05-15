// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADPOLICY_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADPOLICY_H

#include <vector>

namespace KiriView {
enum class ImageSourceLoadAction {
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

struct ImageSourceLoadPolicyInput {
    bool sourceUrlChanged = false;
    bool preserveTwoPageSpreadTransition = false;
    bool resetRightToLeftReading = false;
    bool rightToLeftReadingEnabled = false;
    bool containerNavigationUrlEmpty = false;
};

struct ImageSourceLoadPlan {
    std::vector<ImageSourceLoadAction> actions;
};

namespace ImageDocumentSourceLoadPolicy {
    ImageSourceLoadPlan sourceLoadPlan(const ImageSourceLoadPolicyInput &input);
}
}

#endif
