// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADPOLICY_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADPOLICY_H

namespace KiriView {
enum class ImageDocumentSourceLoadKind {
    CurrentSource,
    ReplacementSource,
};

enum class ImageDocumentRightToLeftReadingTransition {
    Keep,
    ResetAndNotifyBeforeSourceState,
    ResetAndNotifyAfterOpen,
};

enum class ImageDocumentSourceLoadUrlTarget {
    Unchanged,
    Empty,
    RequestedContainerNavigation,
    RequestedSource,
};

struct ImageDocumentSourceLoadPolicyInput {
    ImageDocumentSourceLoadKind loadKind = ImageDocumentSourceLoadKind::CurrentSource;
    bool preserveTwoPageSpreadTransition = false;
    bool rightToLeftReadingEnabled = false;
    bool sourceWithinDisplayedComicBookArchive = false;
    bool hasRequestedContainerNavigationUrl = false;
};

struct ImageDocumentSourceLoadPlan {
    bool cancelNavigationAndPredecode = false;
    bool finishSpreadTransition = false;
    ImageDocumentRightToLeftReadingTransition rightToLeftReadingTransition
        = ImageDocumentRightToLeftReadingTransition::Keep;
    bool clearSecondaryPage = false;
    ImageDocumentSourceLoadUrlTarget loadingContainerNavigationUrl
        = ImageDocumentSourceLoadUrlTarget::Unchanged;
    ImageDocumentSourceLoadUrlTarget containerNavigationUrl
        = ImageDocumentSourceLoadUrlTarget::Unchanged;
    ImageDocumentSourceLoadUrlTarget sourceUrl = ImageDocumentSourceLoadUrlTarget::Unchanged;
    bool beginOpen = false;
};

namespace ImageDocumentSourceLoadPolicy {
    ImageDocumentSourceLoadPlan plan(const ImageDocumentSourceLoadPolicyInput &input);
}
}

#endif
