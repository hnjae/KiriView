// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimagepresentation.h"

#include "predecodecache.h"

namespace {
KiriView::DecodedImagePresentationPlan presentationPlan(bool presentable, bool predecodeCacheable)
{
    return KiriView::DecodedImagePresentationPlan { presentable, predecodeCacheable };
}
}

namespace KiriView {
DecodedImagePresentationPlan decodedImagePresentationPlan(const StaticDecodedImage &decoded)
{
    return presentationPlan(true, PredecodeCache::canCacheImage(decoded.staticImage));
}

DecodedImagePresentationPlan decodedImagePresentationPlan(const ApngAnimationImage &decoded)
{
    return presentationPlan(!decoded.firstFrame.isNull(), false);
}

DecodedImagePresentationPlan decodedImagePresentationPlan(const ReaderAnimationImage &)
{
    return presentationPlan(true, false);
}

DecodedImagePresentationPlan decodedImagePresentationPlan(const HeifSequenceAnimationImage &)
{
    return presentationPlan(true, false);
}
}
