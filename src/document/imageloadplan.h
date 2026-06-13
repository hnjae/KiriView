// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADPLAN_H
#define KIRIVIEW_IMAGELOADPLAN_H

#include "imageloadtypes.h"

#include <QtGlobal>

namespace kiriview {
enum class OpenedCollectionScopeLoadEffect {
    ReadImage,
    LoadImageDocumentPageCandidates,
};

enum class ImageLoadStartEffect {
    DecodeImage,
    LoadOpenedCollectionScopeCandidates,
};

struct OpenedCollectionScopeLoadPlan {
    OpenedCollectionScopeLocation openedCollectionScope;
    OpenedCollectionScopeLoadEffect effect = OpenedCollectionScopeLoadEffect::ReadImage;
};

struct ImageLoadPlan {
    ImageLoadSession session;
    ImageLoadStartEffect startEffect = ImageLoadStartEffect::DecodeImage;
};

OpenedCollectionScopeLoadPlan openedCollectionScopeLoadPlan(const ImageLoadRequest &request);
ImageLoadPlan imageLoadPlan(
    quint64 id, ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext = {});
}

#endif
