// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADPOLICY_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADPOLICY_H

#include "imagedocumenteffectplan.h"
#include "imagedocumentsourceloadrequest.h"
#include "location/imagelocation.h"

#include <QUrl>

namespace KiriView {
enum class ImageDocumentSourceLoadKind {
    CurrentSource,
    ReplacementSource,
};

struct ImageDocumentSourceLoadPolicyInput {
    ImageDocumentSourceLoadKind loadKind = ImageDocumentSourceLoadKind::CurrentSource;
    bool preserveTwoPageSpreadTransition = false;
    bool rightToLeftReadingEnabled = false;
    bool sourceWithinDisplayedComicBookArchive = false;
    bool hasRequestedContainerNavigationUrl = false;
};

struct ImageDocumentSourceLoadSnapshot {
    QUrl currentSourceUrl;
    ArchiveDocumentLocation displayedArchiveDocument;
    bool rightToLeftReadingEnabled = false;
};

ImageDocumentSourceLoadPolicyInput imageDocumentSourceLoadPolicyInput(
    const ImageDocumentSourceLoadSnapshot &snapshot, const ImageDocumentSourceLoadRequest &request);

namespace ImageDocumentSourceLoadPolicy {
    ImageDocumentRuntimePlan plan(const ImageDocumentSourceLoadPolicyInput &input,
        const ImageDocumentSourceLoadRequest &request);
}
}

#endif
