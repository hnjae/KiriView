// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimagepresentation.h"

#include "predecodecache.h"

namespace {
KiriView::DecodedImagePresentationPlan presentationPlan(
    KiriView::DecodedImagePresentationTarget target, bool predecodeCacheable)
{
    return KiriView::DecodedImagePresentationPlan { target, predecodeCacheable };
}
}

namespace KiriView {
DecodedImagePresentationPlan decodedImagePresentationPlan(const StaticDecodedImage &decoded)
{
    return presentationPlan(DecodedImagePresentationTarget::StaticImage,
        PredecodeCache::canCacheImage(decoded.staticImage));
}

DecodedImagePresentationPlan decodedImagePresentationPlan(const DecodedAnimationImage &decoded)
{
    const DecodedImagePresentationTarget target = decoded.frames.empty()
        ? DecodedImagePresentationTarget::DecodeError
        : DecodedImagePresentationTarget::DecodedAnimation;
    return presentationPlan(target, false);
}

DecodedImagePresentationPlan decodedImagePresentationPlan(const ReaderAnimationImage &)
{
    return presentationPlan(DecodedImagePresentationTarget::ReaderAnimation, false);
}

DecodedImagePresentationPlan decodedImagePresentationPlan(const HeifSequenceAnimationImage &)
{
    return presentationPlan(DecodedImagePresentationTarget::HeifSequenceAnimation, false);
}
}
