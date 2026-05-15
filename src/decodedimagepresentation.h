// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DECODEDIMAGEPRESENTATION_H
#define KIRIVIEW_DECODEDIMAGEPRESENTATION_H

#include "decodedimageresult.h"

namespace KiriView {
struct DecodedImagePresentationPlan {
    bool presentable = false;
    bool predecodeCacheable = false;
};

DecodedImagePresentationPlan decodedImagePresentationPlan(const StaticDecodedImage &decoded);
DecodedImagePresentationPlan decodedImagePresentationPlan(const DecodedAnimationImage &decoded);
DecodedImagePresentationPlan decodedImagePresentationPlan(const ReaderAnimationImage &decoded);
DecodedImagePresentationPlan decodedImagePresentationPlan(
    const HeifSequenceAnimationImage &decoded);
}

#endif
