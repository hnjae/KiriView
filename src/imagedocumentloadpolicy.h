// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTLOADPOLICY_H
#define KIRIVIEW_IMAGEDOCUMENTLOADPOLICY_H

#include <vector>

namespace KiriView {
class ImageDocumentState;
class ImageSpreadPresentationController;
struct ImageDocumentSourceLoadRequest;

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

enum class ImageDocumentRightToLeftReadingReset {
    Keep,
    ResetInactive,
    ResetActive,
};

struct ImageDocumentSourceLoadPolicyInput {
    ImageDocumentSourceLoadKind loadKind = ImageDocumentSourceLoadKind::CurrentSource;
    bool preserveTwoPageSpreadTransition = false;
    ImageDocumentRightToLeftReadingReset rightToLeftReadingReset
        = ImageDocumentRightToLeftReadingReset::Keep;
    bool hasRequestedContainerNavigationUrl = false;
};

struct ImageDocumentSourceLoadPlan {
    std::vector<ImageDocumentSourceLoadAction> actions;
};

namespace ImageDocumentLoadPolicy {
    ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentSourceLoadPolicyInput &input);
    ImageDocumentSourceLoadPlan sourceLoadPlan(const ImageDocumentState &state,
        const ImageSpreadPresentationController &spreadController,
        const ImageDocumentSourceLoadRequest &request);
}
}

#endif
