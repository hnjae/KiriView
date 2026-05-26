// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADPLAN_H
#define KIRIVIEW_IMAGELOADPLAN_H

#include "imageloadtypes.h"

#include <QtGlobal>

namespace KiriView {
enum class ImagePageScopeLoadEffect {
    ReadImage,
    LoadImageCandidates,
};

enum class ImageLoadStartEffect {
    DecodeImage,
    LoadImagePageScopeCandidates,
};

struct ImagePageScopeLoadPlan {
    ImagePageScopeLocation imagePageScope;
    ImagePageScopeLoadEffect effect = ImagePageScopeLoadEffect::ReadImage;
};

struct ImageLoadPlan {
    ImageLoadSession session;
    ImageLoadStartEffect startEffect = ImageLoadStartEffect::DecodeImage;
};

ImagePageScopeLoadPlan imagePageScopeLoadPlan(const ImageLoadRequest &request);
ImageLoadPlan imageLoadPlan(
    quint64 id, ImageLoadRequest request, ImageFirstDisplayDecodeContext firstDisplayContext = {});
}

#endif
