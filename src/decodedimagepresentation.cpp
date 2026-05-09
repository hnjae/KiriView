// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "decodedimagepresentation.h"

#include "kiriview/src/decodedimagepresentation.cxx.h"
#include "predecodecache.h"

#include <cstddef>

namespace {
KiriView::DecodedImagePresentationTarget decodedImagePresentationTarget(
    KiriView::RustDecodedImagePresentationTarget target)
{
    switch (target) {
    case KiriView::RustDecodedImagePresentationTarget::StaticImage:
        return KiriView::DecodedImagePresentationTarget::StaticImage;
    case KiriView::RustDecodedImagePresentationTarget::DecodedAnimation:
        return KiriView::DecodedImagePresentationTarget::DecodedAnimation;
    case KiriView::RustDecodedImagePresentationTarget::ReaderAnimation:
        return KiriView::DecodedImagePresentationTarget::ReaderAnimation;
    case KiriView::RustDecodedImagePresentationTarget::HeifSequenceAnimation:
        return KiriView::DecodedImagePresentationTarget::HeifSequenceAnimation;
    case KiriView::RustDecodedImagePresentationTarget::DecodeError:
        return KiriView::DecodedImagePresentationTarget::DecodeError;
    }

    return KiriView::DecodedImagePresentationTarget::DecodeError;
}

KiriView::DecodedImagePresentationPlan decodedImagePresentationPlan(
    KiriView::RustDecodedImagePresentationKind kind, std::size_t decodedAnimationFrameCount,
    bool staticImageCacheable)
{
    const KiriView::RustDecodedImagePresentationPlan plan
        = KiriView::rustDecodedImagePresentationPlan(
            kind, decodedAnimationFrameCount, staticImageCacheable);
    return KiriView::DecodedImagePresentationPlan {
        decodedImagePresentationTarget(plan.target),
        plan.predecode_cacheable,
    };
}
}

namespace KiriView {
DecodedImagePresentationPlan decodedImagePresentationPlan(const StaticDecodedImage &decoded)
{
    return ::decodedImagePresentationPlan(RustDecodedImagePresentationKind::StaticImage, 0,
        PredecodeCache::canCacheImage(decoded.staticImage));
}

DecodedImagePresentationPlan decodedImagePresentationPlan(const DecodedAnimationImage &decoded)
{
    return ::decodedImagePresentationPlan(
        RustDecodedImagePresentationKind::DecodedAnimation, decoded.frames.size(), false);
}

DecodedImagePresentationPlan decodedImagePresentationPlan(const ReaderAnimationImage &)
{
    return ::decodedImagePresentationPlan(
        RustDecodedImagePresentationKind::ReaderAnimation, 0, false);
}

DecodedImagePresentationPlan decodedImagePresentationPlan(const HeifSequenceAnimationImage &)
{
    return ::decodedImagePresentationPlan(
        RustDecodedImagePresentationKind::HeifSequenceAnimation, 0, false);
}
}
